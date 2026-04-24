/**
 * @file    drv_irq.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   IRQ abstraction layer with NVIC control and software dispatch
 *
 * @details
 * This file implements a two-layer interrupt abstraction for the
 * FreeRTOS-OS project:
 *
 * 1. NVIC Control Layer (vendor-specific)
 *    - Provides APIs to configure interrupt priority, enable/disable IRQs,
 *      and perform system reset.
 *    - All vendor-specific calls (e.g., HAL_NVIC_*) are isolated here to
 *      maintain portability of upper layers.
 *
 * 2. Software IRQ Dispatch Layer (vendor-agnostic)
 *    - Implements a generic interrupt dispatch mechanism using an IRQ
 *      descriptor table.
 *    - Decouples hardware ISRs from driver/application callbacks.
 *    - Supports both task-context and ISR-context dispatch.
 *
 * The HAL layer invokes drv_irq_dispatch_from_isr() from ISR callbacks,
 * while drivers and management layers register handlers via drv_irq_register().
 *
 * @dependencies
 * drivers/drv_irq.h, irq/irq_desc.h
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

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h> 
#include <drivers/drv_irq.h>
#include <irq/irq_desc.h>

/* ── NVIC control (vendor-specific) ───────────────────────────────────── */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

/**
 * @brief Set CPU interrupt priority configuration
 *
 * @details
 * Configures system-level interrupt priorities (e.g., SVCall).
 *
 * @context Early initialization
 */
__SECTION_OS __USED
void drv_cpu_interrupt_prio_set(void)
{
#if (__ARM_ARCH_7A__ == 0U)
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif
}

/**
 * @brief Reset MCU
 *
 * @details
 * Triggers a system reset via NVIC.
 *
 * @context Any (typically fatal error handling)
 */
__SECTION_OS __USED
void reset_mcu(void)
{
    NVIC_SystemReset();
}

/**
 * @brief Enable IRQ with priority
 *
 * @param irqn               IRQ number
 * @param preempt_priority   Preemption priority
 *
 * @context Task (initialization/config phase)
 */
__SECTION_OS __USED
void drv_irq_enable(int32_t irqn, uint32_t preempt_priority)
{
    HAL_NVIC_SetPriority((IRQn_Type)irqn, preempt_priority, 0);
    HAL_NVIC_EnableIRQ((IRQn_Type)irqn);
}

/**
 * @brief Set IRQ priority
 *
 * @param irqn               IRQ number
 * @param preempt_priority   Preemption priority
 *
 * @context Task
 */
__SECTION_OS __USED
void drv_irq_set_priority(int32_t irqn, uint32_t preempt_priority)
{
    HAL_NVIC_SetPriority((IRQn_Type)irqn, preempt_priority, 0);
}

/**
 * @brief Disable IRQ
 *
 * @param irqn IRQ number
 *
 * @context Task
 */
__SECTION_OS __USED
void drv_irq_disable(int32_t irqn)
{
    HAL_NVIC_DisableIRQ((IRQn_Type)irqn);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

/* ── Software IRQ dispatch (vendor-agnostic) ─────────────────────────── */

/**
 * @brief Register IRQ handler
 *
 * @param id   IRQ identifier
 * @param cb   Callback function
 * @param arg  User argument
 *
 * @return OS_ERR_NONE on success
 *
 * @context Task
 */
__SECTION_OS __USED
int32_t drv_irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg)
{
    return irq_register(id, cb, arg);
}

/**
 * @brief Dispatch IRQ in task context
 *
 * @param id   IRQ identifier
 * @param data Event data
 *
 * @context Task
 */
__SECTION_OS __USED
void drv_irq_dispatch(irq_id_t id, void *data)
{
    __do_IRQ(id, data);
}

/**
 * @brief Dispatch IRQ from ISR context
 *
 * @param id     IRQ identifier
 * @param data   Event data
 * @param pxHPT  Pointer to higher-priority-task-woken flag
 *
 * @context ISR
 */
__SECTION_OS __USED
void drv_irq_dispatch_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT)
{
    __do_IRQ_from_isr(id, data, pxHPT);
}