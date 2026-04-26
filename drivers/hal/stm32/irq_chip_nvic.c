/**
 * @file    irq_chip_nvic.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  irq_chip
 * @brief   NVIC-backed IRQ controller implementation for Cortex-M (STM32)
 *
 * @details
 * This module implements the generic IRQ controller abstraction
 * (`struct irq_chip`) using the ARM Cortex-M NVIC hardware.
 *
 * It provides a translation layer between:
 * - Generic IRQ subsystem (irq_desc / irq_data)
 * - STM32 HAL NVIC APIs
 *
 * The implementation enables platform-independent interrupt handling
 * while delegating hardware-specific operations to the NVIC.
 *
 * Features:
 * - IRQ enable/disable via NVIC
 * - Mask/unmask abstraction
 * - Priority configuration
 * - Logical IRQ to hardware IRQ binding
 *
 * Limitations:
 * - No explicit ACK or EOI required (handled by hardware)
 * - Trigger type cannot be configured (peripheral-defined)
 *
 * @dependencies
 * board/mcu_config.h, device.h,
 * drivers/hal/stm32/irq_chip_nvic.h
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

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/irq_chip_nvic.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Chip Operations                                                       */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Enable an IRQ line in NVIC.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * Enables interrupt delivery for the given hardware IRQ line.
 *
 * @note
 * - No effect if @p data is NULL
 * - No effect if @p hwirq is invalid (< 0)
 */
__SECTION_OS __USED
static void nvic_irq_enable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_EnableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Disable an IRQ line in NVIC.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * Prevents interrupt delivery from the specified hardware IRQ.
 */
__SECTION_OS __USED
static void nvic_irq_disable(struct irq_data *data)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_DisableIRQ((IRQn_Type)data->hwirq);
}

/**
 * @brief Mask an IRQ line.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * On Cortex-M, masking is equivalent to disabling the IRQ in NVIC.
 */
__SECTION_OS __USED
static void nvic_irq_mask(struct irq_data *data)
{
    nvic_irq_disable(data);
}

/**
 * @brief Unmask an IRQ line.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * On Cortex-M, unmasking is equivalent to enabling the IRQ in NVIC.
 */
__SECTION_OS __USED
static void nvic_irq_unmask(struct irq_data *data)
{
    nvic_irq_enable(data);
}

/**
 * @brief Acknowledge an IRQ.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * NVIC does not require explicit acknowledgment.
 * Interrupt sources must be cleared at the peripheral level.
 */
__SECTION_OS __USED
static void nvic_irq_ack(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief End-of-interrupt handler.
 *
 * @param data Pointer to IRQ descriptor data.
 *
 * @details
 * Cortex-M automatically handles interrupt completion on ISR exit.
 * No explicit EOI operation is required.
 */
__SECTION_OS __USED
static void nvic_irq_eoi(struct irq_data *data)
{
    (void)data;
}

/**
 * @brief Set IRQ priority.
 *
 * @param data     Pointer to IRQ descriptor data.
 * @param priority Preemption priority value.
 *
 * @details
 * Configures the NVIC priority for the given IRQ.
 *
 * @warning
 * Priority encoding depends on NVIC PRIGROUP configuration.
 */
__SECTION_OS __USED
static void nvic_irq_set_affinity(struct irq_data *data, uint32_t priority)
{
    if (data == NULL || data->hwirq < 0)
        return;

    HAL_NVIC_SetPriority((IRQn_Type)data->hwirq, priority, 0);
}

/**
 * @brief Configure IRQ trigger type.
 *
 * @param data      Pointer to IRQ descriptor data.
 * @param flow_type Requested trigger type.
 *
 * @retval 0 Always returns success.
 *
 * @details
 * NVIC does not support runtime trigger configuration.
 * Trigger type is fixed by peripheral hardware design.
 */
__SECTION_OS __USED
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
 * @brief Static NVIC irq_chip instance.
 *
 * @details
 * Provides concrete implementation of generic irq_chip interface
 * using Cortex-M NVIC backend.
 */
static struct irq_chip _nvic_chip = {
    .name             = "NVIC",
    .irq_enable       = nvic_irq_enable,
    .irq_disable      = nvic_irq_disable,
    .irq_ack          = nvic_irq_ack,
    .irq_mask         = nvic_irq_mask,
    .irq_mask_ack     = NULL,   /**< Combined mask+ack not required */
    .irq_unmask       = nvic_irq_unmask,
    .irq_eoi          = nvic_irq_eoi,
    .irq_retrigger    = NULL,
    .irq_set_type     = nvic_irq_set_type,
    .irq_set_affinity = nvic_irq_set_affinity,
    .flags            = 0,
};

/**
 * @brief Get NVIC irq_chip instance.
 *
 * @return Pointer to static NVIC irq_chip object.
 *
 * @details
 * Used by IRQ subsystem to attach NVIC controller implementation.
 */
__SECTION_OS __USED
struct irq_chip *irq_chip_nvic_get(void)
{
    return &_nvic_chip;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Hardware IRQ Binding                                                      */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Bind logical IRQ to NVIC hardware interrupt.
 *
 * @param irq      Logical IRQ identifier.
 * @param irqn     Hardware NVIC IRQ number.
 * @param priority Interrupt priority.
 *
 * @details
 * Associates a logical IRQ (software-managed) with a physical NVIC
 * interrupt line and configures its priority.
 *
 * This function updates:
 *   - irq_desc → irq_data.hwirq
 *   - NVIC priority configuration
 *
 * @pre
 * - @p irq must correspond to a valid irq_desc
 *
 * @note
 * Does not enable the IRQ; only binds and configures it.
 */
__SECTION_OS __USED
void irq_chip_nvic_bind_hwirq(irq_id_t irq, int32_t irqn, uint32_t priority)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc == NULL)
        return;

    desc->irq_data.hwirq = irqn;

    HAL_NVIC_SetPriority((IRQn_Type)irqn, priority, 0);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */