/*
 * drv_gpio.c — Generic GPIO driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Vendor-agnostic GPIO driver.  All pin operations go through the
 * drv_gpio_hal_ops_t table bound at registration time.
 *
 * GPIO lines are registered individually by board-specific init code
 * (e.g. board_init() or stm32f4xx_hal_msp.c).  Each logical line gets a
 * unique dev_id assigned by the caller.
 */

#include <drivers/drv_handle.h>
#include <board/mcu_config.h>
#include <def_err.h>

/* Maximum logical GPIO lines managed by this driver */
#ifndef DRV_GPIO_MAX_LINES
#define DRV_GPIO_MAX_LINES   32
#endif

/* ── Handle storage ───────────────────────────────────────────────────── */

static drv_gpio_handle_t _gpio_handles[DRV_GPIO_MAX_LINES];
static uint8_t           _gpio_registered_count = 0;

/* ── Registration API ─────────────────────────────────────────────────── */

int32_t drv_gpio_register(uint8_t dev_id, const drv_gpio_hal_ops_t *ops)
{
    if (dev_id >= DRV_GPIO_MAX_LINES || ops == NULL)
        return OS_ERR_OP;

    drv_gpio_handle_t *h = &_gpio_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    if (_gpio_registered_count <= dev_id)
        _gpio_registered_count = (uint8_t)(dev_id + 1);

    return h->ops->hw_init(h);
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id)
{
    if (dev_id >= DRV_GPIO_MAX_LINES)
        return NULL;
    return &_gpio_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

int32_t drv_gpio_set_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    h->ops->set(h);
    return OS_ERR_NONE;
}

int32_t drv_gpio_clear_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    h->ops->clear(h);
    return OS_ERR_NONE;
}

int32_t drv_gpio_toggle_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    h->ops->toggle(h);
    return OS_ERR_NONE;
}

int32_t drv_gpio_write_pin(uint8_t dev_id, uint8_t state)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    h->ops->write(h, state);
    return OS_ERR_NONE;
}

uint8_t drv_gpio_read_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return 0;
    return h->ops->read(h);
}
