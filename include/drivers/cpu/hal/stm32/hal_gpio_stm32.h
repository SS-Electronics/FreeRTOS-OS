/*
 * hal_gpio_stm32.h — STM32 HAL GPIO ops table declaration
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_STM32_HAL_GPIO_STM32_H_
#define DRIVERS_HAL_STM32_HAL_GPIO_STM32_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>

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
