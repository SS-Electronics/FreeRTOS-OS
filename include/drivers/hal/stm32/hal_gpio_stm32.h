/**
 * @file    hal_gpio_stm32.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL GPIO abstraction layer (ops table interface)
 *
 * @details
 * This header defines the STM32-specific GPIO HAL interface used by the
 * generic GPIO driver (drv_gpio). It exposes a function pointer table
 * (drv_gpio_hal_ops_t) that abstracts all hardware-dependent GPIO operations.
 *
 * The STM32 HAL backend binds its implementation through this interface,
 * allowing upper layers to remain fully vendor-agnostic.
 *
 * Key features:
 * - HAL ops table retrieval for driver registration
 * - Hardware context configuration helper for GPIO pins
 *
 * All direct STM32 HAL interactions are encapsulated within the HAL backend,
 * ensuring portability and clean separation between driver and hardware layers.
 *
 * @dependencies
 * board/mcu_config.h,
 * drivers/drv_gpio.h
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

/*
 * hal_gpio_stm32.h — STM32 HAL GPIO ops table declaration
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_STM32_HAL_GPIO_STM32_H_
#define DRIVERS_HAL_STM32_HAL_GPIO_STM32_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the STM32 GPIO HAL ops table. */
const drv_gpio_hal_ops_t *hal_gpio_stm32_get_ops(void);

/**
 * @brief  Populate the hardware context in a generic GPIO handle.
 *
 * @param  h            Generic GPIO handle.
 * @param  port         GPIO port (GPIOA, GPIOB, …).
 * @param  pin          GPIO pin mask (GPIO_PIN_0 … GPIO_PIN_15).
 * @param  mode         GPIO_MODE_OUTPUT_PP / GPIO_MODE_INPUT / …
 * @param  pull         GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN
 * @param  speed        GPIO_SPEED_FREQ_LOW … GPIO_SPEED_FREQ_VERY_HIGH
 * @param  active_state GPIO_PIN_SET if logic-high is "active", else GPIO_PIN_RESET
 */
void hal_gpio_stm32_set_config(drv_gpio_handle_t *h,
                                GPIO_TypeDef      *port,
                                uint16_t           pin,
                                uint32_t           mode,
                                uint32_t           pull,
                                uint32_t           speed,
                                uint8_t            active_state);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_GPIO_STM32_H_ */