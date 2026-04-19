/*
 * hal_it_stm32.h — STM32 peripheral interrupt handler declarations
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_STM32_HAL_IT_STM32_H_
#define DRIVERS_HAL_STM32_HAL_IT_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Cortex-M4 core exception handlers */
void NMI_Handler(void);
void DebugMon_Handler(void);

/* STM32F4xx peripheral interrupt handlers */
void TIM1_UP_TIM10_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_STM32_HAL_IT_STM32_H_ */
