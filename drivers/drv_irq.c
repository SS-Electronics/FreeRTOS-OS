/*
 * drv_irq.c — IRQ abstraction layer
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Two sections:
 *
 * 1. NVIC control — vendor-specific, compiled only for the active MCU.
 *    Isolates all HAL_NVIC_* calls so upper layers stay portable.
 *
 * 2. Software IRQ dispatch — vendor-agnostic, always compiled.
 *    Thin wrappers over irq_notify_from_isr() / irq_register().
 *    HAL callbacks call drv_irq_dispatch_from_isr(); drivers and management
 *    tasks call drv_irq_register().  Neither layer includes irq_notify.h.
 */

#include <drivers/drv_irq.h>
#include <irq/irq_desc.h>

/* ── NVIC control (STM32) ─────────────────────────────────────────────── */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

void drv_cpu_interrupt_prio_set(void)
{
#if (__ARM_ARCH_7A__ == 0U)
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif
}

void reset_mcu(void)
{
    NVIC_SystemReset();
}

void drv_irq_enable(int32_t irqn, uint32_t preempt_priority)
{
    HAL_NVIC_SetPriority((IRQn_Type)irqn, preempt_priority, 0);
    HAL_NVIC_EnableIRQ((IRQn_Type)irqn);
}

void drv_irq_set_priority(int32_t irqn, uint32_t preempt_priority)
{
    HAL_NVIC_SetPriority((IRQn_Type)irqn, preempt_priority, 0);
}

void drv_irq_disable(int32_t irqn)
{
    HAL_NVIC_DisableIRQ((IRQn_Type)irqn);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

/* ── Software IRQ dispatch (vendor-agnostic) ─────────────────────────── */

int32_t drv_irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg)
{
    return irq_register(id, cb, arg);
}

void drv_irq_dispatch(irq_id_t id, void *data)
{
    __do_IRQ(id, data);
}

void drv_irq_dispatch_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT)
{
    __do_IRQ_from_isr(id, data, pxHPT);
}
