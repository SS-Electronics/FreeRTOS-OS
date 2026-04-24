/*
File:        drv_gpio.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:      drivers
Info:        Generic GPIO driver implementation with HAL abstraction              
Dependency:  drivers/drv_gpio.h, board/mcu_config.h, def_err.h

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file    drv_gpio.c
 * @brief   Generic GPIO driver with HAL abstraction
 *
 * @details
 * This file implements a vendor-agnostic GPIO driver for the FreeRTOS-OS
 * project. All hardware-specific operations are performed through the
 * drv_gpio_hal_ops_t interface.
 *
 * GPIO lines are treated as independent logical devices. Each line is
 * assigned a unique dev_id and registered by board-specific initialization
 * code (e.g., board_init()).
 *
 * The driver supports:
 * - Pin set / clear / toggle operations
 * - Direct write and read
 * - HAL-based hardware abstraction
 */
#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <drivers/drv_gpio.h>
#include <board/mcu_config.h>
#include <board/board_handles.h>

/* Maximum logical GPIO lines managed by this driver */
#ifndef DRV_GPIO_MAX_LINES
#define DRV_GPIO_MAX_LINES   32
#endif

/* ── Handle storage ───────────────────────────────────────────────────── */

/**
 * @brief GPIO handle storage
 */
__SECTION_OS_DATA __USED
static drv_gpio_handle_t _gpio_handles[DRV_GPIO_MAX_LINES];

/**
 * @brief Number of registered GPIO lines
 */
__SECTION_OS_DATA __USED
static uint8_t _gpio_registered_count = 0;

/* ── Registration API ─────────────────────────────────────────────────── */

/**
 * @brief Register a GPIO line
 *
 * @param dev_id GPIO logical ID
 * @param ops    HAL operations table
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (initialization phase)
 */
__SECTION_OS __USED
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

    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    /* Mark initialized after successful init */
    h->initialized = 1;

    return OS_ERR_NONE;
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

/**
 * @brief Get GPIO handle
 *
 * @param dev_id GPIO logical ID
 * @return Pointer to handle or NULL
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id)
{
    if (dev_id >= DRV_GPIO_MAX_LINES)
        return NULL;
    return &_gpio_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief Set GPIO pin (logic high)
 *
 * @param dev_id GPIO ID
 * @return OS_ERR_NONE on success
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
int32_t drv_gpio_set_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    h->ops->set(h);
    return OS_ERR_NONE;
}

/**
 * @brief Clear GPIO pin (logic low)
 *
 * @param dev_id GPIO ID
 * @return OS_ERR_NONE on success
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
int32_t drv_gpio_clear_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    h->ops->clear(h);
    return OS_ERR_NONE;
}

/**
 * @brief Toggle GPIO pin
 *
 * @param dev_id GPIO ID
 * @return OS_ERR_NONE on success
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
int32_t drv_gpio_toggle_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    h->ops->toggle(h);
    return OS_ERR_NONE;
}

/**
 * @brief Write GPIO pin state
 *
 * @param dev_id GPIO ID
 * @param state  0 = low, non-zero = high
 *
 * @return OS_ERR_NONE on success
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
int32_t drv_gpio_write_pin(uint8_t dev_id, uint8_t state)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    h->ops->write(h, state);
    return OS_ERR_NONE;
}

/**
 * @brief Read GPIO pin state
 *
 * @param dev_id GPIO ID
 * @return Pin state (0 or 1)
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
uint8_t drv_gpio_read_pin(uint8_t dev_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return 0;

    return h->ops->read(h);
}