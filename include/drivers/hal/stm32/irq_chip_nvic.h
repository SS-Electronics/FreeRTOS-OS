/*
 * irq_chip_nvic.h — ARM Cortex-M NVIC irq_chip implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Provides an irq_chip backed by the ARM NVIC (Nested Vectored Interrupt
 * Controller).  Used as the chip for any software IRQ ID that maps 1:1 to
 * a hardware NVIC line (e.g. USART1, SPI1, I2C1_EV).
 *
 * Software-only IRQ IDs (no hardware line) use handle_simple_irq with a NULL
 * chip instead.
 *
 * Usage:
 *   irq_set_chip_and_handler(IRQ_ID_UART_RX(0),
 *                             irq_chip_nvic_get(),
 *                             handle_simple_irq);
 *   irq_chip_nvic_bind_hwirq(IRQ_ID_UART_RX(0), USART1_IRQn, 5);
 */

#ifndef INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_
#define INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_

#include <irq/irq_desc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * irq_chip_nvic_get — return the singleton NVIC irq_chip.
 *
 * The same chip object is shared by all IRQ descriptors — it is stateless
 * (all state lives in irq_data.hwirq / chip_data).
 */
struct irq_chip *irq_chip_nvic_get(void);

/**
 * irq_chip_nvic_bind_hwirq — associate a software IRQ ID with an NVIC line.
 *
 * Sets irq_data.hwirq to @p irqn and configures priority.  Must be called
 * before irq_enable() so the chip ops have a valid hwirq to act on.
 *
 * @param irq      Software IRQ ID.
 * @param irqn     Hardware NVIC IRQn number (e.g. USART1_IRQn).
 * @param priority NVIC preempt priority.
 */
void irq_chip_nvic_bind_hwirq(irq_id_t irq, int32_t irqn, uint32_t priority);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_HAL_STM32_IRQ_CHIP_NVIC_H_ */
