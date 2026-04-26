/**
 * @file    hal_uart_stm32.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL UART backend interface for generic UART driver
 *
 * @details
 * This module provides the STM32-specific HAL implementation for the
 * generic UART driver (drv_uart). It exposes a HAL operations table
 * (drv_uart_hal_ops_t) that binds STM32 HAL APIs to the portable driver layer.
 *
 * The driver supports:
 * - Binding STM32 HAL UART functions to the generic UART driver
 * - Direct configuration of UART peripheral parameters before initialization
 * - IRQ dispatch integration with STM32 HAL (HAL_UART_IRQHandler)
 *
 * The returned ops table is static and must be passed to drv_uart_register()
 * during system initialization. This enables platform-independent UART usage
 * across the system.
 *
 * Typical usage:
 * @code
 * drv_uart_handle_t uart = DRV_UART_HANDLE_INIT(UART_1, 115200, 10);
 *
 * hal_uart_stm32_set_config(&uart, USART1,
 *                           UART_WORDLENGTH_8B,
 *                           UART_STOPBITS_1,
 *                           UART_PARITY_NONE,
 *                           UART_MODE_TX_RX);
 *
 * drv_uart_register(UART_1, hal_uart_stm32_get_ops(), 115200, 10);
 * @endcode
 *
 * @dependencies
 * board/mcu_config.h, drivers/com/drv_uart.h
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

#ifndef DRIVERS_HAL_STM32_HAL_UART_STM32_H_
#define DRIVERS_HAL_STM32_HAL_UART_STM32_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <board/board_config.h>
#include <board/board_handles.h>
#include <board/board_device_ids.h>
#include <drivers/drv_uart.h>


#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Get the STM32 UART HAL operations table.
 *
 * Returns a pointer to a constant drv_uart_hal_ops_t structure that
 * implements all UART operations using STM32 HAL APIs.
 *
 * This pointer must be passed to drv_uart_register() to bind the
 * STM32 backend with the generic UART driver.
 *
 * @note
 * The returned structure is statically allocated and must not be
 * modified or freed by the caller.
 *
 * @return Pointer to STM32 UART HAL operations table.
 */
const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void);

/**
 * @brief  Configure STM32 UART hardware parameters.
 *
 * This function initializes the hardware-specific context inside the
 * generic UART handle before registration. It directly sets fields
 * required by the STM32 HAL (UART_HandleTypeDef).
 *
 * Must be called before drv_uart_register().
 *
 * @param[in,out] h
 *     Pointer to the generic UART handle whose hardware context will be configured.
 *
 * @param[in] instance
 *     STM32 USART peripheral instance (e.g. USART1, USART2).
 *
 * @param[in] word_len
 *     UART word length (e.g. UART_WORDLENGTH_8B, UART_WORDLENGTH_9B).
 *
 * @param[in] stop_bits
 *     Number of stop bits (e.g. UART_STOPBITS_1, UART_STOPBITS_2).
 *
 * @param[in] parity
 *     Parity configuration (UART_PARITY_NONE, EVEN, ODD).
 *
 * @param[in] mode
 *     UART mode (UART_MODE_TX_RX, UART_MODE_TX, UART_MODE_RX).
 */
void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef     *instance,
                               uint32_t           word_len,
                               uint32_t           stop_bits,
                               uint32_t           parity,
                               uint32_t           mode);

/**
 * @brief  STM32 UART IRQ dispatcher.
 *
 * This function must be called from the corresponding USARTx_IRQHandler.
 * It routes the interrupt to the correct UART handle and invokes
 * HAL_UART_IRQHandler() internally.
 *
 * This allows the generic UART driver to remain decoupled from
 * hardware-specific interrupt handling.
 *
 * @param[in] instance
 *     STM32 USART peripheral instance that triggered the interrupt.
 */
void hal_uart_stm32_irq_handler(USART_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_UART_STM32_H_ */