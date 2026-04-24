/**
 * @file    drv_irq.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Vendor-agnostic IRQ abstraction interface
 *
 * @details
 * This header defines the interrupt abstraction layer for the FreeRTOS-OS
 * project. It separates interrupt handling into two orthogonal concerns:
 *
 * 1. NVIC Control (vendor-specific)
 *    - Provides APIs to configure interrupt priorities and enable/disable IRQs.
 *    - Compiled only for the selected MCU family via CONFIG_DEVICE_VARIANT.
 *
 * 2. Software IRQ Dispatch (vendor-agnostic)
 *    - Provides a generic event-driven IRQ dispatch mechanism.
 *    - Decouples hardware ISRs from driver/application callbacks.
 *    - Uses an IRQ descriptor system (irq_desc) for callback registration.
 *
 * Architecture flow:
 * @code
 * Hardware ISR (HAL)
 *        ↓
 * drv_irq_dispatch_from_isr()
 *        ↓
 * irq_desc / __do_IRQ_from_isr()
 *        ↓
 * Registered callbacks (drivers / tasks)
 * @endcode
 *
 * HAL callbacks invoke drv_irq_dispatch_from_isr(), while drivers and
 * management layers register their handlers via drv_irq_register().
 * This ensures clean separation between hardware, driver, and application layers.
 *
 * @dependencies
 * board/mcu_config.h, irq/irq_notify.h
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

#ifndef INCLUDE_DRIVERS_DRV_IRQ_H_
#define INCLUDE_DRIVERS_DRV_IRQ_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h> 
#include <board/mcu_config.h>
#include <irq/irq_notify.h>   /* irq_id_t, irq_notify_cb_t, BaseType_t */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>   /* IRQn_Type */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ── CPU / system control ─────────────────────────────────────────────── */

/**
 * @brief Configure CPU interrupt priority settings
 *
 * @details
 * Typically sets system exception priorities (e.g., SVCall).
 *
 * @context Early initialization
 */
void drv_cpu_interrupt_prio_set(void);

/**
 * @brief Reset MCU
 *
 * @details
 * Triggers a system reset via NVIC or equivalent mechanism.
 *
 * @context Any (fatal error / system recovery)
 */
void reset_mcu(void);

/* ── NVIC control (vendor-specific) ───────────────────────────────────── */

/**
 * @brief Set priority and enable an interrupt line
 *
 * @param irqn              IRQ number (IRQn_Type cast to int32_t)
 * @param preempt_priority  Preemption priority (0 = highest)
 *
 * @context Task (initialization/configuration phase)
 */
void drv_irq_enable(int32_t irqn, uint32_t preempt_priority);

/**
 * @brief Set interrupt priority without enabling
 *
 * @details
 * Used for core exceptions (e.g., SVCall, PendSV) that do not require enabling.
 *
 * @param irqn              IRQ number
 * @param preempt_priority  Preemption priority
 *
 * @context Task
 */
void drv_irq_set_priority(int32_t irqn, uint32_t preempt_priority);

/**
 * @brief Disable an interrupt line
 *
 * @param irqn IRQ number
 *
 * @context Task
 */
void drv_irq_disable(int32_t irqn);

/* ── Software IRQ registration and dispatch ───────────────────────────── */

/**
 * @brief Register a callback for a software IRQ ID
 *
 * @param id   IRQ identifier
 * @param cb   Callback function
 * @param arg  User argument
 *
 * @return OS_ERR_NONE on success, negative error code on failure
 *
 * @context Task (initialization phase)
 */
int32_t drv_irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg);

/**
 * @brief Dispatch IRQ event in task context
 *
 * @param id    IRQ identifier
 * @param data  Event data
 *
 * @context Task
 */
void drv_irq_dispatch(irq_id_t id, void *data);

/**
 * @brief Dispatch IRQ event from ISR context
 *
 * @param id     IRQ identifier
 * @param data   Event data
 * @param pxHPT  Pointer to higher-priority-task-woken flag
 *
 * @details
 * Each registered callback can update @p pxHPT. The caller must invoke
 * portYIELD_FROM_ISR(*pxHPT) after this function returns.
 *
 * @context ISR
 */
void drv_irq_dispatch_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_IRQ_H_ */