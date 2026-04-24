/*
 * drv_irq.h — Vendor-agnostic IRQ abstraction
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Two orthogonal concerns live here:
 *
 * 1. NVIC control (vendor-specific, compiled only for the active MCU family):
 *    drv_irq_enable / drv_irq_disable / drv_irq_set_priority
 *
 * 2. Software IRQ registration and dispatch (vendor-agnostic):
 *    drv_irq_register / drv_irq_dispatch / drv_irq_dispatch_from_isr
 *
 *    HAL callbacks call drv_irq_dispatch_from_isr() with the IRQ ID.
 *    Drivers and management tasks call drv_irq_register() at startup to
 *    subscribe to events.  Neither layer touches irq_notify.h directly.
 */

#ifndef INCLUDE_DRIVERS_DRV_IRQ_H_
#define INCLUDE_DRIVERS_DRV_IRQ_H_

#include <board/mcu_config.h>
#include <irq/irq_notify.h>   /* irq_id_t, irq_notify_cb_t, BaseType_t */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>   /* IRQn_Type */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ── CPU / system control ────────────────────────────────────────────────── */

void drv_cpu_interrupt_prio_set(void);
void reset_mcu(void);

/* ── NVIC control (vendor-specific) ─────────────────────────────────────── */

/**
 * @brief  Set priority and enable an interrupt line.
 * @param  irqn              IRQ number cast to int32_t (IRQn_Type on Cortex-M).
 * @param  preempt_priority  NVIC preemption priority (0 = highest).
 */
void drv_irq_enable(int32_t irqn, uint32_t preempt_priority);

/**
 * @brief  Set only the priority of an interrupt without enabling it.
 *         Used for Cortex-M core exceptions (SVCall, PendSV) that are always
 *         present and do not need to be explicitly enabled.
 */
void drv_irq_set_priority(int32_t irqn, uint32_t preempt_priority);

/**
 * @brief  Disable an interrupt line.
 */
void drv_irq_disable(int32_t irqn);

/* ── Software IRQ registration and dispatch (vendor-agnostic) ───────────── */

/**
 * @brief  Register a callback for a software IRQ ID.
 *         Thin wrapper over irq_register() — upper layers include only
 *         this header and never touch irq_notify.h directly.
 * @return 0 on success, negative on error (subscriber table full, bad id).
 */
int32_t drv_irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg);

/**
 * @brief  Dispatch an IRQ event from task context.
 *         Calls every callback registered for @p id.
 */
void drv_irq_dispatch(irq_id_t id, void *data);

/**
 * @brief  Dispatch an IRQ event from ISR context.
 *         Each registered callback receives @p pxHPT so it can accumulate
 *         the FreeRTOS yield flag.  Caller must call
 *         portYIELD_FROM_ISR(*pxHPT) after this returns.
 */
void drv_irq_dispatch_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_IRQ_H_ */
