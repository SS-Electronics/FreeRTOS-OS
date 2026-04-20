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
#include <board/board_config.h>
#include <def_err.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>

#if (NO_OF_UART > 0)

/* ── Handle storage (owned by this module) ────────────────────────────── */

static drv_uart_handle_t _uart_handles[BOARD_UART_COUNT];

/* One staging byte per UART used by the interrupt-driven RX path */
static uint8_t _rx_stage_byte[BOARD_UART_COUNT];

/* Ring-buffer pointers linked to per-UART IPC queues */
static struct ringbuffer *_rx_rb[BOARD_UART_COUNT];

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

    /* Link the IPC ring buffer for interrupt-driven RX */
    _rx_rb[dev_id] = (struct ringbuffer *)
                     ipc_mqueue_get_handle(global_uart_rx_mqueue_list[dev_id]);

    /* Initialise hardware */
    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    /* Arm the interrupt-driven single-byte receive */
    return h->ops->start_rx_it(h, &_rx_stage_byte[dev_id]);
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

/* ── ISR callback dispatcher ──────────────────────────────────────────── */

/*
 * Called from HAL_UART_RxCpltCallback (vendor-specific ISR glue).
 * Dispatches to the ops rx_isr_cb of the matching handle.
 *
 * The vendor HAL layer (hal_uart_stm32.c) must call this from the STM32
 * HAL callback after identifying which handle received data.
 */
void drv_uart_rx_isr_dispatch(uint8_t dev_id, uint8_t rx_byte)
{
    if (dev_id >= BOARD_UART_COUNT)
        return;

    drv_uart_handle_t *h = &_uart_handles[dev_id];

    if (h->initialized && h->ops != NULL && h->ops->rx_isr_cb != NULL)
        h->ops->rx_isr_cb(h, rx_byte, _rx_rb[dev_id]);

    /* Dispatch to application callback registered via board_uart_register_rx_cb() */
    const board_uart_cbs_t *cbs = board_get_uart_cbs(dev_id);
    if (cbs != NULL && cbs->on_rx_byte != NULL)
        cbs->on_rx_byte(dev_id, rx_byte);
}

/* ── STM32 HAL callback (compiled only for STM32 targets) ─────────────── */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

/*
 * STM32 HAL calls this from the UART IRQ handler on successful reception of
 * one byte.  We identify which logical UART device owns the HAL handle and
 * dispatch to the generic ISR.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint8_t id = 0; id < BOARD_UART_COUNT; id++)
    {
        drv_uart_handle_t *h = &_uart_handles[id];
        if (h->initialized && &h->hw.huart == huart)
        {
            drv_uart_rx_isr_dispatch(id, _rx_stage_byte[id]);
            return;
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint8_t id = 0; id < BOARD_UART_COUNT; id++)
    {
        drv_uart_handle_t *h = &_uart_handles[id];
        if (h->initialized && &h->hw.huart == huart)
        {
            const board_uart_cbs_t *cbs = board_get_uart_cbs(id);
            if (cbs != NULL && cbs->on_tx_done != NULL)
                cbs->on_tx_done(id);
            return;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    for (uint8_t id = 0; id < BOARD_UART_COUNT; id++)
    {
        drv_uart_handle_t *h = &_uart_handles[id];
        if (h->initialized && &h->hw.huart == huart)
        {
            h->last_err = OS_ERR_OP;
            const board_uart_cbs_t *cbs = board_get_uart_cbs(id);
            if (cbs != NULL && cbs->on_error != NULL)
                cbs->on_error(id, huart->ErrorCode);
            return;
        }
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* NO_OF_UART > 0 */
