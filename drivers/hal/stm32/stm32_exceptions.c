/**
 * @file    stm32_exceptions.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   Cortex-M4 core exception handlers for STM32
 *
 * @details
 * This file provides strong definitions for Cortex-M core exception handlers.
 * These override the weak symbols defined in the startup assembly file
 * (e.g., startup_stm32f411vetx.s).
 *
 * The handlers defined here are minimal by design:
 * - Fault handlers are typically extended to trap execution for debugging
 * - Non-critical handlers may remain empty unless explicitly required
 *
 * Peripheral interrupt handlers are implemented separately in:
 * @ref irq_periph_dispatch_generated.c
 *
 * @note
 * This module is compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM.
 *
 * -----------------------------------------------------------------------------
 * @section behavior Exception Handling Behavior
 *
 * - NMI (Non-Maskable Interrupt): Reserved for critical system events
 * - Debug Monitor: Used for debugging support (optional)
 *
 * These default implementations are intentionally minimal and can be
 * extended as needed depending on system requirements.
 *
 * -----------------------------------------------------------------------------
 * @section license License
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the :contentReference[oaicite:0]{index=0},
 * either version 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details:
 * https://www.gnu.org/licenses/
 *
 * You should have received a copy of the GNU General Public License
 * along with this project. If not, see the link above.
 * -----------------------------------------------------------------------------
 */

#include <board/mcu_config.h>



#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

/**
 * @brief Non-Maskable Interrupt (NMI) handler
 *
 * @details
 * Handles non-maskable interrupts which cannot be disabled.
 * Typically used for critical fault conditions such as clock failure.
 *
 * @note
 * Default implementation is empty. Override if system-level handling is required.
 */
void NMI_Handler(void)
{
    while(1)
    {

    }
}

/**
 * @brief Debug Monitor handler
 *
 * @details
 * Invoked when a debug event occurs (if enabled in the system).
 * Used primarily for advanced debugging features.
 *
 * @note
 * Default implementation is empty.
 */
void DebugMon_Handler(void)
{
    while(1)
    {
        
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */