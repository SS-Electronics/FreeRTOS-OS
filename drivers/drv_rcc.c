/**
 * @file    drv_rcc.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Reset and Clock Control (RCC) driver with HAL abstraction
 *
 * @details
 * This file implements a portable RCC (Reset and Clock Control) driver
 * for the FreeRTOS-OS project. It provides a unified interface for
 * system clock configuration and peripheral clock control.
 *
 * The driver uses a HAL abstraction layer where vendor-specific
 * implementations populate a drv_rcc_hal_ops_t table at runtime via
 * a registration mechanism.
 *
 * Vendor selection is compile-time based on CONFIG_DEVICE_VARIANT.
 * Each supported platform provides a dedicated HAL implementation
 * (e.g., hal_rcc_stm32.c), which registers its operations through
 * a private shim function.
 *
 * The driver exposes APIs for:
 * - System clock initialization
 * - Retrieving clock frequencies (SYSCLK, APB1, APB2)
 * - Enabling peripheral clocks
 * - Enabling GPIO clocks
 *
 * @dependencies
 * stddef.h, drivers/drv_rcc.h, board/mcu_config.h, def_err.h
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
#include <def_std.h>
#include <def_err.h>
#include <drivers/drv_rcc.h>
#include <board/board_config.h>
#include <board/board_handles.h>


/* ── Forward declarations (vendor HAL shims) ──────────────────────────── */
/**
 * @brief Vendor-specific RCC registration functions
 *
 * @details
 * Each supported platform must expose a single registration entry point
 * that populates the HAL operations table.
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out);
void hal_rcc_stm32_periph_clk_en(drv_rcc_periph_t periph);
void hal_rcc_stm32_gpio_clk_en(void *port);
#endif

/* ── Singleton ops table ───────────────────────────────────────────────── */

/**
 * @brief RCC HAL operations table (singleton)
 *
 * @details
 * Populated during initialization via vendor-specific registration.
 */
__SECTION_OS_DATA __USED
static const drv_rcc_hal_ops_t *_ops;

/**
 * @brief Register RCC HAL operations
 *
 * @param ops HAL operations table
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (initialization phase)
 */
__SECTION_OS __USED
int32_t drv_rcc_register(const drv_rcc_hal_ops_t *ops)
{
    if (ops == NULL)
        return OS_ERR_OP;

    _ops = ops;
    return OS_ERR_NONE;
}

/* ── Driver API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialize system clocks
 *
 * @details
 * Calls the vendor-specific RCC registration shim and initializes
 * system clocks via the HAL layer.
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (early initialization)
 */
__SECTION_OS __USED
int32_t drv_rcc_clock_init(void)
{
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    _hal_rcc_stm32_register(&_ops);
#endif

    if (_ops == NULL || _ops->clock_init == NULL)
        return OS_ERR_OP;

    return _ops->clock_init();
}

/**
 * @brief Get system clock frequency
 *
 * @return System clock in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
__SECTION_OS __USED
uint32_t drv_rcc_get_sysclk_hz(void)
{
    if (_ops && _ops->get_sysclk_hz)
        return _ops->get_sysclk_hz();
    return 0U;
}

/**
 * @brief Get APB1 clock frequency
 *
 * @return APB1 clock in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
__SECTION_OS __USED
uint32_t drv_rcc_get_apb1_hz(void)
{
    if (_ops && _ops->get_apb1_hz)
        return _ops->get_apb1_hz();
    return 0U;
}

/**
 * @brief Get APB2 clock frequency
 *
 * @return APB2 clock in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
__SECTION_OS __USED
uint32_t drv_rcc_get_apb2_hz(void)
{
    if (_ops && _ops->get_apb2_hz)
        return _ops->get_apb2_hz();
    return 0U;
}

/**
 * @brief Enable peripheral clock
 *
 * @param periph Peripheral identifier
 *
 * @context Task (typically during initialization)
 */
__SECTION_OS __USED
void drv_rcc_periph_clk_en(drv_rcc_periph_t periph)
{
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    hal_rcc_stm32_periph_clk_en(periph);
#endif
}

/**
 * @brief Enable GPIO clock
 *
 * @param port GPIO port identifier (platform-specific)
 *
 * @context Task (typically during initialization)
 */
__SECTION_OS __USED
void drv_rcc_gpio_clk_en(void *port)
{
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    hal_rcc_stm32_gpio_clk_en(port);
#endif
}