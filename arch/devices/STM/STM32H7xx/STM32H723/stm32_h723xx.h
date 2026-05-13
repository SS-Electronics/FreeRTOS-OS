/**
 ******************************************************************************
 * @file    stm32_h723xx.h
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   CMSIS STM32H723xx Device Peripheral Access Layer Header.
 *
 *          Contains:
 *            - Cortex-M7 core configuration macros
 *            - IRQn_Type enum (RM0468 Table 122 — position column)
 *            - Peripheral register TypeDefs
 *            - Peripheral base addresses and instance pointer macros
 *
 *          Reference: STM32H723/733/725/735/730 Reference Manual RM0468 Rev 4.
 *
 *          Peripheral register structures list only the fields used by the
 *          OS HAL layer.  For the full bit-field expansion use ST's official
 *          CMSIS pack (STM32CubeH7 v1.11+).
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __STM32_H723XX_H
#define __STM32_H723XX_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── 1. Cortex-M7 Core Configuration ─────────────────────────────────────── */
/* STM32H723 uses Cortex-M7 r1p1.  FreeRTOS ARM_CM7/r0p1 port targets all
 * CM7 silicon — including r1p1.  The r0p1 name reflects the errata the port
 * works around, not a silicon revision requirement.                          */
#define __CM7_REV                 0x0101U  /*!< Core revision r1p1           */
#define __MPU_PRESENT             1U       /*!< MPU present                  */
#define __NVIC_PRIO_BITS          4U       /*!< 4-bit priority (16 levels)   */
#define __Vendor_SysTickConfig    0U       /*!< Use the standard SysTick     */
#define __FPU_PRESENT             1U       /*!< Cortex-M7 DP-FPU             */
#define __DCACHE_PRESENT          1U       /*!< D-Cache present              */
#define __ICACHE_PRESENT          1U       /*!< I-Cache present              */
#define __DTCM_PRESENT            1U       /*!< DTCM present                 */

/* ── 2. Interrupt Number Definitions ─────────────────────────────────────── */
/* RM0468 Table 122: STM32H723/733 vector table.                              */
typedef enum
{
    /* ── Cortex-M7 core exceptions (negative numbers) ─────────────────────── */
    NonMaskableInt_IRQn     = -14,
    HardFault_IRQn          = -13,
    MemoryManagement_IRQn   = -12,
    BusFault_IRQn           = -11,
    UsageFault_IRQn         = -10,
    SVCall_IRQn             = -5,
    DebugMonitor_IRQn       = -4,
    PendSV_IRQn             = -2,
    SysTick_IRQn            = -1,

    /* ── STM32H723 peripheral interrupts (RM0468 §23.3 Table 122) ─────────── */
    WWDG_IRQn                        = 0,
    PVD_AVD_IRQn                     = 1,
    TAMP_STAMP_RTC_WKUP_LSECSS_IRQn = 2,
    RTC_WKUP_IRQn                    = 3,
    FLASH_IRQn                       = 4,
    RCC_IRQn                         = 5,
    EXTI0_IRQn                       = 6,
    EXTI1_IRQn                       = 7,
    EXTI2_IRQn                       = 8,
    EXTI3_IRQn                       = 9,
    EXTI4_IRQn                       = 10,
    DMA1_Stream0_IRQn                = 11,
    DMA1_Stream1_IRQn                = 12,
    DMA1_Stream2_IRQn                = 13,
    DMA1_Stream3_IRQn                = 14,
    DMA1_Stream4_IRQn                = 15,
    DMA1_Stream5_IRQn                = 16,
    DMA1_Stream6_IRQn                = 17,
    ADC_IRQn                         = 18,
    FDCAN1_IT0_IRQn                  = 19,
    FDCAN2_IT0_IRQn                  = 20,
    FDCAN1_IT1_IRQn                  = 21,
    FDCAN2_IT1_IRQn                  = 22,
    EXTI9_5_IRQn                     = 23,
    TIM1_BRK_IRQn                    = 24,
    TIM1_UP_IRQn                     = 25,
    TIM1_TRG_COM_IRQn                = 26,
    TIM1_CC_IRQn                     = 27,
    TIM2_IRQn                        = 28,
    TIM3_IRQn                        = 29,
    TIM4_IRQn                        = 30,
    I2C1_EV_IRQn                     = 31,
    I2C1_ER_IRQn                     = 32,
    I2C2_EV_IRQn                     = 33,
    I2C2_ER_IRQn                     = 34,
    SPI1_IRQn                        = 35,
    SPI2_IRQn                        = 36,
    USART1_IRQn                      = 37,
    USART2_IRQn                      = 38,
    USART3_IRQn                      = 39,
    EXTI15_10_IRQn                   = 40,
    RTC_Alarm_IRQn                   = 41,
    TIM8_BRK_TIM12_IRQn             = 43,
    TIM8_UP_TIM13_IRQn              = 44,
    TIM8_TRG_COM_TIM14_IRQn        = 45,
    TIM8_CC_IRQn                    = 46,
    DMA1_Stream7_IRQn               = 47,
    FMC_IRQn                        = 48,
    SDMMC1_IRQn                     = 49,
    TIM5_IRQn                       = 50,
    SPI3_IRQn                       = 51,
    UART4_IRQn                      = 52,
    UART5_IRQn                      = 53,
    TIM6_DAC_IRQn                   = 54,
    TIM7_IRQn                       = 55,
    DMA2_Stream0_IRQn               = 56,
    DMA2_Stream1_IRQn               = 57,
    DMA2_Stream2_IRQn               = 58,
    DMA2_Stream3_IRQn               = 59,
    DMA2_Stream4_IRQn               = 60,
    ETH_IRQn                        = 61,
    ETH_WKUP_IRQn                   = 62,
    FDCAN_CAL_IRQn                  = 63,
    DMA2_Stream5_IRQn               = 68,
    DMA2_Stream6_IRQn               = 69,
    DMA2_Stream7_IRQn               = 70,
    USART6_IRQn                     = 71,
    I2C3_EV_IRQn                    = 72,
    I2C3_ER_IRQn                    = 73,
    OTG_HS_EP1_OUT_IRQn             = 74,
    OTG_HS_EP1_IN_IRQn              = 75,
    OTG_HS_WKUP_IRQn               = 76,
    OTG_HS_IRQn                     = 77,
    DCMI_PSSI_IRQn                  = 78,
    CRYP_IRQn                       = 79,
    HASH_RNG_IRQn                   = 80,
    FPU_IRQn                        = 81,
    UART7_IRQn                      = 82,
    UART8_IRQn                      = 83,
    SPI4_IRQn                       = 84,
    SPI5_IRQn                       = 85,
    SPI6_IRQn                       = 86,
    SAI1_IRQn                       = 87,
    LTDC_IRQn                       = 88,
    LTDC_ER_IRQn                    = 89,
    DMA2D_IRQn                      = 90,
    OCTOSPI1_IRQn                   = 92,
    LPTIM1_IRQn                     = 93,
    CEC_IRQn                        = 94,
    I2C4_EV_IRQn                    = 95,
    I2C4_ER_IRQn                    = 96,
    SPDIF_RX_IRQn                   = 97,
    OTG_FS_EP1_OUT_IRQn             = 98,
    OTG_FS_EP1_IN_IRQn              = 99,
    OTG_FS_WKUP_IRQn               = 100,
    OTG_FS_IRQn                     = 101,
    DMAMUX1_OVR_IRQn               = 102,
    HRTIM1_Master_IRQn             = 103,
    HRTIM1_TIMA_IRQn               = 104,
    HRTIM1_TIMB_IRQn               = 105,
    HRTIM1_TIMC_IRQn               = 106,
    HRTIM1_TIMD_IRQn               = 107,
    HRTIM1_TIME_IRQn               = 108,
    HRTIM1_FLT_IRQn                = 109,
    DFSDM1_FLT0_IRQn               = 110,
    DFSDM1_FLT1_IRQn               = 111,
    DFSDM1_FLT2_IRQn               = 112,
    DFSDM1_FLT3_IRQn               = 113,
    SAI3_IRQn                       = 114,
    SWPMI1_IRQn                     = 115,
    TIM15_IRQn                      = 116,
    TIM16_IRQn                      = 117,
    TIM17_IRQn                      = 118,
    MDIOS_WKUP_IRQn                = 119,
    MDIOS_IRQn                      = 120,
    JPEG_IRQn                       = 121,
    MDMA_IRQn                       = 122,
    SDMMC2_IRQn                     = 124,
    HSEM1_IRQn                      = 125,
    ADC3_IRQn                       = 127,
    DMAMUX2_OVR_IRQn               = 128,
    BDMA_Channel0_IRQn             = 129,
    BDMA_Channel1_IRQn             = 130,
    BDMA_Channel2_IRQn             = 131,
    BDMA_Channel3_IRQn             = 132,
    BDMA_Channel4_IRQn             = 133,
    BDMA_Channel5_IRQn             = 134,
    BDMA_Channel6_IRQn             = 135,
    BDMA_Channel7_IRQn             = 136,
    COMP1_IRQn                      = 137,
    LPTIM2_IRQn                     = 138,
    LPTIM3_IRQn                     = 139,
    LPTIM4_IRQn                     = 140,
    LPTIM5_IRQn                     = 141,
    LPUART1_IRQn                    = 142,
    CRS_IRQn                        = 144,
    ECC_IRQn                        = 145,
    SAI4_IRQn                       = 146,
    HOLD_CORE_IRQn                  = 148,
    WAKEUP_PIN_IRQn                 = 149,
    CORDIC_IRQn                     = 150,
    FMAC_IRQn                       = 151,
} IRQn_Type;

/* CMSIS core — IRQn_Type must be defined before this include */
#include "CMSIS/Core/Include/core_cm7.h"  /* resolved via -I arch/arm/CMSIS_6 */

/* ── 3. Peripheral Register TypeDefs ─────────────────────────────────────── */
/* Only fields directly accessed by FreeRTOS-OS HAL drivers are listed.
 * Reserved padding keeps offsets correct per RM0468 register maps.          */

typedef struct
{
    __IO uint32_t MODER;    /*!< 0x00 Mode register               */
    __IO uint32_t OTYPER;   /*!< 0x04 Output type register        */
    __IO uint32_t OSPEEDR;  /*!< 0x08 Output speed register       */
    __IO uint32_t PUPDR;    /*!< 0x0C Pull-up/pull-down register  */
    __IO uint32_t IDR;      /*!< 0x10 Input data register         */
    __IO uint32_t ODR;      /*!< 0x14 Output data register        */
    __IO uint32_t BSRR;     /*!< 0x18 Bit set/reset register      */
    __IO uint32_t LCKR;     /*!< 0x1C Configuration lock register */
    __IO uint32_t AFR[2];   /*!< 0x20 Alternate function [0]=low [1]=high */
} GPIO_TypeDef;

typedef struct
{
    __IO uint32_t CR1;      /*!< 0x00 Control register 1  */
    __IO uint32_t CR2;      /*!< 0x04 Control register 2  */
    __IO uint32_t CR3;      /*!< 0x08 Control register 3  */
    __IO uint32_t BRR;      /*!< 0x0C Baud rate register  */
    __IO uint32_t GTPR;     /*!< 0x10 Guard time / prescaler */
    __IO uint32_t RTOR;     /*!< 0x14 Receiver timeout    */
    __IO uint32_t RQR;      /*!< 0x18 Request register    */
    __IO uint32_t ISR;      /*!< 0x1C Interrupt & status  */
    __IO uint32_t ICR;      /*!< 0x20 Interrupt clear register */
    __IO uint32_t RDR;      /*!< 0x24 Receive data register   */
    __IO uint32_t TDR;      /*!< 0x28 Transmit data register  */
    __IO uint32_t PRESC;    /*!< 0x2C Prescaler register      */
} USART_TypeDef;

/* H7 UART is register-compatible with USART */
#define UART_TypeDef  USART_TypeDef

typedef struct
{
    __IO uint32_t CR1;      /*!< 0x00 Control register 1   */
    __IO uint32_t CR2;      /*!< 0x04 Control register 2   */
    __IO uint32_t OAR1;     /*!< 0x08 Own address 1        */
    __IO uint32_t OAR2;     /*!< 0x0C Own address 2        */
    __IO uint32_t TIMINGR;  /*!< 0x10 Timing register      */
    __IO uint32_t TIMEOUTR; /*!< 0x14 Timeout register     */
    __IO uint32_t ISR;      /*!< 0x18 Interrupt & status   */
    __IO uint32_t ICR;      /*!< 0x1C Interrupt clear      */
    __IO uint32_t PECR;     /*!< 0x20 PEC register         */
    __IO uint32_t RXDR;     /*!< 0x24 Receive data         */
    __IO uint32_t TXDR;     /*!< 0x28 Transmit data        */
} I2C_TypeDef;

typedef struct
{
    __IO uint32_t CR1;      /*!< 0x00 Control register 1  */
    __IO uint32_t CR2;      /*!< 0x04 Control register 2  */
    __IO uint32_t SR;       /*!< 0x08 Status register     */
    __IO uint32_t IFCR;     /*!< 0x0C Interrupt/status clear */
    __IO uint32_t AUTOCR;   /*!< 0x10 Autonomous mode control */
    __IO uint32_t CFG1;     /*!< 0x14 Configuration register 1 */
    __IO uint32_t CFG2;     /*!< 0x18 Configuration register 2 */
    __IO uint32_t IER;      /*!< 0x1C Interrupt enable    */
    __IO uint32_t UDRDR;    /*!< 0x20 Underrun data register */
    __IO uint32_t RXDR;     /*!< 0x24 RX data register (only on SPI3) */
    __IO uint32_t TXDR;     /*!< 0x28 TX data register    */
} SPI_TypeDef;

/* RCC — only fields used by clock init (SystemInit / hal_rcc_stm32.c) */
typedef struct
{
    __IO uint32_t CR;               /*!< 0x000 Clock control register                */
    __IO uint32_t HSICFGR;         /*!< 0x004 HSI calibration                       */
    __IO uint32_t CRRCR;           /*!< 0x008 CSI calibration / RC48 calibration    */
    __IO uint32_t CSICFGR;         /*!< 0x00C CSI calibration                       */
    __IO uint32_t CFGR;            /*!< 0x010 Clock configuration                   */
         uint32_t RESERVED0;       /*!< 0x014                                        */
    __IO uint32_t D1CFGR;          /*!< 0x018 D1 domain clock configuration         */
    __IO uint32_t D2CFGR;          /*!< 0x01C D2 domain clock configuration         */
    __IO uint32_t D3CFGR;          /*!< 0x020 D3 domain clock configuration         */
         uint32_t RESERVED1;       /*!< 0x024                                        */
    __IO uint32_t PLLCKSELR;       /*!< 0x028 PLLs clock source                     */
    __IO uint32_t PLLCFGR;         /*!< 0x02C PLLs configuration                    */
    __IO uint32_t PLL1DIVR;        /*!< 0x030 PLL1 dividers                         */
    __IO uint32_t PLL1FRACR;       /*!< 0x034 PLL1 fractional divider               */
    __IO uint32_t PLL2DIVR;        /*!< 0x038 PLL2 dividers                         */
    __IO uint32_t PLL2FRACR;       /*!< 0x03C PLL2 fractional divider               */
    __IO uint32_t PLL3DIVR;        /*!< 0x040 PLL3 dividers                         */
    __IO uint32_t PLL3FRACR;       /*!< 0x044 PLL3 fractional divider               */
         uint32_t RESERVED2;       /*!< 0x048                                        */
    __IO uint32_t D1CCIPR;         /*!< 0x04C D1 kernel clock prescalers            */
    __IO uint32_t D2CCIP1R;        /*!< 0x050 D2 kernel clock prescalers 1          */
    __IO uint32_t D2CCIP2R;        /*!< 0x054 D2 kernel clock prescalers 2          */
    __IO uint32_t D3CCIPR;         /*!< 0x058 D3 kernel clock prescalers            */
         uint32_t RESERVED3;       /*!< 0x05C                                        */
    __IO uint32_t CIER;            /*!< 0x060 Clock interrupt enable                */
    __IO uint32_t CIFR;            /*!< 0x064 Clock interrupt flag                  */
    __IO uint32_t CICR;            /*!< 0x068 Clock interrupt clear                 */
         uint32_t RESERVED4;       /*!< 0x06C                                        */
    __IO uint32_t BDCR;            /*!< 0x070 Backup domain control                 */
    __IO uint32_t CSR;             /*!< 0x074 Clock control & status                */
         uint32_t RESERVED5[2];    /*!< 0x078-0x07C                                 */
    __IO uint32_t AHB3RSTR;        /*!< 0x07C AHB3 peripheral reset                 */
    __IO uint32_t AHB1RSTR;        /*!< 0x080 AHB1 peripheral reset                 */
    __IO uint32_t AHB2RSTR;        /*!< 0x084 AHB2 peripheral reset                 */
    __IO uint32_t AHB4RSTR;        /*!< 0x088 AHB4 peripheral reset                 */
    __IO uint32_t APB3RSTR;        /*!< 0x08C APB3 peripheral reset                 */
    __IO uint32_t APB1LRSTR;       /*!< 0x090 APB1L peripheral reset                */
    __IO uint32_t APB1HRSTR;       /*!< 0x094 APB1H peripheral reset                */
    __IO uint32_t APB2RSTR;        /*!< 0x098 APB2 peripheral reset                 */
    __IO uint32_t APB4RSTR;        /*!< 0x09C APB4 peripheral reset                 */
    __IO uint32_t GCR;             /*!< 0x0A0 Global control register               */
         uint32_t RESERVED6;       /*!< 0x0A4                                        */
    __IO uint32_t D3AMR;           /*!< 0x0A8 D3 autonomous mode register           */
         uint32_t RESERVED7[9];    /*!< 0x0AC-0x0CC                                 */
    __IO uint32_t RSR;             /*!< 0x0D0 Reset status register                 */
    __IO uint32_t AHB3ENR;         /*!< 0x0D4 AHB3 clock enable                     */
    __IO uint32_t AHB1ENR;         /*!< 0x0D8 AHB1 clock enable                     */
    __IO uint32_t AHB2ENR;         /*!< 0x0DC AHB2 clock enable                     */
    __IO uint32_t AHB4ENR;         /*!< 0x0E0 AHB4 clock enable (GPIO, CRC, BDMA …) */
    __IO uint32_t APB3ENR;         /*!< 0x0E4 APB3 clock enable                     */
    __IO uint32_t APB1LENR;        /*!< 0x0E8 APB1L clock enable (TIM2..7, SPI2/3, USART2/3, UART4..8 …) */
    __IO uint32_t APB1HENR;        /*!< 0x0EC APB1H clock enable                    */
    __IO uint32_t APB2ENR;         /*!< 0x0F0 APB2 clock enable (TIM1/8, USART1/6, SPI1/4/5 …)         */
    __IO uint32_t APB4ENR;         /*!< 0x0F4 APB4 clock enable (SYSCFG, I2C4, LPUART1 …)              */
} RCC_TypeDef;

/* PWR — fields used by SystemInit (VOS scaling) */
typedef struct
{
    __IO uint32_t CR1;    /*!< 0x00 Power control register 1   */
    __IO uint32_t CSR1;   /*!< 0x04 Power control/status 1     */
    __IO uint32_t CR2;    /*!< 0x08 Power control register 2   */
    __IO uint32_t CR3;    /*!< 0x0C Power control register 3   */
    __IO uint32_t CPUCR;  /*!< 0x10 CPU control register       */
         uint32_t RSRVD;  /*!< 0x14 Reserved                   */
    __IO uint32_t D3CR;   /*!< 0x18 D3 domain control register */
         uint32_t RSRVD2; /*!< 0x1C Reserved                   */
    __IO uint32_t WKUPCR; /*!< 0x20 Wakeup clear register      */
    __IO uint32_t WKUPFR; /*!< 0x24 Wakeup flag register       */
    __IO uint32_t WKUPEPR;/*!< 0x28 Wakeup enable/polarity     */
} PWR_TypeDef;

/* FLASH interface (used by HAL_FLASH for programming and latency) */
typedef struct
{
    __IO uint32_t ACR;          /*!< 0x000 Flash access control (latency, caches) */
         uint32_t RESERVED0;    /*!< 0x004                                         */
    __IO uint32_t OPTKEYR;      /*!< 0x008 Option byte key                        */
    __IO uint32_t CR;           /*!< 0x00C Control register                       */
    __IO uint32_t SR;           /*!< 0x010 Status register                        */
    __IO uint32_t CCR;          /*!< 0x014 Clear control register                 */
    __IO uint32_t OPTCR;        /*!< 0x018 Option control register                */
    __IO uint32_t OPTSR_CUR;    /*!< 0x01C Option status current                  */
    __IO uint32_t OPTSR_PRG;    /*!< 0x020 Option status programmed               */
    __IO uint32_t OPTCCR;       /*!< 0x024 Option clear control register          */
} FLASH_TypeDef;

/* SYSCFG — used by HAL for EXTI configuration */
typedef struct
{
    __IO uint32_t PMCR;         /*!< 0x04 Peripheral mode configuration  */
    __IO uint32_t EXTICR[4];    /*!< 0x08 External interrupt config regs */
    __IO uint32_t CFGR;         /*!< 0x18 Configuration register         */
         uint32_t RESERVED0;    /*!< 0x1C                                 */
    __IO uint32_t CCCSR;        /*!< 0x20 Compensation cell ctrl/status  */
    __IO uint32_t CCVR;         /*!< 0x24 Compensation cell value        */
    __IO uint32_t CCCR;         /*!< 0x28 Compensation cell code         */
    __IO uint32_t PWRCR;        /*!< 0x2C Power control                  */
         uint32_t RESERVED1[61];
    __IO uint32_t PKGR;         /*!< 0x124 Package register              */
         uint32_t RESERVED2[118];
    __IO uint32_t UR0;          /*!< 0x300 User register 0               */
} SYSCFG_TypeDef;

/* EXTI — used by HAL GPIO for external interrupt configuration */
typedef struct
{
    __IO uint32_t RTSR1;        /*!< 0x00 Rising trigger select 1   */
    __IO uint32_t FTSR1;        /*!< 0x04 Falling trigger select 1  */
    __IO uint32_t SWIER1;       /*!< 0x08 Software interrupt event 1 */
    __IO uint32_t D3PMR1;       /*!< 0x0C D3 pend mask 1            */
    __IO uint32_t D3PCR1L;      /*!< 0x10 D3 pend clear reg 1L      */
    __IO uint32_t D3PCR1H;      /*!< 0x14 D3 pend clear reg 1H      */
         uint32_t RESERVED0[2]; /*!< 0x18-0x1C                       */
    __IO uint32_t RTSR2;        /*!< 0x20 Rising trigger select 2   */
    __IO uint32_t FTSR2;        /*!< 0x24 Falling trigger select 2  */
    __IO uint32_t SWIER2;       /*!< 0x28 Software interrupt event 2 */
    __IO uint32_t D3PMR2;       /*!< 0x2C D3 pend mask 2            */
    __IO uint32_t D3PCR2L;      /*!< 0x30 D3 pend clear reg 2L      */
    __IO uint32_t D3PCR2H;      /*!< 0x34 D3 pend clear reg 2H      */
         uint32_t RESERVED1[2]; /*!< 0x38-0x3C                       */
    __IO uint32_t RTSR3;        /*!< 0x40 Rising trigger select 3   */
    __IO uint32_t FTSR3;        /*!< 0x44 Falling trigger select 3  */
    __IO uint32_t SWIER3;       /*!< 0x48 Software interrupt event 3 */
    __IO uint32_t D3PMR3;       /*!< 0x4C D3 pend mask 3            */
    __IO uint32_t D3PCR3L;      /*!< 0x50 D3 pend clear reg 3L      */
    __IO uint32_t D3PCR3H;      /*!< 0x54 D3 pend clear reg 3H      */
         uint32_t RESERVED2[10]; /* 0x58-0x7C                        */
    __IO uint32_t IMR1;         /*!< 0x80 CPU1 interrupt mask 1      */
    __IO uint32_t EMR1;         /*!< 0x84 CPU1 event mask 1          */
    __IO uint32_t PR1;          /*!< 0x88 CPU1 pending register 1    */
         uint32_t RESERVED3;
    __IO uint32_t IMR2;         /*!< 0x90 CPU1 interrupt mask 2      */
    __IO uint32_t EMR2;         /*!< 0x94 CPU1 event mask 2          */
    __IO uint32_t PR2;          /*!< 0x98 CPU1 pending register 2    */
         uint32_t RESERVED4;
    __IO uint32_t IMR3;         /*!< 0xA0 CPU1 interrupt mask 3      */
    __IO uint32_t EMR3;         /*!< 0xA4 CPU1 event mask 3          */
    __IO uint32_t PR3;          /*!< 0xA8 CPU1 pending register 3    */
} EXTI_TypeDef;

/* TIM — only the CR1/CR2/ARR/PSC fields used by HAL_TIM timebase */
typedef struct
{
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    __IO uint32_t RCR;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
    __IO uint32_t BDTR;
    __IO uint32_t DCR;
    __IO uint32_t DMAR;
} TIM_TypeDef;

/* IWDG */
typedef struct
{
    __IO uint32_t KR;
    __IO uint32_t PR;
    __IO uint32_t RLR;
    __IO uint32_t SR;
    __IO uint32_t WINR;
} IWDG_TypeDef;

/* ── 4. Peripheral Base Addresses (RM0468 §3 Memory Map) ────────────────── */

/* AHB4 — GPIO and RCC are on the D3 power domain AHB4 bus at 0x58000000    */
#define D3_AHB4PERIPH_BASE    0x58020000UL
#define GPIOA_BASE            (D3_AHB4PERIPH_BASE + 0x0000UL)  /*!< 0x58020000 */
#define GPIOB_BASE            (D3_AHB4PERIPH_BASE + 0x0400UL)  /*!< 0x58020400 */
#define GPIOC_BASE            (D3_AHB4PERIPH_BASE + 0x0800UL)  /*!< 0x58020800 */
#define GPIOD_BASE            (D3_AHB4PERIPH_BASE + 0x0C00UL)  /*!< 0x58020C00 */
#define GPIOE_BASE            (D3_AHB4PERIPH_BASE + 0x1000UL)  /*!< 0x58021000 */
#define GPIOF_BASE            (D3_AHB4PERIPH_BASE + 0x1400UL)  /*!< 0x58021400 */
#define GPIOG_BASE            (D3_AHB4PERIPH_BASE + 0x1800UL)  /*!< 0x58021800 */
#define GPIOH_BASE            (D3_AHB4PERIPH_BASE + 0x1C00UL)  /*!< 0x58021C00 */

#define RCC_BASE              0x58024400UL
#define PWR_BASE              0x58024800UL

/* APB4 */
#define EXTI_BASE             0x58000000UL
#define SYSCFG_BASE           0x58000400UL

/* Flash interface (AHB3 / D1 domain — differs from F4 location) */
#define FLASH_R_BASE          0x52002000UL

/* APB1L — USART2, USART3, UART4, UART5, UART7, UART8, I2C1..3, SPI2/3 */
#define APB1PERIPH_BASE       0x40000000UL
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400UL)  /*!< 0x40004400 */
#define USART3_BASE           (APB1PERIPH_BASE + 0x4800UL)  /*!< 0x40004800 */
#define UART4_BASE            (APB1PERIPH_BASE + 0x4C00UL)  /*!< 0x40004C00 */
#define UART5_BASE            (APB1PERIPH_BASE + 0x5000UL)  /*!< 0x40005000 */
#define I2C1_BASE             (APB1PERIPH_BASE + 0x5400UL)  /*!< 0x40005400 */
#define I2C2_BASE             (APB1PERIPH_BASE + 0x5800UL)  /*!< 0x40005800 */
#define I2C3_BASE             (APB1PERIPH_BASE + 0x5C00UL)  /*!< 0x40005C00 */
#define UART7_BASE            (APB1PERIPH_BASE + 0x7800UL)  /*!< 0x40007800 */
#define UART8_BASE            (APB1PERIPH_BASE + 0x7C00UL)  /*!< 0x40007C00 */
#define SPI2_BASE             (APB1PERIPH_BASE + 0x3800UL)  /*!< 0x40003800 */
#define SPI3_BASE             (APB1PERIPH_BASE + 0x3C00UL)  /*!< 0x40003C00 */
#define TIM2_BASE             (APB1PERIPH_BASE + 0x0000UL)  /*!< 0x40000000 */
#define TIM3_BASE             (APB1PERIPH_BASE + 0x0400UL)  /*!< 0x40000400 */
#define TIM4_BASE             (APB1PERIPH_BASE + 0x0800UL)  /*!< 0x40000800 */
#define TIM5_BASE             (APB1PERIPH_BASE + 0x0C00UL)  /*!< 0x40000C00 */
#define TIM6_BASE             (APB1PERIPH_BASE + 0x1000UL)  /*!< 0x40001000 */
#define TIM7_BASE             (APB1PERIPH_BASE + 0x1400UL)  /*!< 0x40001400 */

/* APB2 — USART1, USART6, SPI1/4/5, TIM1/8 */
#define APB2PERIPH_BASE       0x40010000UL
#define TIM1_BASE             (APB2PERIPH_BASE + 0x0000UL)  /*!< 0x40010000 */
#define TIM8_BASE             (APB2PERIPH_BASE + 0x0400UL)  /*!< 0x40010400 */
#define USART1_BASE           (APB2PERIPH_BASE + 0x1000UL)  /*!< 0x40011000 */
#define USART6_BASE           (APB2PERIPH_BASE + 0x1400UL)  /*!< 0x40011400 */
#define SPI1_BASE             (APB2PERIPH_BASE + 0x3000UL)  /*!< 0x40013000 */
#define SPI4_BASE             (APB2PERIPH_BASE + 0x3400UL)  /*!< 0x40013400 */
#define TIM15_BASE            (APB2PERIPH_BASE + 0x4000UL)  /*!< 0x40014000 */
#define TIM16_BASE            (APB2PERIPH_BASE + 0x4400UL)  /*!< 0x40014400 */
#define TIM17_BASE            (APB2PERIPH_BASE + 0x4800UL)  /*!< 0x40014800 */
#define SPI5_BASE             (APB2PERIPH_BASE + 0x5000UL)  /*!< 0x40015000 */

/* APB4 — I2C4 */
#define APB4PERIPH_BASE       0x58000000UL
#define I2C4_BASE             (APB4PERIPH_BASE + 0x1C00UL)  /*!< 0x58001C00 */
#define SPI6_BASE             (APB4PERIPH_BASE + 0x1400UL)  /*!< 0x58001400 */

/* IWDG1 */
#define IWDG1_BASE            0x58004800UL

/* ── 5. Peripheral Instance Pointers ─────────────────────────────────────── */
#define GPIOA    ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB    ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC    ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD    ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE    ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF    ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG    ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH    ((GPIO_TypeDef *) GPIOH_BASE)

#define USART1   ((USART_TypeDef *) USART1_BASE)
#define USART2   ((USART_TypeDef *) USART2_BASE)
#define USART3   ((USART_TypeDef *) USART3_BASE)
#define UART4    ((USART_TypeDef *) UART4_BASE)
#define UART5    ((USART_TypeDef *) UART5_BASE)
#define USART6   ((USART_TypeDef *) USART6_BASE)
#define UART7    ((USART_TypeDef *) UART7_BASE)
#define UART8    ((USART_TypeDef *) UART8_BASE)

#define I2C1     ((I2C_TypeDef *) I2C1_BASE)
#define I2C2     ((I2C_TypeDef *) I2C2_BASE)
#define I2C3     ((I2C_TypeDef *) I2C3_BASE)
#define I2C4     ((I2C_TypeDef *) I2C4_BASE)

#define SPI1     ((SPI_TypeDef *) SPI1_BASE)
#define SPI2     ((SPI_TypeDef *) SPI2_BASE)
#define SPI3     ((SPI_TypeDef *) SPI3_BASE)
#define SPI4     ((SPI_TypeDef *) SPI4_BASE)
#define SPI5     ((SPI_TypeDef *) SPI5_BASE)
#define SPI6     ((SPI_TypeDef *) SPI6_BASE)

#define TIM1     ((TIM_TypeDef *) TIM1_BASE)
#define TIM2     ((TIM_TypeDef *) TIM2_BASE)
#define TIM3     ((TIM_TypeDef *) TIM3_BASE)
#define TIM4     ((TIM_TypeDef *) TIM4_BASE)
#define TIM5     ((TIM_TypeDef *) TIM5_BASE)
#define TIM6     ((TIM_TypeDef *) TIM6_BASE)
#define TIM7     ((TIM_TypeDef *) TIM7_BASE)
#define TIM8     ((TIM_TypeDef *) TIM8_BASE)
#define TIM15    ((TIM_TypeDef *) TIM15_BASE)
#define TIM16    ((TIM_TypeDef *) TIM16_BASE)
#define TIM17    ((TIM_TypeDef *) TIM17_BASE)

#define RCC      ((RCC_TypeDef *) RCC_BASE)
#define PWR      ((PWR_TypeDef *) PWR_BASE)
#define FLASH    ((FLASH_TypeDef *) FLASH_R_BASE)
#define SYSCFG   ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define EXTI     ((EXTI_TypeDef *) EXTI_BASE)
#define IWDG1    ((IWDG_TypeDef *) IWDG1_BASE)
#define IWDG     IWDG1   /* alias — HAL uses IWDG */

/* ── 6. RCC Bit-field Masks used by __HAL_RCC_xxx_CLK_ENABLE() ──────────── */
/* AHB4ENR — GPIO clock enables (RM0468 §8.7.44) */
#define RCC_AHB4ENR_GPIOAEN_Pos   (0U)
#define RCC_AHB4ENR_GPIOAEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOAEN_Pos)
#define RCC_AHB4ENR_GPIOA         RCC_AHB4ENR_GPIOAEN_Msk
#define RCC_AHB4ENR_GPIOBEN_Pos   (1U)
#define RCC_AHB4ENR_GPIOBEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOBEN_Pos)
#define RCC_AHB4ENR_GPIOB         RCC_AHB4ENR_GPIOBEN_Msk
#define RCC_AHB4ENR_GPIOCEN_Pos   (2U)
#define RCC_AHB4ENR_GPIOCEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOCEN_Pos)
#define RCC_AHB4ENR_GPIOC         RCC_AHB4ENR_GPIOCEN_Msk
#define RCC_AHB4ENR_GPIODEN_Pos   (3U)
#define RCC_AHB4ENR_GPIODEN_Msk   (0x1UL << RCC_AHB4ENR_GPIODEN_Pos)
#define RCC_AHB4ENR_GPIOD         RCC_AHB4ENR_GPIODEN_Msk
#define RCC_AHB4ENR_GPIOEEN_Pos   (4U)
#define RCC_AHB4ENR_GPIOEEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOEEN_Pos)
#define RCC_AHB4ENR_GPIOE         RCC_AHB4ENR_GPIOEEN_Msk
#define RCC_AHB4ENR_GPIOFEN_Pos   (5U)
#define RCC_AHB4ENR_GPIOFEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOFEN_Pos)
#define RCC_AHB4ENR_GPIOF         RCC_AHB4ENR_GPIOFEN_Msk
#define RCC_AHB4ENR_GPIOGEN_Pos   (6U)
#define RCC_AHB4ENR_GPIOGEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOGEN_Pos)
#define RCC_AHB4ENR_GPIOG         RCC_AHB4ENR_GPIOGEN_Msk
#define RCC_AHB4ENR_GPIOHEN_Pos   (7U)
#define RCC_AHB4ENR_GPIOHEN_Msk   (0x1UL << RCC_AHB4ENR_GPIOHEN_Pos)
#define RCC_AHB4ENR_GPIOH         RCC_AHB4ENR_GPIOHEN_Msk

/* APB1LENR — USART2/3, UART4/5/7/8, I2C1..3, SPI2/3 clock enables */
#define RCC_APB1LENR_USART2EN_Pos (17U)
#define RCC_APB1LENR_USART2EN_Msk (0x1UL << RCC_APB1LENR_USART2EN_Pos)
#define RCC_APB1LENR_USART3EN_Pos (18U)
#define RCC_APB1LENR_USART3EN_Msk (0x1UL << RCC_APB1LENR_USART3EN_Pos)
#define RCC_APB1LENR_UART4EN_Pos  (19U)
#define RCC_APB1LENR_UART4EN_Msk  (0x1UL << RCC_APB1LENR_UART4EN_Pos)
#define RCC_APB1LENR_UART5EN_Pos  (20U)
#define RCC_APB1LENR_UART5EN_Msk  (0x1UL << RCC_APB1LENR_UART5EN_Pos)
#define RCC_APB1LENR_I2C1EN_Pos   (21U)
#define RCC_APB1LENR_I2C1EN_Msk   (0x1UL << RCC_APB1LENR_I2C1EN_Pos)
#define RCC_APB1LENR_I2C2EN_Pos   (22U)
#define RCC_APB1LENR_I2C2EN_Msk   (0x1UL << RCC_APB1LENR_I2C2EN_Pos)
#define RCC_APB1LENR_I2C3EN_Pos   (23U)
#define RCC_APB1LENR_I2C3EN_Msk   (0x1UL << RCC_APB1LENR_I2C3EN_Pos)
#define RCC_APB1LENR_UART7EN_Pos  (30U)
#define RCC_APB1LENR_UART7EN_Msk  (0x1UL << RCC_APB1LENR_UART7EN_Pos)
#define RCC_APB1LENR_UART8EN_Pos  (31U)
#define RCC_APB1LENR_UART8EN_Msk  (0x1UL << RCC_APB1LENR_UART8EN_Pos)
#define RCC_APB1LENR_SPI2EN_Pos   (14U)
#define RCC_APB1LENR_SPI2EN_Msk   (0x1UL << RCC_APB1LENR_SPI2EN_Pos)
#define RCC_APB1LENR_SPI3EN_Pos   (15U)
#define RCC_APB1LENR_SPI3EN_Msk   (0x1UL << RCC_APB1LENR_SPI3EN_Pos)

/* APB2ENR — USART1/6, SPI1/4/5, TIM1/8 clock enables */
#define RCC_APB2ENR_TIM1EN_Pos    (0U)
#define RCC_APB2ENR_TIM1EN_Msk    (0x1UL << RCC_APB2ENR_TIM1EN_Pos)
#define RCC_APB2ENR_TIM8EN_Pos    (1U)
#define RCC_APB2ENR_TIM8EN_Msk    (0x1UL << RCC_APB2ENR_TIM8EN_Pos)
#define RCC_APB2ENR_USART1EN_Pos  (4U)
#define RCC_APB2ENR_USART1EN_Msk  (0x1UL << RCC_APB2ENR_USART1EN_Pos)
#define RCC_APB2ENR_USART6EN_Pos  (5U)
#define RCC_APB2ENR_USART6EN_Msk  (0x1UL << RCC_APB2ENR_USART6EN_Pos)
#define RCC_APB2ENR_SPI1EN_Pos    (12U)
#define RCC_APB2ENR_SPI1EN_Msk    (0x1UL << RCC_APB2ENR_SPI1EN_Pos)
#define RCC_APB2ENR_SPI4EN_Pos    (13U)
#define RCC_APB2ENR_SPI4EN_Msk    (0x1UL << RCC_APB2ENR_SPI4EN_Pos)
#define RCC_APB2ENR_SPI5EN_Pos    (20U)
#define RCC_APB2ENR_SPI5EN_Msk    (0x1UL << RCC_APB2ENR_SPI5EN_Pos)

/* APB4ENR — SYSCFG, I2C4, SPI6 */
#define RCC_APB4ENR_SYSCFGEN_Pos  (1U)
#define RCC_APB4ENR_SYSCFGEN_Msk  (0x1UL << RCC_APB4ENR_SYSCFGEN_Pos)
#define RCC_APB4ENR_I2C4EN_Pos    (7U)
#define RCC_APB4ENR_I2C4EN_Msk    (0x1UL << RCC_APB4ENR_I2C4EN_Pos)
#define RCC_APB4ENR_SPI6EN_Pos    (5U)
#define RCC_APB4ENR_SPI6EN_Msk    (0x1UL << RCC_APB4ENR_SPI6EN_Pos)

/* ── 7. HAL RCC Clock-enable Macros ─────────────────────────────────────── */
/* These replicate the STM32H7xx HAL RCC macros for use when building without
 * the full CubeH7 HAL pack.  They are gated on USE_HAL_DRIVER so the real
 * HAL macros take precedence if present.                                     */
#ifndef __HAL_RCC_GPIOA_CLK_ENABLE
#  define __HAL_RCC_GPIOA_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN_Msk)
#  define __HAL_RCC_GPIOB_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN_Msk)
#  define __HAL_RCC_GPIOC_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN_Msk)
#  define __HAL_RCC_GPIOD_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN_Msk)
#  define __HAL_RCC_GPIOE_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN_Msk)
#  define __HAL_RCC_GPIOF_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOFEN_Msk)
#  define __HAL_RCC_GPIOG_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOGEN_Msk)
#  define __HAL_RCC_GPIOH_CLK_ENABLE()   (RCC->AHB4ENR |= RCC_AHB4ENR_GPIOHEN_Msk)
#  define __HAL_RCC_USART1_CLK_ENABLE()  (RCC->APB2ENR  |= RCC_APB2ENR_USART1EN_Msk)
#  define __HAL_RCC_USART2_CLK_ENABLE()  (RCC->APB1LENR |= RCC_APB1LENR_USART2EN_Msk)
#  define __HAL_RCC_USART3_CLK_ENABLE()  (RCC->APB1LENR |= RCC_APB1LENR_USART3EN_Msk)
#  define __HAL_RCC_UART4_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_UART4EN_Msk)
#  define __HAL_RCC_UART5_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_UART5EN_Msk)
#  define __HAL_RCC_USART6_CLK_ENABLE()  (RCC->APB2ENR  |= RCC_APB2ENR_USART6EN_Msk)
#  define __HAL_RCC_UART7_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_UART7EN_Msk)
#  define __HAL_RCC_UART8_CLK_ENABLE()   (RCC->APB1LENR |= RCC_APB1LENR_UART8EN_Msk)
#  define __HAL_RCC_I2C1_CLK_ENABLE()    (RCC->APB1LENR |= RCC_APB1LENR_I2C1EN_Msk)
#  define __HAL_RCC_I2C2_CLK_ENABLE()    (RCC->APB1LENR |= RCC_APB1LENR_I2C2EN_Msk)
#  define __HAL_RCC_I2C3_CLK_ENABLE()    (RCC->APB1LENR |= RCC_APB1LENR_I2C3EN_Msk)
#  define __HAL_RCC_I2C4_CLK_ENABLE()    (RCC->APB4ENR  |= RCC_APB4ENR_I2C4EN_Msk)
#  define __HAL_RCC_SPI1_CLK_ENABLE()    (RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN_Msk)
#  define __HAL_RCC_SPI2_CLK_ENABLE()    (RCC->APB1LENR |= RCC_APB1LENR_SPI2EN_Msk)
#  define __HAL_RCC_SPI3_CLK_ENABLE()    (RCC->APB1LENR |= RCC_APB1LENR_SPI3EN_Msk)
#  define __HAL_RCC_SPI4_CLK_ENABLE()    (RCC->APB2ENR  |= RCC_APB2ENR_SPI4EN_Msk)
#  define __HAL_RCC_SPI5_CLK_ENABLE()    (RCC->APB2ENR  |= RCC_APB2ENR_SPI5EN_Msk)
#  define __HAL_RCC_SPI6_CLK_ENABLE()    (RCC->APB4ENR  |= RCC_APB4ENR_SPI6EN_Msk)
#  define __HAL_RCC_SYSCFG_CLK_ENABLE()  (RCC->APB4ENR  |= RCC_APB4ENR_SYSCFGEN_Msk)
#  define __HAL_RCC_PWR_CLK_ENABLE()     do { } while(0)   /* PWR always on in H7 */
#endif

/* ── 8. Flash Latency (FLASH_ACR) ──────────────────────────────────────── */
/* H7 at VOS1 (1.15V, up to 550 MHz): latency = 4 WS (AXI bus @275 MHz)    */
#define FLASH_LATENCY_0    (0x00000000UL)
#define FLASH_LATENCY_1    (0x00000001UL)
#define FLASH_LATENCY_2    (0x00000002UL)
#define FLASH_LATENCY_3    (0x00000003UL)
#define FLASH_LATENCY_4    (0x00000004UL)
#define FLASH_LATENCY_5    (0x00000005UL)
#define FLASH_LATENCY_6    (0x00000006UL)
#define FLASH_LATENCY_7    (0x00000007UL)

/* ── 9. RCC PLL / clock-mux bit-fields ─────────────────────────────────── */
/* Only the values needed by SystemInit. Full definitions live in the HAL.   */
#define RCC_PLLCKSELR_PLLSRC_Pos  (0U)
#define RCC_PLLCKSELR_PLLSRC_HSE (0x02UL << RCC_PLLCKSELR_PLLSRC_Pos)
#define RCC_PLLCKSELR_PLLSRC_HSI (0x00UL)
#define RCC_PLLCKSELR_DIVM1_Pos   (4U)

#define RCC_PLLCFGR_PLL1VCOSEL_Pos  (2U)    /* 0 = wide (192-836 MHz), 1 = medium */
#define RCC_PLLCFGR_PLL1RGE_Pos     (2U)    /* input freq range */
#define RCC_PLLCFGR_PLL1FRACEN_Pos  (0U)

#define RCC_CR_HSEON_Pos   (16U)
#define RCC_CR_HSEON_Msk   (0x1UL << RCC_CR_HSEON_Pos)
#define RCC_CR_HSERDY_Pos  (17U)
#define RCC_CR_HSERDY_Msk  (0x1UL << RCC_CR_HSERDY_Pos)
#define RCC_CR_PLL1ON_Pos  (24U)
#define RCC_CR_PLL1ON_Msk  (0x1UL << RCC_CR_PLL1ON_Pos)
#define RCC_CR_PLL1RDY_Pos (25U)
#define RCC_CR_PLL1RDY_Msk (0x1UL << RCC_CR_PLL1RDY_Pos)

#define RCC_CFGR_SW_Pos    (0U)
#define RCC_CFGR_SW_PLL1   (0x03UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SWS_Pos   (3U)
#define RCC_CFGR_SWS_PLL1  (0x03UL << RCC_CFGR_SWS_Pos)

/* ── 10. PWR bit-fields used by VOS scaling ─────────────────────────────── */
#define PWR_D3CR_VOS_Pos        (14U)
#define PWR_D3CR_VOS_Msk        (0x3UL << PWR_D3CR_VOS_Pos)
#define PWR_D3CR_VOS_SCALE_1    (0x3UL << PWR_D3CR_VOS_Pos)   /*!< VOS1 — highest perf */
#define PWR_D3CR_VOSRDY_Pos     (13U)
#define PWR_D3CR_VOSRDY_Msk     (0x1UL << PWR_D3CR_VOSRDY_Pos)

/* ── 11. FLASH ACR bit-fields ──────────────────────────────────────────── */
#define FLASH_ACR_LATENCY_Pos   (0U)
#define FLASH_ACR_LATENCY_Msk   (0xFUL << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_WRHIGHFREQ_Pos (4U)
#define FLASH_ACR_WRHIGHFREQ_Msk (0x3UL << FLASH_ACR_WRHIGHFREQ_Pos)

/* ── 12. GPIO AF constants (RM0468 Table 10-12) ─────────────────────────── */
/* Alternate function numbers — used by gen_board_config.py as GPIO_AF<n>_<inst> */
#define GPIO_AF7_USART1    (7U)
#define GPIO_AF7_USART2    (7U)
#define GPIO_AF7_USART3    (7U)
#define GPIO_AF8_UART4     (8U)
#define GPIO_AF8_UART5     (8U)
#define GPIO_AF7_USART6    (7U)
#define GPIO_AF7_UART7     (7U)
#define GPIO_AF8_UART8     (8U)
#define GPIO_AF4_I2C1      (4U)
#define GPIO_AF4_I2C2      (4U)
#define GPIO_AF4_I2C3      (4U)
#define GPIO_AF6_I2C4      (6U)
#define GPIO_AF5_SPI1      (5U)
#define GPIO_AF5_SPI2      (5U)
#define GPIO_AF6_SPI3      (6U)
#define GPIO_AF5_SPI4      (5U)
#define GPIO_AF5_SPI5      (5U)
#define GPIO_AF8_SPI6      (8U)

/* GPIO mode/speed/pull constants — same HAL API names as F4 */
#define GPIO_MODE_INPUT               (0x00000000UL)
#define GPIO_MODE_OUTPUT_PP           (0x00000001UL)
#define GPIO_MODE_OUTPUT_OD           (0x00000011UL)
#define GPIO_MODE_AF_PP               (0x00000002UL)
#define GPIO_MODE_AF_OD               (0x00000012UL)
#define GPIO_MODE_ANALOG              (0x00000003UL)
#define GPIO_MODE_IT_RISING           (0x10110000UL)
#define GPIO_MODE_IT_FALLING          (0x10210000UL)
#define GPIO_MODE_IT_RISING_FALLING   (0x10310000UL)

#define GPIO_NOPULL         (0x00000000UL)
#define GPIO_PULLUP         (0x00000001UL)
#define GPIO_PULLDOWN       (0x00000002UL)

#define GPIO_SPEED_FREQ_LOW       (0x00000000UL)
#define GPIO_SPEED_FREQ_MEDIUM    (0x00000001UL)
#define GPIO_SPEED_FREQ_HIGH      (0x00000002UL)
#define GPIO_SPEED_FREQ_VERY_HIGH (0x00000003UL)

#define GPIO_PIN_0   (0x0001U)
#define GPIO_PIN_1   (0x0002U)
#define GPIO_PIN_2   (0x0004U)
#define GPIO_PIN_3   (0x0008U)
#define GPIO_PIN_4   (0x0010U)
#define GPIO_PIN_5   (0x0020U)
#define GPIO_PIN_6   (0x0040U)
#define GPIO_PIN_7   (0x0080U)
#define GPIO_PIN_8   (0x0100U)
#define GPIO_PIN_9   (0x0200U)
#define GPIO_PIN_10  (0x0400U)
#define GPIO_PIN_11  (0x0800U)
#define GPIO_PIN_12  (0x1000U)
#define GPIO_PIN_13  (0x2000U)
#define GPIO_PIN_14  (0x4000U)
#define GPIO_PIN_15  (0x8000U)

#define GPIO_PIN_SET    1U
#define GPIO_PIN_RESET  0U

#ifdef __cplusplus
}
#endif

#endif /* __STM32_H723XX_H */
