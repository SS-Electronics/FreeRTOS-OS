/*
 * gpio_app.c — Application-level GPIO sync / async operations
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <drv_app/gpio_app.h>
#include <drivers/drv_gpio.h>

/* ── Immediate (direct driver) ────────────────────────────────────────── */

uint8_t gpio_read(uint8_t gpio_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(gpio_id);
    if (h == NULL || h->ops == NULL || !h->initialized)
        return 0xFFU;
    return h->ops->read(h);
}

/* ── Asynchronous via management queue ───────────────────────────────── */

int32_t gpio_async_set(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_SET, 0, 0);
}

int32_t gpio_async_clear(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_CLEAR, 0, 0);
}

int32_t gpio_async_toggle(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_TOGGLE, 0, 0);
}

int32_t gpio_async_write(uint8_t gpio_id, uint8_t state)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_WRITE, state, 0);
}

int32_t gpio_async_delayed(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                            uint8_t state, uint32_t delay_ms)
{
    return gpio_mgmt_post(gpio_id, cmd, state, delay_ms);
}
