/**
 * @file        spi_app.c
 * @brief       spi_app.c — Application-level SPI sync / async operations
 * @ingroup     drv_app
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver App
 * @info        Application-facing helpers that wrap driver init for board-listed peripherals.
 * @dependency  Driver layer, board BSP
 *
 * @details
 * spi_app.c — Application-level SPI sync / async operations
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

#include <drv_app/spi_app.h>
#include <services/spi_mgmt.h>

/* ── Synchronous ──────────────────────────────────────────────────────── */

int32_t spi_sync_transmit(uint8_t bus_id, const uint8_t *data,
                           uint16_t len, uint32_t timeout_ms)
{
    return spi_mgmt_sync_transmit(bus_id, data, len, timeout_ms);
}

int32_t spi_sync_receive(uint8_t bus_id, uint8_t *data,
                          uint16_t len, uint32_t timeout_ms)
{
    return spi_mgmt_sync_receive(bus_id, data, len, timeout_ms);
}

int32_t spi_sync_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                           uint16_t len, uint32_t timeout_ms)
{
    return spi_mgmt_sync_transfer(bus_id, tx, rx, len, timeout_ms);
}

/* ── Asynchronous ────────────────────────────────────────────────────── */

int32_t spi_async_transmit(uint8_t bus_id, const uint8_t *data, uint16_t len)
{
    return spi_mgmt_async_transmit(bus_id, data, len);
}

int32_t spi_async_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                            uint16_t len)
{
    return spi_mgmt_async_transfer(bus_id, tx, rx, len);
}
