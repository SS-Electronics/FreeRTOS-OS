/*
 * drv_time.c — Generic timer / systick driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Systick utilities (drv_time_get_ticks / drv_time_delay_ms) are always
 * available and call the vendor HAL directly — no handle needed.
 *
 * Per-instance timer support (encoder counters, PWM, etc.) follows the same
 * ops-table pattern as drv_uart.c.  The vendor HAL backend populates a
 * drv_timer_hal_ops_t and calls drv_timer_register() at startup.
 */

#include <drivers/drv_handle.h>
#include <config/mcu_config.h>
#include <def_err.h>

/* ── Systick-based utilities (always available) ───────────────────────── */

uint32_t drv_time_get_ticks(void)
{
    return HAL_GetTick();
}

void drv_time_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* ── Per-instance timer driver ────────────────────────────────────────── */

#if (NO_OF_TIMER > 0)

static drv_timer_handle_t _timer_handles[NO_OF_TIMER];

int32_t drv_timer_register(uint8_t dev_id, const drv_timer_hal_ops_t *ops)
{
    if (dev_id >= NO_OF_TIMER || ops == NULL)
        return OS_ERR_OP;

    drv_timer_handle_t *h = &_timer_handles[dev_id];
    h->dev_id      = dev_id;
    h->ops         = ops;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    h->initialized = 1;
    return OS_ERR_NONE;
}

drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id)
{
    if (dev_id >= NO_OF_TIMER)
        return NULL;
    return &_timer_handles[dev_id];
}

uint32_t drv_timer_get_counter(uint8_t dev_id)
{
    if (dev_id >= NO_OF_TIMER)
        return 0;

    drv_timer_handle_t *h = &_timer_handles[dev_id];
    if (!h->initialized || h->ops == NULL || h->ops->get_counter == NULL)
        return 0;

    return h->ops->get_counter(h);
}

void drv_timer_set_counter(uint8_t dev_id, uint32_t val)
{
    if (dev_id >= NO_OF_TIMER)
        return;

    drv_timer_handle_t *h = &_timer_handles[dev_id];
    if (!h->initialized || h->ops == NULL || h->ops->set_counter == NULL)
        return;

    h->ops->set_counter(h, val);
}

#endif /* NO_OF_TIMER > 0 */
