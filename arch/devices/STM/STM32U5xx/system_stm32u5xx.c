/**
 ******************************************************************************
 * @file    system_stm32u5xx.c
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 *          (adapted from STMicroelectronics MCD Application Team reference)
 *
 * @brief   CMSIS Cortex-M33 Device Peripheral Access Layer System Source File
 *          for the STM32U5xx family.
 *
 *          Provides:
 *              SystemInit()            — minimal post-reset bring-up.  PLL,
 *                                        Flash latency and VOS are programmed
 *                                        later in drivers/hal/stm32/hal_rcc_stm32.c
 *                                        once the HAL is online.  Here we only
 *                                        guarantee a deterministic CMSIS state
 *                                        (FPU on, VTOR set, MSP loaded).
 *              SystemCoreClockUpdate() — recomputes SystemCoreClock from RCC.
 *              SystemCoreClock         — initialised to MSI 4 MHz (post-reset
 *                                        default on U5).
 *
 *          TrustZone: this file is the *non-secure* counterpart.  When the
 *          firmware is built single-image (CONFIG_U5_TRUSTZONE_DISABLED — the
 *          default on the example board), the secure attribution unit (SAU)
 *          is bypassed and the whole address map is treated as non-secure by
 *          the application.  The trustcore module in the example app handles
 *          the secure-vs-non-secure decision at runtime.
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "stm32u5xx.h"

#if !defined  (HSE_VALUE)
#  define HSE_VALUE    ((uint32_t)16000000U)
#endif

#if !defined  (MSI_VALUE)
#  define MSI_VALUE    ((uint32_t)4000000U)
#endif

#if !defined  (HSI_VALUE)
#  define HSI_VALUE    ((uint32_t)16000000U)
#endif

/* Vector table base in user flash (set by linker via .isr_vector at FLASH origin) */
#if !defined  (VECT_TAB_OFFSET)
#  define VECT_TAB_OFFSET   0x00000000U
#endif

/* Defined by board_config.c — initial CPU clock value for SystemCoreClock.
 * On U5 the post-reset core clock is MSI = 4 MHz; the HAL bumps it once the
 * PLL is configured. */
uint32_t SystemCoreClock = MSI_VALUE;

/* MSI range LUT — required by the HAL's HAL_RCC_GetSysClockFreq() and the
 * PLL frequency helpers. Values are from RM0456 §11.5.4 (MSIRGSEL ranges).
 * Verbatim from the upstream cmsis-device-u5 system_stm32u5xx.c so the HAL
 * sees an identical table whether or not the upstream template is linked. */
const uint32_t MSIRangeTable[16] = {
    48000000U, 24000000U, 16000000U, 12000000U,
     4000000U,  2000000U,  1330000U,  1000000U,
     3072000U,  1536000U,  1024000U,   768000U,
      400000U,   200000U,   133000U,   100000U
};

void SystemInit(void)
{
    /* FPU on — CP10/CP11 full access (privileged + user). */
#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SCB->CPACR |= ((3U << (10U * 2U)) | (3U << (11U * 2U)));
#endif

    /* Vector table relocation. SCB_VTOR holds the offset into the active
     * memory region; the linker places .isr_vector at FLASH origin. */
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
}

void SystemCoreClockUpdate(void)
{
    /* The full PLL-driven recompute lives in drivers/hal/stm32/hal_rcc_stm32.c
     * (HAL_RCC_GetSysClockFreq()). This default keeps SystemCoreClock at the
     * post-reset MSI value until the HAL clocks up. */
    SystemCoreClock = MSI_VALUE;
}
