/**
 * @file    drv_rcc.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Reset and Clock Control (RCC) driver public interface
 *
 * @details
 * This header defines the portable interface for the clock initialization
 * and control subsystem in the FreeRTOS-OS project.
 *
 * The driver abstracts all vendor-specific RCC implementations behind a
 * HAL operations table (drv_rcc_hal_ops_t). The appropriate backend is
 * selected at compile time via CONFIG_DEVICE_VARIANT and registered
 * internally during drv_rcc_clock_init().
 *
 * Key features:
 * - System clock initialization (PLL, flash latency, bus dividers)
 * - Runtime clock frequency queries (SYSCLK, APB1, APB2)
 * - Peripheral and GPIO clock enable APIs
 *
 * The implementation ensures that application code remains vendor-agnostic
 * and does not depend on MCU-specific headers.
 *
 * Typical call sequence (from main, before scheduler start):
 * @code
 * HAL_Init();
 * drv_rcc_clock_init();
 * @endcode
 *
 * @dependencies
 * stdint.h, def_err.h
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

#ifndef OS_DRIVERS_DRV_RCC_H_
#define OS_DRIVERS_DRV_RCC_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_handles.h>
#include <board/mcu_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Peripheral IDs ───────────────────────────────────────────────────── */
/**
 * @brief RCC peripheral identifiers
 *
 * @details
 * Used with drv_rcc_periph_clk_en() to enable peripheral clocks
 * in a platform-independent manner.
 */
typedef enum {
    DRV_RCC_PERIPH_USART1 = 0,
    DRV_RCC_PERIPH_USART2,
    DRV_RCC_PERIPH_I2C1,
    DRV_RCC_PERIPH_SPI1,
    DRV_RCC_PERIPH_TIM1,
    DRV_RCC_PERIPH_SYSCFG,
    DRV_RCC_PERIPH_PWR,
    DRV_RCC_PERIPH_GPIOA,
    DRV_RCC_PERIPH_GPIOB,
    DRV_RCC_PERIPH_GPIOC,
} drv_rcc_periph_t;

/* ── HAL ops table ────────────────────────────────────────────────────── */
/**
 * @brief RCC HAL operations table
 *
 * @details
 * Defines the interface that must be implemented by each
 * vendor-specific RCC backend.
 */
typedef struct {

    /**
     * @brief Initialize system clocks
     *
     * @details
     * Configures PLL, flash latency, and bus dividers.
     *
     * @return OS_ERR_NONE on success
     */
    int32_t (*clock_init)(void);

    /**
     * @brief Get system clock frequency
     *
     * @return SYSCLK frequency in Hz
     */
    uint32_t (*get_sysclk_hz)(void);

    /**
     * @brief Get APB1 clock frequency
     *
     * @return APB1 clock frequency in Hz
     */
    uint32_t (*get_apb1_hz)(void);

    /**
     * @brief Get APB2 clock frequency
     *
     * @return APB2 clock frequency in Hz
     */
    uint32_t (*get_apb2_hz)(void);

} drv_rcc_hal_ops_t;

/* ── Driver API ───────────────────────────────────────────────────────── */

/**
 * @brief Register RCC HAL operations
 *
 * @param ops HAL operations table
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (initialization phase)
 */
int32_t drv_rcc_register(const drv_rcc_hal_ops_t *ops);

/**
 * @brief Initialize system clocks
 *
 * @details
 * Selects the appropriate vendor backend, registers the HAL ops,
 * and performs clock configuration.
 *
 * Must be called once after HAL_Init() and before scheduler start.
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (early initialization)
 */
int32_t drv_rcc_clock_init(void);

/**
 * @brief Get system clock frequency
 *
 * @return SYSCLK frequency in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
uint32_t drv_rcc_get_sysclk_hz(void);

/**
 * @brief Get APB1 clock frequency
 *
 * @return APB1 frequency in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
uint32_t drv_rcc_get_apb1_hz(void);

/**
 * @brief Get APB2 clock frequency
 *
 * @return APB2 frequency in Hz, or 0 on failure
 *
 * @context Task/ISR
 * @reentrant Yes
 */
uint32_t drv_rcc_get_apb2_hz(void);

/**
 * @brief Enable peripheral clock
 *
 * @param periph Peripheral identifier
 *
 * @context Task (initialization phase)
 */
void drv_rcc_periph_clk_en(drv_rcc_periph_t periph);

/**
 * @brief Enable GPIO clock
 *
 * @param port GPIO port (opaque pointer to avoid vendor dependency)
 *
 * @context Task (initialization phase)
 */
void drv_rcc_gpio_clk_en(void *port);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DRV_RCC_H_ */