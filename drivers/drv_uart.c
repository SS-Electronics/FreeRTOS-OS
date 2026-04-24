/*
 * drv_uart.c — Generic UART driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * This layer is vendor-agnostic.  All hardware access is performed through
 * the drv_uart_hal_ops_t function-pointer table bound at registration time.
 * See drivers/hal/stm32/ or drivers/hal/infineon/ for the actual HAL code.
 *
 * The management thread (drivers/mgmt/uart_mgmt.c) calls drv_uart_register()
 * at startup, then application code uses the public API below.
 */

#include <drivers/drv_handle.h>
#include <board/mcu_config.h>
#include <conf_os.h>
#include <def_err.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>

#if (BOARD_UART_COUNT > 0)

/* ── Handle storage (owned by this module) ────────────────────────────── */

static drv_uart_handle_t _uart_handles[BOARD_UART_COUNT];

/* Ring-buffer pointer for interrupt-driven TX per UART */
static struct ringbuffer *_tx_rb[BOARD_UART_COUNT];

/* ── Registration API ─────────────────────────────────────────────────── */

/**
 * @brief  Register a UART device and trigger hardware initialisation.
 *
 * Called by uart_mgmt.c at startup.  Binds the vendor ops table and calls
 * hw_init() so the peripheral is ready for use.
 */
int32_t drv_uart_register(uint8_t dev_id,
                           const drv_uart_hal_ops_t *ops,
                           uint32_t baudrate,
                           uint32_t timeout_ms)
{
    if (dev_id >= BOARD_UART_COUNT || ops == NULL)
        return OS_ERR_OP;

    drv_uart_handle_t *h = &_uart_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->baudrate    = baudrate;
    h->timeout_ms  = timeout_ms;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    /* Link IPC ring buffer for interrupt-driven TX */
    _tx_rb[dev_id] = (struct ringbuffer *)
                     ipc_mqueue_get_handle(global_uart_tx_mqueue_list[dev_id]);

    /* Initialise hardware — RXNEIE is enabled inside hw_init */
    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    /* Drain any bytes that were written to the TX ring buffer (e.g. by
     * printk) before the UART was initialised. */
    drv_uart_tx_kick(dev_id);
    return OS_ERR_NONE;
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_UART_COUNT)
        return NULL;
    return &_uart_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief  Blocking UART transmit.
 *
 * @param  dev_id   Device index (0 … NO_OF_UART-1).
 * @param  data     Pointer to TX buffer.
 * @param  len      Number of bytes to send.
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure.
 */
int32_t drv_serial_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_UART_COUNT || data == NULL || len == 0)
        return OS_ERR_OP;

    drv_uart_handle_t *h = &_uart_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transmit(h, data, len, h->timeout_ms);
}

/**
 * @brief  Blocking UART receive.
 *
 * @param  dev_id   Device index.
 * @param  data     Pointer to RX buffer.
 * @param  len      Number of bytes to receive.
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure.
 */
int32_t drv_serial_receive(uint8_t dev_id, uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_UART_COUNT || data == NULL || len == 0)
        return OS_ERR_OP;

    drv_uart_handle_t *h = &_uart_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->receive(h, data, len, h->timeout_ms);
}

/* ── Interrupt-driven TX helpers ──────────────────────────────────────── */

/**
 * drv_uart_tx_get_next_byte — pull the next pending TX byte.
 *
 * Called from the UART TXE ISR.  Returns OS_ERR_NONE and fills *byte if a
 * byte was available; returns OS_ERR_OP and leaves *byte unchanged if the
 * TX ring buffer is empty.
 */
int32_t drv_uart_tx_get_next_byte(uint8_t dev_id, uint8_t *byte)
{
    if (dev_id >= BOARD_UART_COUNT || byte == NULL || _tx_rb[dev_id] == NULL)
        return OS_ERR_OP;
    return (ringbuffer_getchar_from_isr(_tx_rb[dev_id], byte) > 0) ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * drv_uart_tx_kick — enable the UART TXE interrupt to start draining the
 *                    TX ring buffer.
 *
 * Call after writing one or more bytes to the TX ring buffer (e.g. printk).
 * If the UART is not yet initialised the call is silently ignored; the bytes
 * will be sent when drv_uart_register() calls this function at the end of
 * hardware init.
 */
int32_t drv_uart_tx_kick(uint8_t dev_id)
{
    drv_uart_handle_t *h = drv_uart_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL || h->ops->tx_start == NULL)
        return OS_ERR_OP;
    h->ops->tx_start(h);
    return OS_ERR_NONE;
}

#endif /* NO_OF_UART > 0 */
