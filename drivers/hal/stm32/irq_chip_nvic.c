/*
 * irq_chip_nvic.c — ARM Cortex-M NVIC irq_chip implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Maps irq_chip ops onto ARM NVIC / HAL_NVIC calls so the generic irq_desc
 * layer can mask, unmask, enable, and prioritise hardware interrupt lines
 * without touching vendor HAL headers directly.
 *
 * Compiled only for the STM32 (MCU_VAR_STM) target.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/irq_chip_nvic.h>

/* ── Chip ops ─────────────────────────────────────────────────────────────── */

static void nvic_irq_enable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;
    HAL_NVIC_EnableIRQ((IRQn_Type)data->hwirq);
}

static void nvic_irq_disable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;
    HAL_NVIC_DisableIRQ((IRQn_Type)data->hwirq);
}

static void nvic_irq_mask(struct irq_data *data)
{
    /* On Cortex-M, masking == disabling the NVIC line */
    nvic_irq_disable(data);
}

static void nvic_irq_unmask(struct irq_data *data)
{
    nvic_irq_enable(data);
}

static void nvic_irq_ack(struct irq_data *data)
{
    /* NVIC uses a write-to-clear model via the peripheral SR registers.
       The ack is handled in the HAL ISR before dispatch; nothing to do here. */
    (void)data;
}

static void nvic_irq_eoi(struct irq_data *data)
{
    /* Cortex-M NVIC is auto-cleared on ISR entry. Nothing needed. */
    (void)data;
}

static void nvic_irq_set_affinity(struct irq_data *data, uint32_t priority)
{
    if (data == NULL || data->hwirq < 0)
        return;
    HAL_NVIC_SetPriority((IRQn_Type)data->hwirq, priority, 0);
}

static int nvic_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
    /* NVIC interrupt sense is fixed by peripheral hardware. Not configurable. */
    (void)data;
    (void)flow_type;
    return 0;
}

/* ── Singleton chip object ────────────────────────────────────────────────── */

static struct irq_chip _nvic_chip = {
    .name             = "NVIC",
    .irq_enable       = nvic_irq_enable,
    .irq_disable      = nvic_irq_disable,
    .irq_ack          = nvic_irq_ack,
    .irq_mask         = nvic_irq_mask,
    .irq_mask_ack     = NULL,   /* use separate mask + ack */
    .irq_unmask       = nvic_irq_unmask,
    .irq_eoi          = nvic_irq_eoi,
    .irq_retrigger    = NULL,
    .irq_set_type     = nvic_irq_set_type,
    .irq_set_affinity = nvic_irq_set_affinity,
    .flags            = 0,
};

struct irq_chip *irq_chip_nvic_get(void)
{
    return &_nvic_chip;
}

/* ── hwirq binding ────────────────────────────────────────────────────────── */

void irq_chip_nvic_bind_hwirq(irq_id_t irq, int32_t irqn, uint32_t priority)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc == NULL)
        return;

    desc->irq_data.hwirq = irqn;
    HAL_NVIC_SetPriority((IRQn_Type)irqn, priority, 0);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
