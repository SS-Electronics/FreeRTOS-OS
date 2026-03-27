/*
 * uart_mgmt.c — UART management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Thread lifecycle
 * ────────────────
 *   1. uart_mgmt_start() creates the FreeRTOS task and queue.
 *   2. After TIME_OFFSET_SERIAL_MANAGEMENT ms the task registers every
 *      enabled UART device with the generic driver layer (drv_uart.c) by
 *      binding the correct vendor HAL ops table.
 *   3. The task then loops forever on the management queue, dispatching
 *      transmit / reinit / deinit commands.
 *   4. On error (h->last_err != OS_ERR_NONE) the task attempts a recovery
 *      reinit before returning an error to the caller.
 */

#include <services/uart_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_handle.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>

/* Vendor HAL selection — include only the active backend */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/com/hal/stm32/hal_uart_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <drivers/com/hal/infineon/hal_uart_infineon.h>
#endif

#if (INC_SERVICE_UART_MGMT == 1)

/* ── Private state ────────────────────────────────────────────────────── */

static QueueHandle_t _mgmt_queue = NULL;

/* ── Thread body ──────────────────────────────────────────────────────── */

static void uart_mgmt_thread(void *arg)
{
    (void)arg;

    /* ── Startup delay: wait for other subsystems to initialise ── */
    os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT);

    /* ── Register and initialise all enabled UART devices ── */
#if (NO_OF_UART > 0)
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
            for (uint8_t id = 0; id < NO_OF_UART; id++)
            {
                int32_t err = drv_uart_register(id,
                                                ops,
                                                UART_1_BAUD,    /* default baud */
                                                10);            /* 10 ms timeout */
                (void)err;
            }
        }
    }
#endif /* NO_OF_UART > 0 */

    /* ── Main event loop ── */
    uart_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        drv_uart_handle_t *h = drv_uart_get_handle(msg.dev_id);
        if (h == NULL)
            continue;

        switch (msg.cmd)
        {
            case UART_MGMT_CMD_TRANSMIT:
                if (msg.data != NULL && msg.len > 0)
                {
                    int32_t err = h->ops->transmit(h,
                                                   msg.data,
                                                   msg.len,
                                                   h->timeout_ms);
                    if (err != OS_ERR_NONE)
                    {
                        /* Attempt a single recovery reinit */
                        h->ops->hw_deinit(h);
                        h->ops->hw_init(h);
                    }
                }
                break;

            case UART_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h);
                h->ops->hw_init(h);
                break;

            case UART_MGMT_CMD_DEINIT:
                h->ops->hw_deinit(h);
                break;

            default:
                break;
        }
    }
}

/* ── Public API ───────────────────────────────────────────────────────── */

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
        .cmd    = UART_MGMT_CMD_TRANSMIT,
        .dev_id = dev_id,
        .data   = data,
        .len    = len,
    };

    BaseType_t ok = xQueueSend(_mgmt_queue, &msg, 0);
    return (ok == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* INC_SERVICE_UART_MGMT == 1 */
