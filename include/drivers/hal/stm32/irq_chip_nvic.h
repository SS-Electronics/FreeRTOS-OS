/**
 * @file    irq_chip_nvic.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   ARM Cortex-M NVIC-backed irq_chip implementation
 *
 * @details
 * This module provides an implementation of the generic irq_chip interface
 * using the ARM Cortex-M NVIC (Nested Vectored Interrupt Controller).
 *
 * It enables the IRQ subsystem to map software-defined IRQ IDs to actual
 * hardware interrupt lines and control them through a uniform abstraction.
 *
 * The driver supports:
 * - Binding software IRQ IDs to NVIC IRQn lines
 * - Configuring interrupt priority
 * - Providing a shared irq_chip instance for all NVIC-backed interrupts
 *
 * Software-only IRQs (i.e., those not mapped to hardware lines) must use
 * a NULL irq_chip and typically rely on handle_simple_irq().
 *
 * Typical usage:
 * @code
 * irq_set_chip_and_handler(IRQ_ID_UART_RX(0),
 *                          irq_chip_nvic_get(),
 *                          handle_simple_irq);
 *
 * irq_chip_nvic_bind_hwirq(IRQ_ID_UART_RX(0), USART1_IRQn, 5);
 * @endcode
 *
 * @dependencies
 * irq/irq_desc.h
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

#ifndef INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_
#define INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_


#include <irq/irq_desc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the singleton NVIC irq_chip instance.
 *
 * Returns a pointer to the NVIC-backed irq_chip structure. This instance
 * is shared across all IRQ descriptors and does not store per-interrupt state.
 *
 * All hardware-specific information such as IRQ number and priority is stored
 * in the corresponding irq_data structure.
 *
 * @return Pointer to the global NVIC irq_chip instance.
 */
struct irq_chip *irq_chip_nvic_get(void);

/**
 * @brief Bind a software IRQ ID to a hardware NVIC interrupt line.
 *
 * Associates a logical IRQ identifier with a physical NVIC interrupt number
 * and configures its priority.
 *
 * This function must be called before enabling the interrupt via irq_enable(),
 * otherwise the irq_chip will not have a valid hardware IRQ number to operate on.
 *
 * @param[in] irq
 *     Software IRQ identifier used by the IRQ subsystem.
 *
 * @param[in] irqn
 *     Hardware NVIC interrupt number (IRQn_Type cast to int32_t).
 *
 * @param[in] priority
 *     NVIC preemption priority (0 = highest priority).
 */
void irq_chip_nvic_bind_hwirq(irq_id_t irq, int32_t irqn, uint32_t priority);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_ */