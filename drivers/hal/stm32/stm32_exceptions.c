/*
 * stm32_exceptions.c — Cortex-M4 core exception handlers (STM32)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * These strong definitions override the weak defaults in the startup assembly
 * file (startup_stm32f411vetx.s).  Fault handlers spin in a tight loop so a
 * debugger can break in and inspect the stack frame.
 *
 * Peripheral IRQ handlers are generated into app/board/irq_periph_dispatch_generated.c.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

void NMI_Handler(void) {}

void DebugMon_Handler(void) {}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
