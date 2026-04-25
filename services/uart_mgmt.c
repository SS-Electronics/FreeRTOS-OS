/**
 * @file    uart_mgmt.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   UART management service thread
 *
 * @details
 * This module implements a centralized UART management service running as a
 * FreeRTOS thread. It abstracts UART driver operations and provides:
 *
 *   - Serialized UART access across multiple tasks
 *   - Interrupt-driven RX handling via ring buffers
 *   - Optional synchronous / asynchronous transmit APIs
 *   - Centralized UART initialization from board configuration
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * IRQ FLOW (Hardware → Application Layer)
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   startup_stm32f411vetx.c
 *          │ (weak ISR vector)
 *          ▼
 *   irq_periph_dispatch_generated.c
 *          │ USARTx_IRQHandler()
 *          ▼
 *   hal_uart_stm32.c
 *          │ HAL_UART_IRQHandler()
 *          ├── HAL_UART_RxCpltCallback()
 *          ├── HAL_UART_TxCpltCallback()
 *          └── HAL_UART_ErrorCallback()
 *                  │
 *                  ▼
 *         irq_notify_from_isr()
 *                  │
 *                  ▼
 *   irq subsystem dispatcher (irq/irq.c)
 *          │
 *          ├── _uart_rx_cb  → ringbuffer_putchar()
 *          └── _uart_err_cb → update drv_uart_handle_t
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * THREAD LIFECYCLE
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *  1. uart_mgmt_start()
 *       → creates queue + FreeRTOS thread
 *
 *  2. After TIME_OFFSET_SERIAL_MANAGEMENT delay:
 *       → registers all UARTs from board config
 *       → initializes HAL + driver layer
 *       → subscribes to IRQ notification framework
 *
 *  3. Main loop:
 *       → processes UART_MGMT_CMD_* requests
 *       → performs transmit / reinit / deinit operations
 *
 *  4. RX path is fully interrupt-driven:
 *       → ISR → ringbuffer → user reads via uart_mgmt_read_byte()
 *
 * This design ensures:
 *   - No blocking in ISR context
 *   - Deterministic UART access
 *   - Safe multi-task communication
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <conf_os.h>
#include <services/uart_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_uart.h>

#include <board/board_config.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>

#include <irq/irq_notify.h>

/* ────────────────────────────────────────────────────────────────────────── */
/*                          Vendor HAL Selection                              */
/* ────────────────────────────────────────────────────────────────────────── */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#   include <drivers/hal/stm32/hal_uart_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#   include <drivers/hal/stm32/hal_uart_infineon.h>
#endif

#if (BOARD_UART_COUNT > 0)

/* ────────────────────────────────────────────────────────────────────────── */
/*                              Private State                                 */
/* ────────────────────────────────────────────────────────────────────────── */

/** @brief UART management command queue */
__SECTION_OS_DATA __USED
static QueueHandle_t _mgmt_queue = NULL;

/* ────────────────────────────────────────────────────────────────────────── */
/*                      IRQ Subscriber Callbacks (ISR)                        */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief UART RX interrupt callback.
 *
 * @details
 * Called in ISR context when a byte is received.
 * Pushes received byte into the UART RX ring buffer.
 */
__SECTION_OS __USED
static void _uart_rx_cb(irq_id_t id,
                         void *data,
                         void *arg,
                         BaseType_t *pxHPT)
{
    (void)id;
    (void)pxHPT;

    if (data == NULL || arg == NULL)
        return;

    ringbuffer_putchar((struct ringbuffer *)arg, *(uint8_t *)data);
}

/**
 * @brief UART error interrupt callback.
 *
 * @details
 * Updates driver error state for the corresponding UART handle.
 */
__SECTION_OS __USED
static void _uart_err_cb(irq_id_t id,
                         void *data,
                         void *arg,
                         BaseType_t *pxHPT)
{
    (void)id;
    (void)data;
    (void)pxHPT;

    drv_uart_handle_t *h = (drv_uart_handle_t *)arg;
    if (h != NULL)
        h->last_err = OS_ERR_OP;
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                           UART Management Thread                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief UART management service thread.
 *
 * @details
 * Responsible for:
 *   - UART peripheral initialization
 *   - IRQ subscription setup
 *   - Processing UART management commands
 */
__SECTION_OS __USED
static void uart_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT);

    /* ── UART initialization from board configuration ── */
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
                drv_uart_handle_t *h = drv_uart_get_handle(d->dev_id);

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
                hal_uart_stm32_set_config(h, d->instance,
                                          d->word_len, d->stop_bits,
                                          d->parity, d->mode);
#endif

                (void)drv_uart_register(d->dev_id, ops,
                                        d->baudrate, 10);

                /* ── IRQ subscription setup ── */
                struct ringbuffer *rb =
                    (struct ringbuffer *)ipc_mqueue_get_handle(
                        global_uart_rx_mqueue_list[d->dev_id]);

                irq_register(IRQ_ID_UART_RX(d->dev_id),    _uart_rx_cb,  rb);
                irq_register(IRQ_ID_UART_ERROR(d->dev_id), _uart_err_cb, h);
            }
        }
    }

    /* ── Main management loop ── */
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

/* ────────────────────────────────────────────────────────────────────────── */
/*                              Public API                                    */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Start UART management service.
 */
__SECTION_OS __USED
int32_t uart_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(UART_MGMT_QUEUE_DEPTH,
                               sizeof(uart_mgmt_msg_t));

    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(uart_mgmt_thread,
                                    "MGMT_UART",
                                    PROC_SERVICE_SERIAL_MGMT_STACK_SIZE,
                                    PROC_SERVICE_SERIAL_MGMT_PRIORITY,
                                    NULL);

    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief Get UART management queue handle.
 */
__SECTION_OS __USED
QueueHandle_t uart_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

/**
 * @brief Send UART transmit request (non-blocking).
 */
__SECTION_OS __USED
int32_t uart_mgmt_async_transmit(uint8_t dev_id,
                                 const uint8_t *data,
                                 uint16_t len)
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

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
           ? OS_ERR_NONE
           : OS_ERR_OP;
}

/**
 * @brief Read one byte from UART RX ring buffer.
 */
__SECTION_OS __USED
int32_t uart_mgmt_read_byte(uint8_t dev_id, uint8_t *byte)
{
    if (dev_id >= NO_OF_UART || byte == NULL)
        return OS_ERR_OP;

    struct ringbuffer *rb =
        (struct ringbuffer *)ipc_mqueue_get_handle(
            global_uart_rx_mqueue_list[dev_id]);

    if (rb == NULL)
        return OS_ERR_OP;

    return (ringbuffer_getchar(rb, byte) > 0U)
           ? OS_ERR_NONE
           : OS_ERR_OP;
}

#endif /* BOARD_UART_COUNT > 0 */