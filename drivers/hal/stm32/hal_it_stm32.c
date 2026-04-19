/*
 * hal_it_stm32.c — STM32 peripheral interrupt handlers
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Each handler delegates immediately to the appropriate driver-layer
 * dispatcher so this file contains no business logic and no direct HAL
 * handle references.
 *
 * Adding a new peripheral ISR:
 *   1. Declare the handler in hal_it_stm32.h.
 *   2. Add the body here — one call to the relevant hal_xxx_irq_handler().
 *   3. Add the function prototype to the ops header if a new dispatcher
 *      is needed.
 */

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/hal_it_stm32.h>
#include <drivers/hal/stm32/hal_timebase_stm32.h>
#include <drivers/com/hal/stm32/hal_uart_stm32.h>

/* ── Cortex-M4 core exception handlers ───────────────────────────────── */

void NMI_Handler(void) {}

void DebugMon_Handler(void) {}

/* ── STM32F4xx peripheral interrupt handlers ──────────────────────────── */

void TIM1_UP_TIM10_IRQHandler(void)
{
    hal_timebase_stm32_irq_handler();
}

void USART1_IRQHandler(void)
{
    hal_uart_stm32_irq_handler(USART1);
}

void USART2_IRQHandler(void)
{
    hal_uart_stm32_irq_handler(USART2);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
