/*
 * drv_iic.c — Generic I2C driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Vendor-agnostic I2C driver.  All bus operations are performed through the
 * drv_iic_hal_ops_t table bound at registration time by iic_mgmt.c.
 */

#include <drivers/drv_handle.h>
#include <config/mcu_config.h>
#include <config/os_config.h>
#include <def_err.h>

#if (NO_OF_IIC > 0)

/* ── Handle storage ───────────────────────────────────────────────────── */

static drv_iic_handle_t _iic_handles[NO_OF_IIC];

/* ── Registration API ─────────────────────────────────────────────────── */

int32_t drv_iic_register(uint8_t dev_id,
                          const drv_iic_hal_ops_t *ops,
                          uint32_t clock_hz,
                          uint32_t timeout_ms)
{
    if (dev_id >= NO_OF_IIC || ops == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->clock_hz    = clock_hz;
    h->timeout_ms  = timeout_ms;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    return h->ops->hw_init(h);
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

drv_iic_handle_t *drv_iic_get_handle(uint8_t dev_id)
{
    if (dev_id >= NO_OF_IIC)
        return NULL;
    return &_iic_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief  Write @p len bytes to a device register.
 *
 * @param  dev_id     I2C bus index.
 * @param  dev_addr   7-bit device address (not shifted).
 * @param  reg_addr   Register address.
 * @param  data       TX data buffer.
 * @param  len        Number of bytes.
 */
int32_t drv_iic_transmit(uint8_t dev_id, uint16_t dev_addr,
                          uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev_id >= NO_OF_IIC || data == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transmit(h, dev_addr, reg_addr, 1, data, len, h->timeout_ms);
}

/**
 * @brief  Read @p len bytes from a device register.
 */
int32_t drv_iic_receive(uint8_t dev_id, uint16_t dev_addr,
                         uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev_id >= NO_OF_IIC || data == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->receive(h, dev_addr, reg_addr, 1, data, len, h->timeout_ms);
}

/**
 * @brief  Probe a device — returns OS_ERR_NONE if it ACKs.
 */
int32_t drv_iic_device_ready(uint8_t dev_id, uint16_t dev_addr)
{
    if (dev_id >= NO_OF_IIC)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->is_device_ready(h, dev_addr, h->timeout_ms);
}

#endif /* NO_OF_IIC > 0 */
