/**
 * @file        hal_eth_stm32.h
 * @brief       STM32H7 Ethernet MAC backend — ISR entry declaration
 * @ingroup     hal_stm32
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      STM32 HAL
 * @info        Declares the ETH global IRQ handler that the generated vector
 *              dispatch (app/board/irq_periph_dispatch_generated.c) routes the
 *              ETH_IRQn vector to. The driver implementation lives in
 *              hal_eth_stm32.c and is only compiled when CONFIG_NET=y.
 * @dependency  drv_eth.h
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
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
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef DRIVERS_HAL_STM32_HAL_ETH_STM32_H_
#define DRIVERS_HAL_STM32_HAL_ETH_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ETH global interrupt entry point (ETH_IRQn / vector 61).
 * @note  Called from the generated ETH_IRQHandler. Services the MAC/DMA
 *        interrupt and signals the net RX thread on a completed receive.
 */
void hal_eth_stm32_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_STM32_HAL_ETH_STM32_H_ */
