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
 * Clock tree (HSI source, STM32F411xE @ 100 MHz):
 *
 *   HSI (CONFIG_HSI_VALUE = 16 MHz)
 *     └─ /PLLM (16) ──→ 1 MHz VCO input
 *          └─ ×PLLN (200) ──→ 200 MHz VCO output
 *               ├─ /PLLP (2)  ──→ SYSCLK = 100 MHz
 *               └─ /PLLQ (4)  ──→ USB/SDIO =  50 MHz
 *   AHB  = SYSCLK / 1 = 100 MHz
 *   APB1 = HCLK   / 2 =  50 MHz  (F411 max)
 *   APB2 = HCLK   / 1 = 100 MHz
 *   Flash latency = 3 wait-states
 *
 * All PLL constants are derived from CONFIG_HSI_VALUE (autoconf.h via
 * board/mcu_config.h).  To retarget, adjust the three _VAL macros and
 * RCC_FLASH_LATENCY below.
 */

#ifndef OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_
#define OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── CMSIS system symbols — defined in hal_rcc_stm32.c ──────────────────── */

extern uint32_t      SystemCoreClock;    /*!< SYSCLK frequency in Hz         */
extern const uint8_t AHBPrescTable[16]; /*!< AHB prescaler → shift LUT      */
extern const uint8_t APBPrescTable[8];  /*!< APB prescaler → shift LUT      */

/* ── CMSIS-mandated API ──────────────────────────────────────────────────── */

/* Called from Reset_Handler before .data/.bss initialisation.
 * Enables FPU and optionally relocates the vector table.               */
void SystemInit(void);

/* Updates SystemCoreClock by reading the live RCC register state.
 * Called internally by HAL_RCC_ClockConfig() and by the HAL library.  */
void SystemCoreClockUpdate(void);

/* ── PLL parameters — derived from CONFIG_HSI_VALUE ─────────────────────── */
/* These are used only in hal_rcc_stm32.c; exposed here for documentation.  */

/* VCO input must be 1–2 MHz.  PLLM = HSI_MHz gives 1 MHz.                  */
#define RCC_PLLM_VAL        ((uint32_t)(16U))   /* CONFIG_HSI_VALUE / 1 MHz  */
#define RCC_PLLN_VAL        200U
#define RCC_PLLP_VAL        RCC_PLLP_DIV2       /* SYSCLK = 200 / 2 = 100 MHz */
#define RCC_PLLQ_VAL        4U                  /* 200 / 4 = 50 MHz           */

/* Resulting bus frequencies (informational — drivers should query HAL)      */
#define RCC_SYSCLK_HZ       100000000UL
#define RCC_APB1_HZ          50000000UL
#define RCC_APB2_HZ         100000000UL

/* Flash wait-states for 90–100 MHz, VDD 2.7–3.6 V (STM32F411 RM §3.4)    */
#define RCC_FLASH_LATENCY   FLASH_LATENCY_3

/* ── drv_rcc integration — internal registration shim ───────────────────── */

#include <drivers/drv_rcc.h>

void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_HAL_STM32_HAL_RCC_STM32_H_ */
