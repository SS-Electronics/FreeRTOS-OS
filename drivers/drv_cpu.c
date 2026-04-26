/*
File:        drv_cpu.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Module:      drivers/cpu
Info:        CPU-level utilities and watchdog driver (NVIC helpers, reset,
             fault integration, and watchdog abstraction)              
Dependency:  drv_cpu.h, mcu_config.h

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
 * @file drv_cpu.c
 * @brief CPU-level driver: watchdog abstraction and system-level utilities
 *
 * This module provides:
 *  - Watchdog (WDG) abstraction using ops-table pattern
 *  - CPU-level utilities (implemented elsewhere depending on MCU)
 *
 * Design:
 *  - Watchdog is implemented as a singleton driver
 *  - Vendor-specific implementation is injected via drv_wdg_hal_ops_t
 *  - Fault handlers are implemented in HAL layer
 *
 * Notes:
 *  - NVIC configuration and fault handlers are MCU-specific
 *  - This file remains vendor-agnostic
 */
#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <drivers/drv_cpu.h>
#include <board/board_config.h>
#include <board/board_config.h>
#include <board/board_handles.h>

/* ────────────────────────────────────────────────────────────────────── */
/* Watchdog driver                                                       */
/* ────────────────────────────────────────────────────────────────────── */

#if (CONFIG_MCU_WDG_EN == 1)

/** @brief Singleton watchdog handle */
__SECTION_OS_DATA __USED
static drv_wdg_handle_t _wdg_handle;

/**
 * @brief Register watchdog HAL implementation
 *
 * @param ops Pointer to HAL ops table
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 */
__SECTION_OS __USED
int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops)
{
    if (ops == NULL)
        return OS_ERR_OP;

    _wdg_handle.ops         = ops;
    _wdg_handle.initialized = 0;
    _wdg_handle.last_err    = OS_ERR_NONE;

    int32_t err = _wdg_handle.ops->hw_init(&_wdg_handle);
    if (err != OS_ERR_NONE)
        return err;

    _wdg_handle.initialized = 1;
    return OS_ERR_NONE;
}

/**
 * @brief Get watchdog handle
 *
 * @return Pointer to watchdog handle
 */
__SECTION_OS __USED
drv_wdg_handle_t *drv_wdg_get_handle(void)
{
    return &_wdg_handle;
}

/**
 * @brief Refresh (kick) the watchdog timer
 *
 * Safe to call periodically to prevent system reset.
 * Does nothing if watchdog is not initialized.
 */
__SECTION_OS __USED
void drv_wdg_refresh(void)
{
    if (!_wdg_handle.initialized || _wdg_handle.ops == NULL)
        return;

    _wdg_handle.ops->refresh(&_wdg_handle);
}

#else  /* CONFIG_MCU_WDG_EN == 0 */

/**
 * @brief Stub implementation when watchdog is disabled
 */
__SECTION_OS __USED
int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops)
{
    (void)ops;
    return OS_ERR_NONE;
}

/**
 * @brief Stub handle getter
 */
__SECTION_OS __USED
drv_wdg_handle_t *drv_wdg_get_handle(void)
{
    return NULL;
}

/**
 * @brief Stub refresh function
 */
__SECTION_OS __USED
void drv_wdg_refresh(void)
{
    /* Watchdog disabled */
}

#endif /* CONFIG_MCU_WDG_EN */

/* ────────────────────────────────────────────────────────────────────── */
/* Notes                                                                 */
/* ────────────────────────────────────────────────────────────────────── */

/*
 * Fault handlers (HardFault, MemManage, BusFault, UsageFault)
 * are implemented in:
 *
 *   drivers/hal/stm32/stm32_exceptions.c
 *
 * This keeps CPU abstraction clean and vendor-independent.
 */