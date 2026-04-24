/*
 * drv_spi.c — Generic SPI driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Vendor-agnostic SPI driver.  All bus operations go through the
 * drv_spi_hal_ops_t table bound at registration time by spi_mgmt.c.
 * Chip-select control is the caller's responsibility.
 */

#include <drivers/com/drv_spi.h>
#include <board/mcu_config.h>   /* pulls board_device_ids.h → BOARD_SPI_COUNT */
#include <def_err.h>

#if (BOARD_SPI_COUNT > 0)

/* ── Handle storage ───────────────────────────────────────────────────── */

static drv_spi_handle_t _spi_handles[BOARD_SPI_COUNT];

/* ── Registration API ─────────────────────────────────────────────────── */

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

drv_spi_handle_t *drv_spi_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_SPI_COUNT)
        return NULL;
    return &_spi_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

int32_t drv_spi_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transmit(h, data, len, h->timeout_ms);
}

int32_t drv_spi_receive(uint8_t dev_id, uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->receive(h, data, len, h->timeout_ms);
}

int32_t drv_spi_transfer(uint8_t dev_id,
                          const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (dev_id >= BOARD_SPI_COUNT || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    drv_spi_handle_t *h = &_spi_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transfer(h, tx, rx, len, h->timeout_ms);
}

#endif /* BOARD_SPI_COUNT > 0 */
