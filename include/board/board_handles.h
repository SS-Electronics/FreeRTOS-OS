/*
 * AUTO-GENERATED — DO NOT EDIT
 * Generator : scripts/gen_board_config.py
 * Source    : boards/stm32f411_devboard.xml
 * Date      : 2026-04-18
 *
 * Re-generate:
 *   python3 scripts/gen_board_config.py boards/stm32f411_devboard.xml
 */

#ifndef BOARD_STM32F411_DEVBOARD_HANDLES_H_
#define BOARD_STM32F411_DEVBOARD_HANDLES_H_

/* Board: stm32f411_devboard  MCU: STM32F411VET6 */

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f4xx_hal_i2c.h"
extern I2C_HandleTypeDef hi2c1;
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f4xx_hal_spi.h"
extern SPI_HandleTypeDef hspi1;
#endif /* HAL_SPI_MODULE_ENABLED */

#endif /* BOARD_STM32F411_DEVBOARD_HANDLES_H_ */
