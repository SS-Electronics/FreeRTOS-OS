/**
 * @file    hal_spi_stm32.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL SPI backend interface for generic SPI driver
 *
 * @details
 * This module provides the STM32-specific HAL implementation for the
 * generic SPI driver (drv_spi). It exposes a HAL operations table
 * (drv_spi_hal_ops_t) that maps SPI operations to STM32 HAL APIs.
 *
 * The driver supports:
 * - Binding STM32 HAL SPI functions to the generic SPI driver
 * - Direct configuration of SPI peripheral parameters before initialization
 * - IRQ dispatch integration with STM32 HAL (HAL_SPI_IRQHandler)
 *
 * The HAL operations table returned by this module must be passed to
 * drv_spi_register() during initialization to enable platform-independent
 * SPI communication.
 *
 * Typical usage:
 * @code
 * drv_spi_handle_t spi = DRV_SPI_HANDLE_INIT(SPI_1, 1000000, 10);
 *
 * hal_spi_stm32_set_config(&spi, SPI1,
 *                          SPI_MODE_MASTER,
 *                          SPI_DIRECTION_2LINES,
 *                          SPI_DATASIZE_8BIT,
 *                          SPI_POLARITY_LOW,
 *                          SPI_PHASE_1EDGE,
 *                          SPI_NSS_SOFT,
 *                          SPI_BAUDRATEPRESCALER_8,
 *                          SPI_FIRSTBIT_MSB);
 *
 * drv_spi_register(SPI_1, hal_spi_stm32_get_ops(), 1000000, 10);
 * @endcode
 *
 * @dependencies
 * board/mcu_config.h, drivers/drv_spi.h
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

#ifndef DRIVERS_HAL_STM32_HAL_SPI_STM32_H_
#define DRIVERS_HAL_STM32_HAL_SPI_STM32_H_

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)

#include <drivers/drv_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the STM32 SPI HAL operations table.
 *
 * Returns a pointer to a constant drv_spi_hal_ops_t structure that
 * implements SPI operations using STM32 HAL APIs.
 *
 * This pointer must be passed to drv_spi_register() to bind the
 * STM32 backend with the generic SPI driver.
 *
 * @note
 * The returned structure is statically allocated and must not be
 * modified or freed by the caller.
 *
 * @return Pointer to STM32 SPI HAL operations table.
 */
const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void);

/**
 * @brief STM32 SPI IRQ dispatcher.
 *
 * This function must be called from the corresponding SPIx_IRQHandler.
 * It routes the interrupt to the appropriate SPI handle and invokes
 * HAL_SPI_IRQHandler() internally.
 *
 * This ensures proper handling of interrupt-driven SPI operations while
 * keeping the generic driver layer hardware-independent.
 *
 * @param[in] instance
 *     STM32 SPI peripheral instance that triggered the interrupt (e.g. SPI1, SPI2).
 */
void hal_spi_stm32_irq_handler(SPI_TypeDef *instance);

/**
 * @brief Configure STM32 SPI hardware parameters.
 *
 * This function initializes the hardware-specific context inside the
 * generic SPI handle before registration. It sets parameters required
 * by the STM32 HAL (SPI_HandleTypeDef).
 *
 * Must be called before drv_spi_register().
 *
 * @param[in,out] h
 *     Pointer to the generic SPI handle whose hardware context will be configured.
 *
 * @param[in] instance
 *     STM32 SPI peripheral instance (SPI1, SPI2, etc.).
 *
 * @param[in] mode
 *     SPI mode (SPI_MODE_MASTER or SPI_MODE_SLAVE).
 *
 * @param[in] direction
 *     Data direction (SPI_DIRECTION_2LINES or SPI_DIRECTION_1LINE).
 *
 * @param[in] data_size
 *     Data size (SPI_DATASIZE_8BIT or SPI_DATASIZE_16BIT).
 *
 * @param[in] clk_polarity
 *     Clock polarity (SPI_POLARITY_LOW or SPI_POLARITY_HIGH).
 *
 * @param[in] clk_phase
 *     Clock phase (SPI_PHASE_1EDGE or SPI_PHASE_2EDGE).
 *
 * @param[in] nss
 *     NSS management mode (SPI_NSS_SOFT, SPI_NSS_HARD_INPUT, SPI_NSS_HARD_OUTPUT).
 *
 * @param[in] baud_prescaler
 *     Baud rate prescaler (SPI_BAUDRATEPRESCALER_2 to SPI_BAUDRATEPRESCALER_256).
 *
 * @param[in] bit_order
 *     Bit transmission order (SPI_FIRSTBIT_MSB or SPI_FIRSTBIT_LSB).
 */
void hal_spi_stm32_set_config(drv_spi_handle_t *h,
                               SPI_TypeDef      *instance,
                               uint32_t          mode,
                               uint32_t          direction,
                               uint32_t          data_size,
                               uint32_t          clk_polarity,
                               uint32_t          clk_phase,
                               uint32_t          nss,
                               uint32_t          baud_prescaler,
                               uint32_t          bit_order);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_SPI_MODULE_ENABLED */

#endif /* DRIVERS_HAL_STM32_HAL_SPI_STM32_H_ */