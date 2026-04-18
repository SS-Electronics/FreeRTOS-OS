/*
 * AUTO-GENERATED — DO NOT EDIT
 * Generator : scripts/gen_board_config.py
 * Source    : boards/stm32f411_devboard.xml
 * Date      : 2026-04-06
 *
 * Re-generate:
 *   python3 scripts/gen_board_config.py boards/stm32f411_devboard.xml
 */

#ifndef BOARD_STM32F411_DEVBOARD_DEVICE_IDS_H_
#define BOARD_STM32F411_DEVBOARD_DEVICE_IDS_H_

/* Board: stm32f411_devboard  MCU: STM32F411VET6 */

/* UART device IDs */
#define UART_DEBUG                       0
#define UART_APP                         1
#define BOARD_UART_COUNT             2

/* I2C device IDs */
#define I2C_SENSOR_BUS                   0
#define BOARD_IIC_COUNT             1

/* SPI device IDs */
#define SPI_FLASH_BUS                    0
#define BOARD_SPI_COUNT             1

/* GPIO device IDs */
#define LED_BOARD                        0
#define BTN_USER                         1
#define LED_STATUS                       2
#define BOARD_GPIO_COUNT             3

#endif /* BOARD_STM32F411_DEVBOARD_DEVICE_IDS_H_ */
