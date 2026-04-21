/**
 * @file irq_chip_nvic.c
 * @brief ARM Cortex-M NVIC irq_chip implementation
 *
 * This module provides an implementation of the generic IRQ chip interface
 * for ARM Cortex-M NVIC (Nested Vectored Interrupt Controller).
 *
 * It maps generic IRQ operations (enable, disable, mask, unmask, etc.)
 * to STM32 HAL NVIC APIs, allowing the upper IRQ subsystem to remain
 * platform-agnostic.
 *
 * @note Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 *
 * @details
 * The NVIC hardware has the following characteristics:
 * - Interrupt enable/disable is handled directly via NVIC registers
 * - No explicit ACK/EOI required (handled automatically or by peripheral)
 * - Interrupt type (edge/level) is fixed by hardware
 * - Priority-based preemption supported
 *
 * This implementation adapts those constraints into the generic irq_chip API.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/irq_chip_nvic.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Chip Operations                                                       */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Enable an interrupt line in NVIC
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_enable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_EnableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Disable an interrupt line in NVIC
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_disable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_DisableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Mask an interrupt
 *
 * @details
 * On Cortex-M, masking is equivalent to disabling the IRQ at NVIC level.
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_mask(struct irq_data *data)
{
    nvic_irq_disable(data);
}

/**
 * @brief Unmask an interrupt
 *
 * @details
 * Restores interrupt delivery by enabling the NVIC line.
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_unmask(struct irq_data *data)
{
    nvic_irq_enable(data);
}

/**
 * @brief Acknowledge an interrupt
 *
 * @details
 * NVIC does not require explicit acknowledge. Interrupt flags are cleared
 * in peripheral registers (typically inside HAL ISR handlers).
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_ack(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief End Of Interrupt (EOI)
 *
 * @details
 * Cortex-M NVIC automatically handles EOI on ISR exit. No action required.
 *
 * @param data Pointer to IRQ data structure
 */
static void nvic_irq_eoi(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief Set interrupt priority (affinity)
 *
 * @param data     Pointer to IRQ data structure
 * @param priority Priority level (lower value = higher priority)
 */
static void nvic_irq_set_affinity(struct irq_data *data, uint32_t priority)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_SetPriority((IRQn_Type)data->hwirq, priority, 0);
}

/**
 * @brief Set interrupt trigger type
 *
 * @details
 * NVIC does not support configurable trigger types (edge/level).
 * This is defined by the peripheral hardware.
 *
 * @param data      Pointer to IRQ data structure
 * @param flow_type Requested flow type (ignored)
 *
 * @return Always returns 0 (success)
 */
static int nvic_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
    (void)data;
    (void)flow_type;
    return 0;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* NVIC IRQ Chip Instance                                                    */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Static NVIC irq_chip instance
 *
 * This structure defines the NVIC-backed implementation of the generic
 * irq_chip interface.
 */
static struct irq_chip _nvic_chip = {
    .name             = "NVIC",
    .irq_enable       = nvic_irq_enable,
    .irq_disable      = nvic_irq_disable,
    .irq_ack          = nvic_irq_ack,
    .irq_mask         = nvic_irq_mask,
    .irq_mask_ack     = NULL,   /**< Mask + ACK not combined */
    .irq_unmask       = nvic_irq_unmask,
    .irq_eoi          = nvic_irq_eoi,
    .irq_retrigger    = NULL,
    .irq_set_type     = nvic_irq_set_type,
    .irq_set_affinity = nvic_irq_set_affinity,
    .flags            = 0,
};

/**
 * @brief Get NVIC irq_chip instance
 *
 * @return Pointer to NVIC irq_chip
 */
struct irq_chip *irq_chip_nvic_get(void)
{
    return &_nvic_chip;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Hardware IRQ Binding                                                      */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Bind a logical IRQ to a hardware NVIC interrupt line
 *
 * @param irq      Logical IRQ identifier
 * @param irqn     Hardware IRQ number (NVIC IRQn)
 * @param priority Interrupt priority
 *
 * @details
 * This function associates a generic IRQ descriptor with a specific
 * NVIC interrupt line and configures its priority.
 */
void irq_chip_nvic_bind_hwirq(irq_id_t irq, int32_t irqn, uint32_t priority)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc == NULL)
        return;

    desc->irq_data.hwirq = irqn;

    HAL_NVIC_SetPriority((IRQn_Type)irqn, priority, 0);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */