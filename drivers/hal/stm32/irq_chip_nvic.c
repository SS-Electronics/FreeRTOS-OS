/**
 * @file    irq_chip_nvic.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   NVIC-backed irq_chip implementation for ARM Cortex-M (STM32)
 *
 * @ingroup irq_chip
 *
 * @details
 * This module implements the generic IRQ controller abstraction
 * (`struct irq_chip`) using the ARM Cortex-M NVIC hardware.
 *
 * It provides a translation layer between:
 * - Generic IRQ subsystem (irq_desc / irq_data)
 * - STM32 HAL NVIC APIs
 *
 * This allows upper layers to remain platform-independent while
 * delegating interrupt control to the NVIC.
 *
 * -----------------------------------------------------------------------------
 * @section irq_flow IRQ Execution Flow
 *
 * Typical interrupt handling path:
 *
 * @code
 * Vector Table → ISR (startup.s)
 *     → HAL IRQ Handler
 *         → Generic IRQ Dispatcher
 *             → irq_chip callbacks (this file)
 * @endcode
 *
 * -----------------------------------------------------------------------------
 * @section nvic_constraints NVIC Hardware Constraints
 *
 * - No explicit ACK mechanism
 * - No End-Of-Interrupt (EOI) signaling required
 * - Interrupt trigger type is fixed (peripheral-defined)
 * - Masking == disabling at NVIC level
 * - Priority-based preemption supported
 *
 * -----------------------------------------------------------------------------
 * @note
 * Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 */

/* ────────────────────────────────────────────────────────────────────────── */

#include <board/mcu_config.h>



#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/irq_chip_nvic.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Chip Operations                                                       */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Enable IRQ line
 *
 * @param data IRQ descriptor data (must be valid)
 *
 * @note
 * No effect if @p data is NULL or hwirq is invalid.
 */
static void nvic_irq_enable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_EnableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Disable IRQ line
 *
 * @param data IRQ descriptor data
 */
static void nvic_irq_disable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_DisableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Mask IRQ (disable delivery)
 *
 * @details
 * On Cortex-M, masking is implemented by disabling the NVIC line.
 *
 * @param data IRQ descriptor data
 */
static void nvic_irq_mask(struct irq_data *data)
{
    nvic_irq_disable(data);
}

/**
 * @brief Unmask IRQ (enable delivery)
 *
 * @param data IRQ descriptor data
 */
static void nvic_irq_unmask(struct irq_data *data)
{
    nvic_irq_enable(data);
}

/**
 * @brief Acknowledge IRQ
 *
 * @details
 * NVIC does not require explicit acknowledgment. Interrupt flags
 * must be cleared at the peripheral level before returning from ISR.
 *
 * @param data IRQ descriptor data
 */
static void nvic_irq_ack(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief End Of Interrupt (EOI)
 *
 * @details
 * Cortex-M automatically completes interrupt handling on ISR exit.
 *
 * @param data IRQ descriptor data
 */
static void nvic_irq_eoi(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief Set IRQ priority
 *
 * @param data     IRQ descriptor data
 * @param priority Preemption priority (implementation-defined range)
 *
 * @warning
 * Priority encoding depends on NVIC configuration (PRIGROUP).
 */
static void nvic_irq_set_affinity(struct irq_data *data, uint32_t priority)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_SetPriority((IRQn_Type)data->hwirq, priority, 0);
}

/**
 * @brief Configure IRQ trigger type
 *
 * @details
 * Not supported on NVIC. Trigger behavior is fixed by hardware.
 *
 * @param data      IRQ descriptor data
 * @param flow_type Requested trigger type (ignored)
 *
 * @retval 0 Always succeeds
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
 * @brief NVIC irq_chip singleton
 *
 * Provides the concrete implementation of the generic irq_chip interface.
 */
static struct irq_chip _nvic_chip = {
    .name             = "NVIC",
    .irq_enable       = nvic_irq_enable,
    .irq_disable      = nvic_irq_disable,
    .irq_ack          = nvic_irq_ack,
    .irq_mask         = nvic_irq_mask,
    .irq_mask_ack     = NULL,   /**< Separate mask + ack */
    .irq_unmask       = nvic_irq_unmask,
    .irq_eoi          = nvic_irq_eoi,
    .irq_retrigger    = NULL,
    .irq_set_type     = nvic_irq_set_type,
    .irq_set_affinity = nvic_irq_set_affinity,
    .flags            = 0,
};

/**
 * @brief Retrieve NVIC irq_chip instance
 *
 * @return Pointer to static irq_chip instance
 */
struct irq_chip *irq_chip_nvic_get(void)
{
    return &_nvic_chip;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Hardware IRQ Binding                                                      */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Bind logical IRQ to NVIC hardware interrupt
 *
 * @param irq      Logical IRQ ID
 * @param irqn     NVIC IRQ number
 * @param priority Interrupt priority
 *
 * @details
 * Associates a generic IRQ descriptor with a physical NVIC line and
 * initializes its priority.
 *
 * @pre irq must map to a valid irq_desc
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