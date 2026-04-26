/**
 * @file    drv_time.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic system time and timer driver interface with HAL abstraction
 *
 * @details
 * This header defines the interface for system timing utilities and
 * per-instance timer drivers in the FreeRTOS-OS project.
 *
 * It provides:
 * - Global system tick access (g_ms_ticks)
 * - Systick-based timing utilities (get ticks, delay)
 * - Timer handle definition (drv_timer_handle_t)
 * - Vendor-specific hardware abstraction (drv_hw_timer_ctx_t)
 * - HAL operations table (drv_timer_hal_ops_t)
 * - Public APIs for timer device management
 *
 * The design follows a hardware abstraction model where all MCU-specific
 * implementations are encapsulated in a HAL layer selected via
 * CONFIG_DEVICE_VARIANT.
 *
 * Supported platforms include:
 * - STM32 (HAL TIM-based)
 * - Infineon (register-level abstraction)
 *
 * @dependencies
 * def_std.h, def_err.h, board/mcu_config.h
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

#ifndef OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_
#define OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */
#include <board/board_handles.h>

/* ── Forward declaration ───────────────────────────────────────────────── */
/**
 * @brief Timer driver handle forward declaration
 */
typedef struct drv_timer_handle drv_timer_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
/**
 * @brief Vendor-specific timer hardware context
 *
 * @details
 * Abstracts underlying timer peripheral depending on MCU platform.
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef HAL_TIM_MODULE_ENABLED
/**
 * @brief STM32 TIM hardware context
 */
typedef struct {
    TIM_HandleTypeDef htim;
} drv_hw_timer_ctx_t;
#else
/**
 * @brief Placeholder context when TIM HAL is disabled
 */
typedef struct {
    uint32_t _reserved;
} drv_hw_timer_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

/**
 * @brief Infineon timer hardware context
 */
typedef struct {
    uint32_t timer_base;
    uint32_t period;
} drv_hw_timer_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check board_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
/**
 * @brief Timer HAL operations table
 *
 * @details
 * Provides abstraction for hardware-specific timer operations.
 */
typedef struct drv_timer_hal_ops {

    /**
     * @brief Initialize timer hardware
     * @param h Timer handle
     * @return OS_ERR_NONE on success
     */
    int32_t (*hw_init)(drv_timer_handle_t *h);

    /**
     * @brief Deinitialize timer hardware
     * @param h Timer handle
     * @return OS_ERR_NONE on success
     */
    int32_t (*hw_deinit)(drv_timer_handle_t *h);

    /**
     * @brief Get current counter value
     * @param h Timer handle
     * @return Counter value
     */
    uint32_t (*get_counter)(drv_timer_handle_t *h);

    /**
     * @brief Set counter value
     * @param h Timer handle
     * @param val Counter value
     */
    void (*set_counter)(drv_timer_handle_t *h, uint32_t val);

} drv_timer_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
/**
 * @brief Timer driver handle structure
 *
 * @details
 * Stores runtime state, HAL bindings, and configuration for a timer instance.
 */
struct drv_timer_handle {
    uint8_t                    dev_id;       /**< Timer device index */
    uint8_t                    initialized; /**< Initialization state */
    drv_hw_timer_ctx_t         hw;           /**< Hardware context */
    const drv_timer_hal_ops_t *ops;          /**< HAL operations */
    int32_t                    last_err;     /**< Last error code */
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
/**
 * @brief Initialize timer handle with default values
 */
#define DRV_TIMER_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

/* ── Systick utilities (always available) ─────────────────────────────── */

/**
 * @brief Global system tick counter (milliseconds)
 *
 * @context ISR (updated), Task (read)
 */
extern volatile uint32_t g_ms_ticks;

/**
 * @brief Get system tick count
 *
 * @return Current tick count in milliseconds
 *
 * @context Task/ISR
 * @reentrant Yes
 */
uint32_t drv_time_get_ticks(void);

/**
 * @brief Blocking delay in milliseconds
 *
 * @param ms Delay duration
 *
 * @context Task only
 * @warning Busy-wait implementation
 */
void drv_time_delay_ms(uint32_t ms);

#if (NO_OF_TIMER > 0)

/**
 * @brief Register timer device
 *
 * @param dev_id Timer index
 * @param ops    HAL operations
 *
 * @return OS_ERR_NONE on success
 *
 * @context Task (initialization phase)
 */
int32_t drv_timer_register(uint8_t dev_id,
                           const drv_timer_hal_ops_t *ops);

/**
 * @brief Get timer handle
 *
 * @param dev_id Timer index
 * @return Pointer to handle or NULL
 *
 * @context Task/ISR
 */
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id);

/**
 * @brief Get timer counter value
 *
 * @param dev_id Timer index
 * @return Counter value (0 on error)
 *
 * @context Task/ISR
 */
uint32_t drv_timer_get_counter(uint8_t dev_id);

/**
 * @brief Set timer counter value
 *
 * @param dev_id Timer index
 * @param val    Counter value
 *
 * @context Task/ISR
 */
void drv_timer_set_counter(uint8_t dev_id, uint32_t val);

#endif /* NO_OF_TIMER > 0 */

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_ */