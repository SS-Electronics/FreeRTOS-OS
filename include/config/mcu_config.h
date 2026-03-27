/**
 * @file    mcu_config.h
 * @brief   MCU and peripheral configuration — STM32F411VET6
 *
 * Static peripheral-count and ID definitions for the STM32F411VET6.
 * Dynamic enable/disable of HAL modules is controlled by Kconfig
 * (make menuconfig) and reflected in autoconf.h → stm32f4xx_hal_conf.h.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_
#define OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_

/* Pull in every CONFIG_* symbol produced by "make config-outputs" */
#include "autoconf.h"


/* ═══════════════════════════════════════════════════════════════════════════
 * MCU Vendor / Variant Selection
 * ═══════════════════════════════════════════════════════════════════════════ */
#define MCU_VAR_MICROCHIP   1
#define MCU_VAR_STM         2

#define CONFIG_DEVICE_VARIANT    MCU_VAR_STM   /* STM32F411VET6 */


/* ═══════════════════════════════════════════════════════════════════════════
 * STM32F411VET6 — available peripheral instance counts
 * (hardware-fixed; not configurable via Kconfig)
 * ═══════════════════════════════════════════════════════════════════════════ */

#define MCU_MAX_UART_PERIPHERAL     3   /* USART1, USART2, USART6          */
#define MCU_MAX_IIC_PERIPHERAL      3   /* I2C1, I2C2, I2C3                */
#define MCU_MAX_SPI_PERIPHERAL      5   /* SPI1..SPI5 (shared with I2S)    */
#define MCU_MAX_TIM_PERIPHERAL      8   /* TIM1..TIM5, TIM9..TIM11         */

/* No CAN, No DAC, No FMC, No ETH, No SAI, No DMA2D on STM32F411 */


/* ═══════════════════════════════════════════════════════════════════════════
 * OS-managed peripheral counts  (from Kconfig → autoconf.h)
 * These control IPC queue array sizes and service thread counts.
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifndef CONFIG_NO_OF_UART
  #define NO_OF_UART    1
#else
  #define NO_OF_UART    CONFIG_NO_OF_UART
#endif

#ifndef CONFIG_NO_OF_IIC
  #define NO_OF_IIC     0
#else
  #define NO_OF_IIC     CONFIG_NO_OF_IIC
#endif


/* ═══════════════════════════════════════════════════════════════════════════
 * Peripheral enable flags  (legacy macros, kept for driver compatibility)
 * Driven by Kconfig — enable a HAL module in menuconfig to turn these on.
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifdef CONFIG_HAL_IWDG_MODULE_ENABLED
  #define CONFIG_MCU_WDG_EN               (1)
#else
  #define CONFIG_MCU_WDG_EN               (0)
#endif

#define CONFIG_MCU_FLASH_DRV_EN           (1)   /* Always available */

#define CONFIG_MCU_NO_OF_UART_PERIPHERAL  NO_OF_UART
#define CONFIG_MCU_NO_OF_IIC_PERIPHERAL   NO_OF_IIC

#ifdef CONFIG_HAL_SPI_MODULE_ENABLED
  #define CONFIG_MCU_NO_OF_SPI_PERIPHERAL (1)
#else
  #define CONFIG_MCU_NO_OF_SPI_PERIPHERAL (0)
#endif

/* RS-485, CAN, USB, ETH — not on STM32F411 */
#define CONFIG_MCU_NO_OF_RS485_PERIPHERAL (0)
#define CONFIG_MCU_NO_OF_CAN_PERIPHERAL   (0)   /* No CAN on STM32F411 */
#define CONFIG_MCU_NO_OF_USB_PERIPHERAL   (0)
#define CONFIG_MCU_NO_OF_ETH_PERIPHERAL   (0)   /* No ETH on STM32F411 */

#ifdef CONFIG_HAL_TIM_MODULE_ENABLED
  #define CONFIG_MCU_NO_OF_TIMER_PERIPHERAL (2) /* TIM2 (1ms base), TIM3 (app) */
#else
  #define CONFIG_MCU_NO_OF_TIMER_PERIPHERAL (0)
#endif


/* ═══════════════════════════════════════════════════════════════════════════
 * Individual UART instance enable
 * STM32F411VET6 has USART1 (PA9/PA10), USART2 (PA2/PA3), USART6 (PC6/PC7)
 * ═══════════════════════════════════════════════════════════════════════════ */

#define UART_1_EN    (NO_OF_UART >= 1 ? 1 : 0)
#define UART_2_EN    (NO_OF_UART >= 2 ? 1 : 0)
#define UART_6_EN    (NO_OF_UART >= 3 ? 1 : 0)  /* USART6 is instance 3 on F411 */
#define UART_3_EN    (0)   /* Not present on F411VET6 */
#define UART_4_EN    (0)   /* Not present on F411VET6 */
#define UART_5_EN    (0)   /* Not present on F411VET6 */


/* ═══════════════════════════════════════════════════════════════════════════
 * Peripheral identifier enumerations
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum
{
    UART_1 = 0,  /* USART1 */
    UART_2,      /* USART2 */
    UART_3,      /* not on F411, placeholder */
    UART_4,      /* not on F411, placeholder */
    UART_5,      /* not on F411, placeholder */
    UART_6,      /* USART6 */
    UART_7,
    UART_8
} UART_PORTS;

typedef enum
{
    IIC_1 = 0,
    IIC_2,
    IIC_3
} IIC_PORTS;

typedef enum
{
    TIMER_1,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    INVALID_TIMER_ID
} TIMER_PORTS;


#endif /* OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_ */
