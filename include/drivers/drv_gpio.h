/*
File:        drv_gpio.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:      drivers
Info:        Generic GPIO driver interface with HAL abstraction              
Dependency:  def_std.h, def_err.h, board/mcu_config.h

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
 * @file    drv_gpio.h
 * @brief   Generic GPIO driver interface
 *
 * @details
 * This file defines the interface for a vendor-agnostic GPIO driver.
 * All hardware-specific functionality is abstracted through the
 * drv_gpio_hal_ops_t operations table.
 *
 * Design model:
 * - Each GPIO line is treated as an independent logical device
 * - Identified using dev_id
 * - Registered during board initialization
 *
 * Architecture:
 * Application → Driver API → HAL Ops → Vendor HAL → Hardware
 */

#ifndef INCLUDE_DRIVERS_DRV_GPIO_H_
#define INCLUDE_DRIVERS_DRV_GPIO_H_


#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <board/board_handles.h>
#include <board/board_config.h>

/* ── Forward declaration ───────────────────────────────────────────────── */
/**
 * @brief GPIO driver handle
 */
typedef struct drv_gpio_handle drv_gpio_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
/**
 * @brief Vendor-specific GPIO hardware context
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

/**
 * @brief STM32 GPIO context
 */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      mode;         /**< GPIO mode (input/output/alternate) */
    uint32_t      pull;         /**< Pull configuration */
    uint32_t      speed;        /**< Output speed */
    uint8_t       active_state; /**< Active logic level */
} drv_hw_gpio_ctx_t;

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

/**
 * @brief Infineon GPIO context
 */
typedef struct {
    uint32_t port_base;
    uint32_t pin;
    uint32_t mode;
    uint32_t pull;
    uint32_t speed;
    uint8_t  active_state;
} drv_hw_gpio_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check board_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
/**
 * @brief GPIO HAL operations
 *
 * @details
 * All low-level GPIO operations must be implemented by the vendor HAL.
 */
typedef struct drv_gpio_hal_ops {

    int32_t (*hw_init)  (drv_gpio_handle_t *h);
    int32_t (*hw_deinit)(drv_gpio_handle_t *h);

    void    (*set)      (drv_gpio_handle_t *h);
    void    (*clear)    (drv_gpio_handle_t *h);
    void    (*toggle)   (drv_gpio_handle_t *h);

    uint8_t (*read)     (drv_gpio_handle_t *h);
    void    (*write)    (drv_gpio_handle_t *h, uint8_t state);

} drv_gpio_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
/**
 * @brief GPIO driver handle
 */
struct drv_gpio_handle {
    uint8_t                   dev_id;
    uint8_t                   initialized;
    drv_hw_gpio_ctx_t         hw;
    const drv_gpio_hal_ops_t *ops;
    int32_t                   last_err;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
/**
 * @brief Initialize GPIO handle
 */
#define DRV_GPIO_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register a GPIO line
 *
 * @param dev_id Logical GPIO ID
 * @param ops    HAL operations table
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 */
int32_t drv_gpio_register(uint8_t dev_id,
                          const drv_gpio_hal_ops_t *ops);

/**
 * @brief Get GPIO handle
 *
 * @param dev_id Logical GPIO ID
 * @return Pointer to handle or NULL
 */
drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);

/* ── GPIO Operations ───────────────────────────────────────────────────── */

/**
 * @brief Set GPIO pin (logic high)
 */
int32_t drv_gpio_set_pin(uint8_t dev_id);

/**
 * @brief Clear GPIO pin (logic low)
 */
int32_t drv_gpio_clear_pin(uint8_t dev_id);

/**
 * @brief Toggle GPIO pin
 */
int32_t drv_gpio_toggle_pin(uint8_t dev_id);

/**
 * @brief Write GPIO pin state
 */
int32_t drv_gpio_write_pin(uint8_t dev_id, uint8_t state);

/**
 * @brief Read GPIO pin state
 */
uint8_t drv_gpio_read_pin(uint8_t dev_id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_GPIO_H_ */