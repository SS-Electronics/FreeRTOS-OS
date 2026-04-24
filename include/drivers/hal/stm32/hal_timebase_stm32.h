/**
 * @file    hal_timebase_stm32.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL timebase implementation using TIM1
 *
 * @details
 * This module provides an alternative HAL timebase implementation for STM32
 * microcontrollers using a hardware timer (TIM1) instead of the default SysTick.
 *
 * By overriding HAL_InitTick(), the system tick used by the STM32 HAL is driven
 * by TIM1, allowing SysTick to be exclusively used by FreeRTOS.
 *
 * The driver supports:
 * - HAL tick generation using TIM1 update interrupts
 * - Separation of RTOS tick (SysTick) and HAL tick source
 * - Interrupt-based timebase increment
 *
 * The TIM1 interrupt handler must call hal_timebase_stm32_irq_handler() to
 * properly increment the HAL tick counter.
 *
 * Typical usage:
 * @code
 * // Inside TIM1_UP_TIM10_IRQHandler
 * void TIM1_UP_TIM10_IRQHandler(void)
 * {
 *     hal_timebase_stm32_irq_handler();
 * }
 * @endcode
 *
 * @dependencies
 * def_attributes.h, def_compiler.h, def_std.h, def_err.h,
 * board/mcu_config.h, board/board_config.h, board/board_handles.h,
 * drivers/drv_time.h
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

#ifndef DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_
#define DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/mcu_config.h>
#include <board/board_config.h>
#include <board/board_handles.h>
#include <drivers/drv_time.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  STM32 HAL timebase IRQ handler (TIM1-based).
 *
 * This function must be called from the TIM1 update interrupt handler
 * (e.g. TIM1_UP_TIM10_IRQHandler). It advances the HAL tick counter
 * used internally by STM32 HAL.
 *
 * This implementation replaces the default SysTick-based HAL timebase,
 * allowing SysTick to be reserved for the RTOS scheduler (FreeRTOS).
 *
 * @note
 * Ensure TIM1 is properly configured to generate periodic update
 * interrupts at 1ms intervals (or desired HAL tick frequency).
 */
void hal_timebase_stm32_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_ */