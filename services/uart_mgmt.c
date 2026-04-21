/*
 * uart_mgmt.c — UART management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * IRQ flow (chip → app)
 * ─────────────────────
 *
 *  [startup_stm32f411vetx.c]  vector table — weak alias USART1_IRQHandler
 *           │ overridden by strong symbol in…
 *           ▼
 *  [hal_it_stm32.c]  USART1_IRQHandler() { hal_uart_stm32_irq_handler(USART1); }
 *           │
 *           ▼
 *  [hal_uart_stm32.c]  HAL_UART_IRQHandler()  →  HAL_UART_RxCpltCallback()
 *           │                                      HAL_UART_TxCpltCallback()
 *           │                                      HAL_UART_ErrorCallback()
 *           │
 *           ▼  irq_notify_from_isr(IRQ_ID_UART_RX(n), &byte, pxHPT)
 *  [irq/irq.c]  generic pub/sub dispatch
 *           │
 *           ├──► _uart_rx_cb  (this file, ISR context)
 *           │      → ringbuffer_putchar(rb, byte)
 *           │
 *           └──► app-registered callbacks (e.g. echo_task wakeup)
 *                  → vTaskNotifyGiveFromISR(app_task, pxHPT)
 *
 * Thread lifecycle
 * ────────────────
 *   1. uart_mgmt_start() creates the FreeRTOS task and management queue.
 *   2. After TIME_OFFSET_SERIAL_MANAGEMENT ms the task registers every
 *      enabled UART with the generic driver layer and subscribes to the
 *      generic IRQ notification framework for RX, TX-done, and error events.
 *   3. The task then loops on the management queue, processing transmit /
 *      reinit / deinit commands.
 */

#include <services/uart_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_handle.h>
#include <board/board_config.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>
#include <irq/irq_notify.h>

/* Vendor HAL selection */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/com/hal/stm32/hal_uart_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <drivers/com/hal/infineon/hal_uart_infineon.h>
#endif

#if (INC_SERVICE_UART_MGMT == 1)

/* ── Private state ────────────────────────────────────────────────────────── */

static QueueHandle_t _mgmt_queue = NULL;

/* ── IRQ subscriber callbacks (all run in ISR context) ───────────────────── */

/*
 * _uart_rx_cb — byte received on a UART channel.
 * arg = struct ringbuffer * for the channel's RX queue.
 * data = uint8_t * to the received byte (lives in the HAL staging array).
 */
static void _uart_rx_cb(irq_id_t      id,
                         void         *data,
                         void         *arg,
                         BaseType_t   *pxHPT)
{
    (void)id;
    (void)pxHPT;
    if (data == NULL || arg == NULL)
        return;
    ringbuffer_putchar((struct ringbuffer *)arg, *(uint8_t *)data);
}

/*
 * _uart_err_cb — peripheral error detected.
 * arg = drv_uart_handle_t * for the channel.
 */
static void _uart_err_cb(irq_id_t      id,
                          void         *data,
                          void         *arg,
                          BaseType_t   *pxHPT)
{
    (void)id;
    (void)data;
    (void)pxHPT;
    drv_uart_handle_t *h = (drv_uart_handle_t *)arg;
    if (h != NULL)
        h->last_err = OS_ERR_OP;
}

/* ── Management thread ────────────────────────────────────────────────────── */

static void uart_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT);

    /* ── Register and initialise all enabled UART devices ── */
    {
        const drv_uart_hal_ops_t *ops =
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
            hal_uart_stm32_get_ops();
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
            hal_uart_infineon_get_ops();
#else
            NULL;
#endif
        if (ops != NULL)
        {
            const board_config_t *bc = board_get_config();

            for (uint8_t i = 0; i < bc->uart_count; i++)
            {
                const board_uart_desc_t *d = &bc->uart_table[i];
                drv_uart_handle_t       *h = drv_uart_get_handle(d->dev_id);

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
                hal_uart_stm32_set_config(h, d->instance,
                                          d->word_len, d->stop_bits,
                                          d->parity,   d->mode);
#endif
                int32_t err = drv_uart_register(d->dev_id, ops,
                                                d->baudrate, 10);
                (void)err;

                /*
                 * Subscribe to the generic IRQ notification framework.
                 *
                 * _uart_rx_cb feeds every received byte straight into the
                 * channel's RX ring buffer, which callers drain via
                 * uart_mgmt_read_byte().  The arg pointer is retrieved here
                 * after ipc_queues_init() has already allocated the buffer.
                 */
                struct ringbuffer *rb = (struct ringbuffer *)
                    ipc_mqueue_get_handle(
                        global_uart_rx_mqueue_list[d->dev_id]);

                irq_register(IRQ_ID_UART_RX(d->dev_id),    _uart_rx_cb,  rb);
                irq_register(IRQ_ID_UART_ERROR(d->dev_id),  _uart_err_cb, h);
            }
        }
    }

    /* ── Main event loop ── */
    uart_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        drv_uart_handle_t *h = drv_uart_get_handle(msg.dev_id);
        if (h == NULL)
            continue;

        int32_t result = OS_ERR_OP;

        switch (msg.cmd)
        {
            case UART_MGMT_CMD_TRANSMIT:
                if (msg.data != NULL && msg.len > 0)
                {
                    result = h->ops->transmit(h, msg.data, msg.len, h->timeout_ms);
                    if (result != OS_ERR_NONE)
                    {
                        h->ops->hw_deinit(h);
                        h->ops->hw_init(h);
                    }
                }
                break;

            case UART_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h);
                result = h->ops->hw_init(h);
                break;

            case UART_MGMT_CMD_DEINIT:
                h->ops->hw_deinit(h);
                result = OS_ERR_NONE;
                break;

            default:
                break;
        }

        if (msg.result_code != NULL)
            *msg.result_code = result;
        if (msg.result_notify != NULL)
            xTaskNotifyGive(msg.result_notify);
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int32_t uart_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(UART_MGMT_QUEUE_DEPTH, sizeof(uart_mgmt_msg_t));
    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(uart_mgmt_thread,
                                   "uart_mgmt",
                                   PROC_SERVICE_SERIAL_MGMT_STACK_SIZE,
                                   PROC_SERVICE_SERIAL_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

QueueHandle_t uart_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

int32_t uart_mgmt_async_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len)
{
    if (_mgmt_queue == NULL || data == NULL || len == 0)
        return OS_ERR_OP;

    uart_mgmt_msg_t msg = {
        .cmd           = UART_MGMT_CMD_TRANSMIT,
        .dev_id        = dev_id,
        .data          = data,
        .len           = len,
        .result_notify = NULL,
        .result_code   = NULL,
    };

    BaseType_t ok = xQueueSend(_mgmt_queue, &msg, 0);
    return (ok == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

int32_t uart_mgmt_read_byte(uint8_t dev_id, uint8_t *byte)
{
    if (dev_id >= NO_OF_UART || byte == NULL)
        return OS_ERR_OP;

    struct ringbuffer *rb =
        (struct ringbuffer *)ipc_mqueue_get_handle(
            global_uart_rx_mqueue_list[dev_id]);

    if (rb == NULL)
        return OS_ERR_OP;

    return (ringbuffer_getchar(rb, byte) > 0U) ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* INC_SERVICE_UART_MGMT == 1 */
