/*
 * hal_timebase_stm32.h — STM32 HAL tick/timebase via TIM1
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * HAL_InitTick() overrides the default SysTick-based implementation and drives
 * the HAL tick counter from TIM1 instead, freeing SysTick for FreeRTOS.
 */

#ifndef DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_
#define DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#ifdef __cplusplus
extern "C" {
#endif

/** Called from TIM1_UP_TIM10_IRQHandler to advance the HAL tick. */
void hal_timebase_stm32_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_TIMEBASE_STM32_H_ */
