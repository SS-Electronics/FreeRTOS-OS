/**
 * @file    drv_time.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic system time and timer driver with HAL abstraction
 *
 * @details
 * This file provides system timing utilities and a generic timer driver
 * abstraction layer for the FreeRTOS-OS project.
 *
 * It includes:
 * - Global millisecond tick counter (g_ms_ticks)
 * - Systick-based utilities (tick retrieval and delay)
 * - Per-instance timer driver using HAL operations table
 *
 * The systick utilities are always available and directly use the global
 * tick counter without requiring a device handle.
 *
 * The per-instance timer driver follows the same design pattern as other
 * drivers (e.g., UART), where hardware-specific implementations are
 * abstracted via drv_timer_hal_ops_t and registered at startup.
 *
 * @dependencies
 * def_attributes.h, def_compiler.h, def_err.h, def_std.h,
 * board/mcu_config.h, drivers/timer/drv_time.h
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
#include <def_err.h>
#include <def_std.h>
#include <board/board_config.h>
#include <drivers/drv_time.h>

/* ── Global millisecond tick counter ──────────────────────────────────── */

/**
 * @brief Global millisecond tick counter
 *
 * @details
 * Incremented from SysTick ISR (or equivalent timer interrupt).
 * Provides a system-wide time base.
 *
 * @context ISR (writer), Task (reader)
 */
__SECTION_OS_DATA __USED
volatile uint32_t g_ms_ticks = 0;

/* ── Systick-based utilities (always available) ───────────────────────── */

/**
 * @brief Get system tick count in milliseconds
 *
 * @return Current tick count
 *
 * @context Task/ISR
 * @reentrant Yes
 */
__SECTION_OS __USED
uint32_t drv_time_get_ticks(void)
{
    return g_ms_ticks;
}

/**
 * @brief Blocking delay in milliseconds
 *
 * @details
 * Busy-wait loop based on system tick counter.
 * Not suitable for low-power or RTOS-friendly designs.
 *
 * @param ms Delay duration in milliseconds
 *
 * @context Task only
 * @warning Busy-wait; blocks CPU execution
 */
__SECTION_OS __USED
void drv_time_delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {}
}

/* ── Per-instance timer driver ────────────────────────────────────────── */

#if (NO_OF_TIMER > 0)

/* Timer handle storage */
__SECTION_OS_DATA __USED
static drv_timer_handle_t _timer_handles[NO_OF_TIMER];

/**
 * @brief Register timer device and initialize hardware
 *
 * @param dev_id Timer device index
 * @param ops    HAL operations table
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (initialization phase)
 */
__SECTION_OS __USED
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

/**
 * @brief Get timer handle by device ID
 *
 * @param dev_id Timer device index
 * @return Pointer to handle or NULL if invalid
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id)
{
    if (dev_id >= NO_OF_TIMER)
        return NULL;
    return &_timer_handles[dev_id];
}

/**
 * @brief Get timer counter value
 *
 * @param dev_id Timer device index
 * @return Counter value (0 on error)
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
uint32_t drv_timer_get_counter(uint8_t dev_id)
{
    if (dev_id >= NO_OF_TIMER)
        return 0;

    drv_timer_handle_t *h = &_timer_handles[dev_id];

    if (!h->initialized || h->ops == NULL || h->ops->get_counter == NULL)
        return 0;

    return h->ops->get_counter(h);
}

/**
 * @brief Set timer counter value
 *
 * @param dev_id Timer device index
 * @param val    Counter value to set
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
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