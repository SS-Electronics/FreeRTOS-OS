/*
 * hal_spi_stm32.h — STM32 HAL SPI ops table declaration
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_STM32_HAL_SPI_STM32_H_
#define DRIVERS_HAL_STM32_HAL_SPI_STM32_H_

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)

#include <drivers/drv_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the STM32 SPI HAL ops table. */
const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void);

/**
 * @brief  Populate the hardware context in a generic SPI handle.
 *
 * @param  h            Generic SPI handle.
 * @param  instance     SPI peripheral instance (SPI1, SPI2, …).
 * @param  mode         SPI_MODE_MASTER / SPI_MODE_SLAVE
 * @param  direction    SPI_DIRECTION_2LINES / SPI_DIRECTION_1LINE
 * @param  data_size    SPI_DATASIZE_8BIT / SPI_DATASIZE_16BIT
 * @param  clk_polarity SPI_POLARITY_LOW / SPI_POLARITY_HIGH
 * @param  clk_phase    SPI_PHASE_1EDGE / SPI_PHASE_2EDGE
 * @param  nss          SPI_NSS_SOFT / SPI_NSS_HARD_INPUT / SPI_NSS_HARD_OUTPUT
 * @param  baud_prescaler SPI_BAUDRATEPRESCALER_2 … SPI_BAUDRATEPRESCALER_256
 * @param  bit_order    SPI_FIRSTBIT_MSB / SPI_FIRSTBIT_LSB
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
