/*
 * hal_rcc_stm32.h — STM32 RCC + CMSIS system declarations
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Replaces system_stm32f4xx.h.  Included by:
 *   - startup_stm32f411vetx.c  (needs SystemInit declaration)
 *   - device.h                 (exports CMSIS symbols to the rest of the OS)
 *   - hal_rcc_stm32.c          (implementation)
 *   - drv_rcc.c                (forward-declares _hal_rcc_stm32_register)
 *
 * Clock tree parameters and CMSIS variable declarations live in
 * app/board/board_config.h (BOARD_RCC_* macros) and board_config.c
 * (SystemCoreClock / AHBPrescTable / APBPrescTable definitions).
 */

#ifndef OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_
#define OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_

#include <stdint.h>
#include <board/board_config.h>   /* BOARD_RCC_*, BOARD_SYSCLK_HZ, BOARD_FLASH_LATENCY */

#ifdef __cplusplus
extern "C" {
#endif

/* ── CMSIS-mandated API ──────────────────────────────────────────────────── */

/* Called from Reset_Handler before .data/.bss initialisation.
 * Enables FPU and optionally relocates the vector table.               */
void SystemInit(void);

/* Updates SystemCoreClock by reading the live RCC register state.
 * Called internally by HAL_RCC_ClockConfig() and by the HAL library.  */
void SystemCoreClockUpdate(void);

/* ── drv_rcc integration — internal registration shim ───────────────────── */

#include <drivers/drv_rcc.h>

void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out);

/* ── Per-peripheral clock enables ────────────────────────────────────────── */
/* All __HAL_RCC_*_CLK_ENABLE() calls are confined to hal_rcc_stm32.c.
 * Every other file calls these named functions instead of the macros.      */

void hal_rcc_stm32_usart1_clk_en(void);
void hal_rcc_stm32_usart2_clk_en(void);
void hal_rcc_stm32_i2c1_clk_en(void);
void hal_rcc_stm32_spi1_clk_en(void);
void hal_rcc_stm32_tim1_clk_en(void);
void hal_rcc_stm32_syscfg_clk_en(void);
void hal_rcc_stm32_pwr_clk_en(void);

/* Dispatches by drv_rcc_periph_t value — called from drv_rcc_periph_clk_en() */
void hal_rcc_stm32_periph_clk_en(drv_rcc_periph_t periph);

/* GPIO port clock enable — port is GPIO_TypeDef * cast to void * for portability */
void hal_rcc_stm32_gpio_clk_en(void *port);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_ */
