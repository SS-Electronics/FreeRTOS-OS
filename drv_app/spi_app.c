/*
 * spi_app.c — Application-level SPI sync / async operations
 *
 * This file is part of FreeRTOS-OS Project.
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
