/**
 * @file        spi_app.h
 * @brief       spi_app.h — Application-level SPI API
 * @ingroup     drv_app
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver App
 * @info        Application-facing helpers that wrap driver init for board-listed peripherals.
 * @dependency  Driver layer, board BSP
 *
 * @details
 * spi_app.h — Application-level SPI API
 *
 * Thin wrappers over spi_mgmt that expose a clean sync / async interface.
 * Chip-select (CS) control is the caller's responsibility — assert CS before
 * calling and deassert it after the call returns (or after the async operation
 * has been scheduled).
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
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
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_APP_SPI_APP_H_
#define FREERTOS_OS_INCLUDE_DRV_APP_SPI_APP_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Synchronous (blocking) ───────────────────────────────────────────── */

/**
 * @brief  Blocking transmit-only. RX data clocked in is discarded.
 */
int32_t spi_sync_transmit(uint8_t bus_id, const uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);

/**
 * @brief  Blocking receive-only. TX line clocked with 0xFF dummy bytes.
 */
int32_t spi_sync_receive(uint8_t bus_id, uint8_t *data,
                          uint16_t len, uint32_t timeout_ms);

/**
 * @brief  Blocking full-duplex transfer (TX and RX simultaneously).
 *
 * @param  tx   TX buffer (NULL → dummy 0xFF bytes sent).
 * @param  rx   RX buffer (NULL → received bytes discarded).
 */
int32_t spi_sync_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                           uint16_t len, uint32_t timeout_ms);

/* ── Asynchronous (non-blocking) ─────────────────────────────────────── */

/**
 * @brief  Non-blocking transmit-only. Returns immediately after queuing.
 *
 * Data buffer must stay valid until the management thread drains the queue.
 */
int32_t spi_async_transmit(uint8_t bus_id, const uint8_t *data, uint16_t len);

/**
 * @brief  Non-blocking full-duplex transfer. Returns immediately after queuing.
 *
 * Both tx and rx buffers must remain valid until the management thread
 * processes the request. No completion notification is provided.
 */
int32_t spi_async_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                            uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_APP_SPI_APP_H_ */
