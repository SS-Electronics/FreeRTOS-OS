/*
File:        drv_cpu.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Module:      drivers/cpu
Info:        CPU-level watchdog driver interface, hardware abstraction,
             and public API definitions              
Dependency:  def_std.h, def_err.h, mcu_config.h

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
 * @file drv_cpu.h
 * @brief CPU-level driver interface (Watchdog abstraction)
 *
 * Provides:
 *  - Watchdog (WDG) driver abstraction
 *  - HAL ops interface for vendor-specific implementations
 *  - Handle structure and lifecycle API
 *
 * Design:
 *  - Singleton watchdog instance
 *  - Vendor-specific HAL injected via ops table
 *  - Fully portable upper-layer API
 *
 * Notes:
 *  - CPU/NVIC utilities are implemented separately (drv_irq / HAL)
 *  - Fault handlers are vendor-specific and not exposed here
 */

#ifndef OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_
#define OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */
#include <board/board_config.h>
#include <board/board_handles.h>

/* ────────────────────────────────────────────────────────────────────── */
/* Forward declaration                                                    */
/* ────────────────────────────────────────────────────────────────────── */

/** @brief Watchdog handle forward declaration */
typedef struct drv_wdg_handle drv_wdg_handle_t;

/* ────────────────────────────────────────────────────────────────────── */
/* Vendor hardware context                                                */
/* ────────────────────────────────────────────────────────────────────── */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef HAL_IWDG_MODULE_ENABLED

/** @brief STM32 watchdog hardware context */
typedef struct {
    IWDG_HandleTypeDef hiwdg;
} drv_hw_wdg_ctx_t;

#else

typedef struct {
    uint32_t _reserved;
} drv_hw_wdg_ctx_t;

#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

/** @brief Infineon watchdog hardware context */
typedef struct {
    uint32_t wdg_base;
    uint32_t timeout_ms;
} drv_hw_wdg_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check board_config.h."
#endif

/* ────────────────────────────────────────────────────────────────────── */
/* HAL operations table                                                   */
/* ────────────────────────────────────────────────────────────────────── */

/**
 * @brief Watchdog HAL operations
 *
 * Implemented by vendor-specific backend.
 */
typedef struct drv_wdg_hal_ops {
    int32_t (*hw_init)  (drv_wdg_handle_t *h); /**< Initialize watchdog hardware */
    int32_t (*hw_deinit)(drv_wdg_handle_t *h); /**< Deinitialize watchdog       */
    void    (*refresh)  (drv_wdg_handle_t *h); /**< Refresh (kick) watchdog     */
} drv_wdg_hal_ops_t;

/* ────────────────────────────────────────────────────────────────────── */
/* Handle structure                                                      */
/* ────────────────────────────────────────────────────────────────────── */

/**
 * @brief Watchdog handle structure
 */
struct drv_wdg_handle {
    uint8_t                  initialized; /**< Initialization state */
    drv_hw_wdg_ctx_t         hw;          /**< Hardware context     */
    const drv_wdg_hal_ops_t *ops;         /**< HAL operations table */
    int32_t                  last_err;    /**< Last error code      */
};

/* ────────────────────────────────────────────────────────────────────── */
/* Handle initializer                                                    */
/* ────────────────────────────────────────────────────────────────────── */

#define DRV_WDG_HANDLE_INIT() \
    { .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ────────────────────────────────────────────────────────────────────── */
/* Public API                                                            */
/* ────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register watchdog HAL implementation
 *
 * @param ops Pointer to HAL operations table
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 */
int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops);

/**
 * @brief Get watchdog handle
 *
 * @return Pointer to watchdog handle (NULL if disabled)
 */
drv_wdg_handle_t *drv_wdg_get_handle(void);

/**
 * @brief Refresh (kick) watchdog
 *
 * Prevents watchdog reset. Safe to call periodically.
 */
void drv_wdg_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_ */