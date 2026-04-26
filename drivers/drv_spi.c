/**
 * @file    drv_spi.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic SPI driver with HAL abstraction
 *
 * @details
 * This file implements a vendor-agnostic SPI driver layer for the
 * FreeRTOS-OS project. All hardware-specific operations are abstracted
 * via the drv_spi_hal_ops_t function pointer table, which is bound
 * during device registration.
 *
 * The driver supports:
 * - Blocking transmit and receive operations
 * - Full-duplex SPI transfer
 * - Device registration and hardware initialization via HAL
 *
 * Chip-select (CS) control is not handled by this driver and must be
 * managed by the caller (application or higher-level driver).
 *
 * The SPI management layer initializes devices at startup using
 * drv_spi_register(), after which application code interacts through
 * the public APIs provided in this module.
 *
 * @dependencies
 * drivers/com/drv_spi.h, board/mcu_config.h, def_err.h
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
#include <def_compiler.h>
#include <def_std.h> 
#include <def_err.h>
#include <board/board_handles.h>
#include <board/board_config.h>   /* pulls board_device_ids.h → BOARD_SPI_COUNT */
#include <drivers/drv_spi.h>

#if (BOARD_SPI_COUNT > 0)

/* ── Handle storage ───────────────────────────────────────────────────── */

/**
 * @brief SPI handle storage (owned by this module)
 */
__SECTION_OS_DATA __USED
static drv_spi_handle_t _spi_handles[BOARD_SPI_COUNT];

/* ── Registration API ─────────────────────────────────────────────────── */

/**
 * @brief Register SPI device and initialize hardware
 *
 * @details
 * Binds the HAL operations table and initializes the SPI peripheral.
 * Called by spi_mgmt layer during system startup.
 *
 * @param dev_id      SPI device index
 * @param ops         HAL operations table
 * @param clock_hz    SPI clock frequency
 * @param timeout_ms  Blocking timeout in milliseconds
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP or HAL error on failure
 *
 * @context Task (initialization phase)
 */
__SECTION_OS __USED
int32_t drv_spi_register(uint8_t dev_id,
                         const drv_spi_hal_ops_t *ops,
                         uint32_t clock_hz,
                         uint32_t timeout_ms)
{
    if (dev_id >= BOARD_SPI_COUNT || ops == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->clock_hz    = clock_hz;
    h->timeout_ms  = timeout_ms;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    return h->ops->hw_init(h);
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

/**
 * @brief Get SPI handle by device ID
 *
 * @param dev_id SPI device index
 * @return Pointer to SPI handle or NULL if invalid
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
drv_spi_handle_t *drv_spi_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_SPI_COUNT)
        return NULL;
    return &_spi_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief Blocking SPI transmit
 *
 * @param dev_id SPI device index
 * @param data   Pointer to TX buffer
 * @param len    Number of bytes to send
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task
 */
__SECTION_OS __USED
int32_t drv_spi_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transmit(h, data, len, h->timeout_ms);
}

/**
 * @brief Blocking SPI receive
 *
 * @param dev_id SPI device index
 * @param data   Pointer to RX buffer
 * @param len    Number of bytes to receive
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task
 */
__SECTION_OS __USED
int32_t drv_spi_receive(uint8_t dev_id, uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->receive(h, data, len, h->timeout_ms);
}

/**
 * @brief Blocking SPI full-duplex transfer
 *
 * @param dev_id SPI device index
 * @param tx     Pointer to TX buffer
 * @param rx     Pointer to RX buffer
 * @param len    Number of bytes to transfer
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task
 */
__SECTION_OS __USED
int32_t drv_spi_transfer(uint8_t dev_id,
                         const uint8_t *tx,
                         uint8_t *rx,
                         uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transfer(h, tx, rx, len, h->timeout_ms);
}

#endif /* BOARD_SPI_COUNT > 0 */