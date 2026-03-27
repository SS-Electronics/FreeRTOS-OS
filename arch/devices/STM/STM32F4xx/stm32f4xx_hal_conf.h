/**
 ******************************************************************************
 * @file    stm32f4xx_hal_conf.h
 * @brief   HAL configuration — driven by Kconfig via autoconf.h
 *
 * DO NOT hand-edit module enables or clock values here.
 * Run  make menuconfig   to change settings interactively.
 * Run  make config-outputs  to regenerate include/config/autoconf.h.
 * Then rebuild with  make all.
 *
 * Original template: STMicroelectronics MCD Application Team
 * Adapted for FreeRTOS-OS Kconfig integration: Subhajit Roy
 ******************************************************************************
 */

#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Pull in every CONFIG_* symbol produced by "make config-outputs" */
#include "autoconf.h"


/* ##########################################################################
 * HAL Module Selection
 * Each define below mirrors the corresponding CubeMX checkbox.
 * The source of truth is .config → autoconf.h → here.
 * ########################################################################## */

#define HAL_MODULE_ENABLED   /* master HAL enable — always on */

#ifdef CONFIG_HAL_CORTEX_MODULE_ENABLED
  #define HAL_CORTEX_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_PWR_MODULE_ENABLED
  #define HAL_PWR_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_RCC_MODULE_ENABLED
  #define HAL_RCC_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_GPIO_MODULE_ENABLED
  #define HAL_GPIO_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_EXTI_MODULE_ENABLED
  #define HAL_EXTI_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_FLASH_MODULE_ENABLED
  #define HAL_FLASH_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_DMA_MODULE_ENABLED
  #define HAL_DMA_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_IWDG_MODULE_ENABLED
  #define HAL_IWDG_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_WWDG_MODULE_ENABLED
  #define HAL_WWDG_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_CRC_MODULE_ENABLED
  #define HAL_CRC_MODULE_ENABLED
#endif

/* Communication */
#ifdef CONFIG_HAL_UART_MODULE_ENABLED
  #define HAL_UART_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_USART_MODULE_ENABLED
  #define HAL_USART_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_SPI_MODULE_ENABLED
  #define HAL_SPI_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_I2C_MODULE_ENABLED
  #define HAL_I2C_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_I2S_MODULE_ENABLED
  #define HAL_I2S_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_IRDA_MODULE_ENABLED
  #define HAL_IRDA_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_SMARTCARD_MODULE_ENABLED
  #define HAL_SMARTCARD_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_SMBUS_MODULE_ENABLED
  #define HAL_SMBUS_MODULE_ENABLED
#endif

/* Timer */
#ifdef CONFIG_HAL_TIM_MODULE_ENABLED
  #define HAL_TIM_MODULE_ENABLED
#endif

/* Analog */
#ifdef CONFIG_HAL_ADC_MODULE_ENABLED
  #define HAL_ADC_MODULE_ENABLED
#endif

/* USB */
#ifdef CONFIG_HAL_PCD_MODULE_ENABLED
  #define HAL_PCD_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_HCD_MODULE_ENABLED
  #define HAL_HCD_MODULE_ENABLED
#endif

/* Storage */
#ifdef CONFIG_HAL_SD_MODULE_ENABLED
  #define HAL_SD_MODULE_ENABLED
#endif

/* RTC & Security */
#ifdef CONFIG_HAL_RTC_MODULE_ENABLED
  #define HAL_RTC_MODULE_ENABLED
#endif

#ifdef CONFIG_HAL_RNG_MODULE_ENABLED
  #define HAL_RNG_MODULE_ENABLED
#endif


/* ##########################################################################
 * Clock values — sourced from CONFIG_* integers in autoconf.h.
 * Guard with #if !defined so a toolchain -D flag can still override.
 * ########################################################################## */

#if !defined(HSE_VALUE)
  #define HSE_VALUE    ((uint32_t)CONFIG_HSE_VALUE)
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    ((uint32_t)CONFIG_HSE_STARTUP_TIMEOUT)
#endif

#if !defined(HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)CONFIG_HSI_VALUE)
#endif

#if !defined(LSI_VALUE)
  #define LSI_VALUE    ((uint32_t)CONFIG_LSI_VALUE)
#endif

#if !defined(LSE_VALUE)
  #define LSE_VALUE    ((uint32_t)CONFIG_LSE_VALUE)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    ((uint32_t)CONFIG_LSE_STARTUP_TIMEOUT)
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
  #define EXTERNAL_CLOCK_VALUE   ((uint32_t)CONFIG_EXTERNAL_CLOCK_VALUE)
#endif


/* ##########################################################################
 * System / board configuration
 * ########################################################################## */

#define  VDD_VALUE                    ((uint32_t)CONFIG_VDD_VALUE)
#define  TICK_INT_PRIORITY            ((uint32_t)CONFIG_TICK_INT_PRIORITY)
#define  USE_RTOS                     0U   /* Must stay 0 — STM32 HAL rejects 1 (stm32f4xx_hal_def.h:91) */

#ifdef CONFIG_PREFETCH_ENABLE
  #define PREFETCH_ENABLE             1U
#else
  #define PREFETCH_ENABLE             0U
#endif

#ifdef CONFIG_INSTRUCTION_CACHE_ENABLE
  #define INSTRUCTION_CACHE_ENABLE    1U
#else
  #define INSTRUCTION_CACHE_ENABLE    0U
#endif

#ifdef CONFIG_DATA_CACHE_ENABLE
  #define DATA_CACHE_ENABLE           1U
#else
  #define DATA_CACHE_ENABLE           0U
#endif


/* ##########################################################################
 * Register-callback switches — all off by default; enable per-peripheral
 * if you need runtime-swappable callbacks.
 * ########################################################################## */

#define USE_HAL_ADC_REGISTER_CALLBACKS         0U
#define USE_HAL_CAN_REGISTER_CALLBACKS         0U
#define USE_HAL_CEC_REGISTER_CALLBACKS         0U
#define USE_HAL_CRYP_REGISTER_CALLBACKS        0U
#define USE_HAL_DAC_REGISTER_CALLBACKS         0U
#define USE_HAL_DCMI_REGISTER_CALLBACKS        0U
#define USE_HAL_DFSDM_REGISTER_CALLBACKS       0U
#define USE_HAL_DMA2D_REGISTER_CALLBACKS       0U
#define USE_HAL_DSI_REGISTER_CALLBACKS         0U
#define USE_HAL_ETH_REGISTER_CALLBACKS         0U
#define USE_HAL_HASH_REGISTER_CALLBACKS        0U
#define USE_HAL_HCD_REGISTER_CALLBACKS         0U
#define USE_HAL_I2C_REGISTER_CALLBACKS         0U
#define USE_HAL_FMPI2C_REGISTER_CALLBACKS      0U
#define USE_HAL_FMPSMBUS_REGISTER_CALLBACKS    0U
#define USE_HAL_I2S_REGISTER_CALLBACKS         0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS        0U
#define USE_HAL_LPTIM_REGISTER_CALLBACKS       0U
#define USE_HAL_LTDC_REGISTER_CALLBACKS        0U
#define USE_HAL_MMC_REGISTER_CALLBACKS         0U
#define USE_HAL_NAND_REGISTER_CALLBACKS        0U
#define USE_HAL_NOR_REGISTER_CALLBACKS         0U
#define USE_HAL_PCCARD_REGISTER_CALLBACKS      0U
#define USE_HAL_PCD_REGISTER_CALLBACKS         0U
#define USE_HAL_QSPI_REGISTER_CALLBACKS        0U
#define USE_HAL_RNG_REGISTER_CALLBACKS         0U
#define USE_HAL_RTC_REGISTER_CALLBACKS         0U
#define USE_HAL_SAI_REGISTER_CALLBACKS         0U
#define USE_HAL_SD_REGISTER_CALLBACKS          0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS   0U
#define USE_HAL_SDRAM_REGISTER_CALLBACKS       0U
#define USE_HAL_SRAM_REGISTER_CALLBACKS        0U
#define USE_HAL_SPDIFRX_REGISTER_CALLBACKS     0U
#define USE_HAL_SMBUS_REGISTER_CALLBACKS       0U
#define USE_HAL_SPI_REGISTER_CALLBACKS         0U
#define USE_HAL_TIM_REGISTER_CALLBACKS         0U
#define USE_HAL_UART_REGISTER_CALLBACKS        0U
#define USE_HAL_USART_REGISTER_CALLBACKS       0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS        0U


/* ##########################################################################
 * assert_param
 * ########################################################################## */

#ifdef CONFIG_USE_FULL_ASSERT
  #define USE_FULL_ASSERT    1U
#endif

#ifdef USE_FULL_ASSERT
  #define assert_param(expr) \
      ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif


/* ##########################################################################
 * SPI CRC feature
 * ########################################################################## */

#define USE_SPI_CRC    0U


/* ##########################################################################
 * Conditional HAL driver includes
 * ########################################################################## */

#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32f4xx_hal_exti.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32f4xx_hal_adc.h"
#endif
#ifdef HAL_CRC_MODULE_ENABLED
  #include "stm32f4xx_hal_crc.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f4xx_hal_i2c.h"
#endif
#ifdef HAL_SMBUS_MODULE_ENABLED
  #include "stm32f4xx_hal_smbus.h"
#endif
#ifdef HAL_I2S_MODULE_ENABLED
  #include "stm32f4xx_hal_i2s.h"
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
  #include "stm32f4xx_hal_iwdg.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif
#ifdef HAL_RNG_MODULE_ENABLED
  #include "stm32f4xx_hal_rng.h"
#endif
#ifdef HAL_RTC_MODULE_ENABLED
  #include "stm32f4xx_hal_rtc.h"
#endif
#ifdef HAL_SD_MODULE_ENABLED
  #include "stm32f4xx_hal_sd.h"
#endif
#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32f4xx_hal_spi.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f4xx_hal_uart.h"
#endif
#ifdef HAL_USART_MODULE_ENABLED
  #include "stm32f4xx_hal_usart.h"
#endif
#ifdef HAL_IRDA_MODULE_ENABLED
  #include "stm32f4xx_hal_irda.h"
#endif
#ifdef HAL_SMARTCARD_MODULE_ENABLED
  #include "stm32f4xx_hal_smartcard.h"
#endif
#ifdef HAL_WWDG_MODULE_ENABLED
  #include "stm32f4xx_hal_wwdg.h"
#endif
#ifdef HAL_PCD_MODULE_ENABLED
  #include "stm32f4xx_hal_pcd.h"
#endif
#ifdef HAL_HCD_MODULE_ENABLED
  #include "stm32f4xx_hal_hcd.h"
#endif


#include "device.h"

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
