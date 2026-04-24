/**
 * @file    drv_uart.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic UART driver with HAL abstraction and ring-buffered TX support
 *
 * @details
 * This file implements a vendor-agnostic UART driver layer. All hardware-specific
 * operations are abstracted via the drv_uart_hal_ops_t function pointer table,
 * which is bound during device registration.
 *
 * The driver supports:
 * - Blocking transmit and receive APIs
 * - Interrupt-driven transmit using IPC ring buffers
 * - Device registration and hardware initialization via HAL
 *
 * The UART management layer initializes devices at startup using
 * drv_uart_register(), after which application code interacts through
 * the public APIs provided in this module.
 *
 * @dependencies
 * def_attributes.h, conf_os.h, def_err.h,
 * ipc/ringbuffer.h, ipc/global_var.h,
 * drivers/com/drv_uart.h, board/mcu_config.h
 *
 * @note
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#include <def_attributes.h>
#include <def_std.h>
#include <conf_os.h>
#include <def_err.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>

#include <drivers/com/drv_uart.h>
#include <board/mcu_config.h>

#if (BOARD_UART_COUNT > 0)

/* ── Handle storage (owned by this module) ────────────────────────────── */

static drv_uart_handle_t _uart_handles[BOARD_UART_COUNT];

/* Ring-buffer pointer for interrupt-driven TX per UART */
static struct ringbuffer *_tx_rb[BOARD_UART_COUNT];

/* ── Registration API ─────────────────────────────────────────────────── */

/**
 * @brief  Register a UART device and trigger hardware initialisation.
 *
 * @details
 * Called by uart_mgmt.c at startup. Binds the vendor HAL operations table
 * and initializes the hardware via hw_init(). Also links the IPC TX ring
 * buffer and drains any pending buffered data.
 *
 * @param  dev_id      UART device index
 * @param  ops         HAL operations table
 * @param  baudrate    Communication baudrate
 * @param  timeout_ms  Blocking timeout in milliseconds
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP or HAL error on failure
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

    /* Initialise hardware */
    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    /* Drain buffered TX data (e.g. printk before init) */
    drv_uart_tx_kick(dev_id);

    return OS_ERR_NONE;
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

/**
 * @brief  Get UART handle by device ID
 *
 * @param  dev_id  UART device index
 * @return Pointer to UART handle or NULL if invalid
 */
drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_UART_COUNT)
        return NULL;
    return &_uart_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief  Blocking UART transmit
 *
 * @param  dev_id  Device index
 * @param  data    Pointer to TX buffer
 * @param  len     Number of bytes to send
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
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
 * @brief  Blocking UART receive
 *
 * @param  dev_id  Device index
 * @param  data    Pointer to RX buffer
 * @param  len     Number of bytes to receive
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
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
 * @brief  Get next byte for TX from ring buffer (ISR context)
 *
 * @param  dev_id  UART device index
 * @param  byte    Output byte pointer
 *
 * @return OS_ERR_NONE if byte available, OS_ERR_OP if empty/error
 */
int32_t drv_uart_tx_get_next_byte(uint8_t dev_id, uint8_t *byte)
{
    if (dev_id >= BOARD_UART_COUNT || byte == NULL || _tx_rb[dev_id] == NULL)
        return OS_ERR_OP;

    return (ringbuffer_getchar_from_isr(_tx_rb[dev_id], byte) > 0)
           ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief  Kick/start UART TX interrupt
 *
 * @details
 * Enables TXE interrupt to start draining the TX ring buffer.
 * Safe to call after writing data into the TX buffer.
 *
 * @param  dev_id  UART device index
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 */
int32_t drv_uart_tx_kick(uint8_t dev_id)
{
    drv_uart_handle_t *h = drv_uart_get_handle(dev_id);

    if (h == NULL || !h->initialized || h->ops == NULL || h->ops->tx_start == NULL)
        return OS_ERR_OP;

    h->ops->tx_start(h);

    return OS_ERR_NONE;
}

#endif /* BOARD_UART_COUNT > 0 */