/**
 ******************************************************************************
 * @file    stm32u575xx.h
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   CMSIS chip-variant header — STM32U575xx (Cortex-M33 + TrustZone)
 *
 *          ──── STUB ────
 *          This file is a deliberately minimal stub.  The full vendor header
 *          (~30 KLOC of register, IRQn and bit-field definitions) is shipped
 *          by the STM32CubeU5 CMSIS device package.  Drop the upstream file
 *          in place — git submodule below — to replace this stub:
 *
 *             git submodule add \
 *                 https://github.com/STMicroelectronics/cmsis-device-u5.git \
 *                 arch/devices/STM/cmsis-device-u5
 *             ln -sf ../../cmsis-device-u5/Include/stm32u575xx.h \
 *                 arch/devices/STM/STM32U5xx/STM32U575/stm32u575xx.h
 *
 *          What this stub provides:
 *            * Cortex-M33 core selection (__CM33_REV etc.) so core_cm33.h
 *              pulls in correctly via CMSIS-Core (arch/arm/CMSIS_6).
 *            * The IRQn enum entries used by the FreeRTOS-OS startup +
 *              IRQ-table for the example board (USART1, USART3, I2C1, SPI1,
 *              EXTI15_10, TIM6, SysTick / PendSV / SVCall).
 *            * Memory-map base addresses referenced by the linker script.
 *
 *          Once the upstream header is in place the rest of the build
 *          (HAL drivers, board_config.c register access) sees the full
 *          peripheral definitions and the stub macros are masked.
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __STM32U575xx_H
#define __STM32U575xx_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Cortex-M33 configuration (consumed by core_cm33.h) ───────────────────── */

#define __CM33_REV                  0x0000U   /* Core revision r0p0           */
#define __NVIC_PRIO_BITS            4U        /* U5 NVIC priority bits        */
#define __Vendor_SysTickConfig      0U        /* Use CMSIS-Core SysTick_Config*/
#define __VTOR_PRESENT              1U
#define __MPU_PRESENT               1U
#define __FPU_PRESENT               1U
#define __SAUREGION_PRESENT         1U        /* TrustZone SAU regions on M33 */
#define __DSP_PRESENT               1U

/* ── IRQ numbers (subset — full table comes from the upstream CMSIS header).
 *    The values match RM0456 (STM32U5 reference manual) Table 138 so the
 *    HAL_NVIC_SetPriority() calls in irq_hw_init_generated.c are correct.   */

typedef enum IRQn
{
    /* Cortex-M33 system exceptions */
    NonMaskableInt_IRQn         = -14,
    HardFault_IRQn              = -13,
    MemoryManagement_IRQn       = -12,
    BusFault_IRQn               = -11,
    UsageFault_IRQn             = -10,
    SecureFault_IRQn            =  -9,
    SVCall_IRQn                 =  -5,
    DebugMonitor_IRQn           =  -4,
    PendSV_IRQn                 =  -2,
    SysTick_IRQn                =  -1,

    /* STM32U575 device interrupts (subset used by the example board) */
    WWDG_IRQn                   =   0,
    PVD_PVM_IRQn                =   1,
    RTC_IRQn                    =   2,
    FLASH_IRQn                  =   4,
    RCC_IRQn                    =   5,
    EXTI0_IRQn                  =   6,
    EXTI1_IRQn                  =   7,
    EXTI2_IRQn                  =   8,
    EXTI3_IRQn                  =   9,
    EXTI4_IRQn                  =  10,
    EXTI5_IRQn                  =  11,
    EXTI6_IRQn                  =  12,
    EXTI7_IRQn                  =  13,
    EXTI8_IRQn                  =  14,
    EXTI9_IRQn                  =  15,
    EXTI10_IRQn                 =  16,
    EXTI11_IRQn                 =  17,
    EXTI12_IRQn                 =  18,
    EXTI13_IRQn                 =  19,   /* User button on Nucleo-U575ZI-Q   */
    EXTI14_IRQn                 =  20,
    EXTI15_IRQn                 =  21,
    EXTI15_10_IRQn              =  21,   /* alias — full vendor header uses
                                            EXTIxx, our generator emits 10_15 */
    DMA1_Channel1_IRQn          =  25,
    ADC1_IRQn                   =  37,
    TIM6_IRQn                   =  55,   /* HAL timebase on U5 (basic timer) */
    TIM6_DAC_IRQn               =  55,   /* alias — H7 calls it TIM6_DAC     */
    I2C1_EV_IRQn                =  57,
    I2C1_ER_IRQn                =  58,
    SPI1_IRQn                   =  63,
    USART1_IRQn                 =  66,
    USART2_IRQn                 =  67,
    USART3_IRQn                 =  68,
} IRQn_Type;

/* ── Memory map (RM0456 Table 2) ─────────────────────────────────────────── */

#define FLASH_BASE                  (0x08000000UL)
#define SRAM1_BASE                  (0x20000000UL)   /* 192 KB on U575       */
#define SRAM2_BASE                  (0x20030000UL)   /*  64 KB               */
#define SRAM3_BASE                  (0x20040000UL)   /* 512 KB               */
#define SRAM4_BASE                  (0x28000000UL)   /*  16 KB Backup SRAM   */

/* ── CMSIS-Core include — must come AFTER __CM33_REV etc. are visible ──── */

#include "core_cm33.h"

/* When the upstream stm32u575xx.h is dropped in alongside the HAL driver,
 * the #include "stm32u5xx_hal.h" path in stm32u5xx.h pulls the rest of the
 * peripheral definitions. The stub above is enough to satisfy the linker,
 * the startup file, and the IRQ dispatcher. */

#ifdef __cplusplus
}
#endif

#endif /* __STM32U575xx_H */
