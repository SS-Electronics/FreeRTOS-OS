/*
 * hal_uart_stm32.h — STM32 HAL UART ops table declaration
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Usage
 * ─────
 *   #include <drivers/com/hal/stm32/hal_uart_stm32.h>
 *
 *   // At init time, bind the STM32 ops to a generic UART handle:
 *   drv_uart_register(UART_1, hal_uart_stm32_get_ops(), 115200, 10);
 */

#ifndef DRIVERS_HAL_STM32_HAL_UART_STM32_H_
#define DRIVERS_HAL_STM32_HAL_UART_STM32_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Return a pointer to the STM32 UART HAL ops table.
 *
 * Pass this to drv_uart_register() to bind STM32 HAL functions to a generic
 * UART handle.  The returned pointer is to a static const struct — do not
 * free or modify it.
 */
const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void);

/**
 * @brief  Configure the STM32 peripheral parameters directly in the hw context
 *         before calling drv_uart_register().
 *
 * @param  h            Generic UART handle whose hw.huart will be filled.
 * @param  instance     USART peripheral instance (e.g. USART1, USART2).
 * @param  word_len     UART_WORDLENGTH_8B / UART_WORDLENGTH_9B
 * @param  stop_bits    UART_STOPBITS_1 / UART_STOPBITS_2
 * @param  parity       UART_PARITY_NONE / UART_PARITY_EVEN / UART_PARITY_ODD
 * @param  mode         UART_MODE_TX_RX / UART_MODE_TX / UART_MODE_RX
 */
void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef     *instance,
                               uint32_t           word_len,
                               uint32_t           stop_bits,
                               uint32_t           parity,
                               uint32_t           mode);

/**
 * @brief  Called from the USARTx_IRQHandler to dispatch HAL_UART_IRQHandler
 *         to whichever generic handle owns @p instance.
 *
 * @param  instance  USART peripheral instance (e.g. USART1, USART2).
 */
void hal_uart_stm32_irq_handler(USART_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_UART_STM32_H_ */
