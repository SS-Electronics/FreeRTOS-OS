/*
 * hal_rcc_stm32.c — STM32 RCC + CMSIS system init backend
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Replaces system_stm32f4xx.c.  Provides two groups of functionality:
 *
 * 1. Pre-scheduler (called from Reset_Handler before .data/.bss init):
 *      SystemInit()           — FPU enable, optional VTOR relocation
 *      SystemCoreClockUpdate()— read RCC registers → update SystemCoreClock
 *      SystemCoreClock        — CMSIS global (required by STM32 HAL library)
 *      AHBPrescTable[]        — CMSIS prescaler LUT (required by HAL library)
 *      APBPrescTable[]        — CMSIS prescaler LUT (required by HAL library)
 *
 * 2. Post-HAL_Init() (called from main() via drv_rcc_clock_init()):
 *      _stm32_clock_init()    — PLL + bus dividers + flash latency
 *      _stm32_get_sysclk_hz() — return current SYSCLK via HAL query
 *      _stm32_get_apb1_hz()   — return current APB1 via HAL query
 *      _stm32_get_apb2_hz()   — return current APB2 via HAL query
 *
 * Clock tree configured by _stm32_clock_init():
 *   HSI (CONFIG_HSI_VALUE = 16 MHz)
 *     └─ /PLLM (16) ──→ 1 MHz VCO input
 *          └─ ×PLLN (200) ──→ 200 MHz VCO output
 *               ├─ /PLLP (2)  ──→ SYSCLK = 100 MHz
 *               └─ /PLLQ (4)  ──→ USB/SDIO =  50 MHz
 *   AHB  = SYSCLK / 1 = 100 MHz
 *   APB1 = HCLK   / 2 =  50 MHz  (F411 max)
 *   APB2 = HCLK   / 1 = 100 MHz
 *   Flash latency = 3 wait-states
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <stdint.h>
#include <device.h>
#include <def_attributes.h>
#include <def_err.h>
#include <drivers/hal/stm32/hal_rcc_stm32.h>

/* ══════════════════════════════════════════════════════════════════════════
 * CMSIS system variables — required by the STM32 HAL library.
 * Must be in .boot_data because SystemInit() runs before .data is copied.
 * ══════════════════════════════════════════════════════════════════════════ */

__SECTION_BOOT_DATA uint32_t SystemCoreClock = RCC_SYSCLK_HZ;

/* AHB/APB prescaler lookup tables — index maps HPRE/PPRE bitfield values
 * to shift amounts (actual divisor = 1 << table[index]).                   */
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

/* ══════════════════════════════════════════════════════════════════════════
 * SystemInit — called from Reset_Handler before .data/.bss initialisation.
 * Must not access any non-boot RAM (no globals outside .boot_data).
 * ══════════════════════════════════════════════════════════════════════════ */

__SECTION_BOOT void SystemInit(void)
{
    /* Enable FPU coprocessors CP10 and CP11 (full access) */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10U * 2U) | (3UL << 11U * 2U));
#endif

    /* Relocate vector table when explicitly requested via linker/Kconfig */
#if defined(USER_VECT_TAB_ADDRESS)
#if defined(VECT_TAB_SRAM)
    SCB->VTOR = SRAM_BASE | 0x00000000U;
#else
    SCB->VTOR = FLASH_BASE | 0x00000000U;
#endif
#endif
}

/* ══════════════════════════════════════════════════════════════════════════
 * SystemCoreClockUpdate — read live RCC registers, update SystemCoreClock.
 * Called by HAL library whenever the clock tree may have changed.
 * ══════════════════════════════════════════════════════════════════════════ */

__SECTION_BOOT void SystemCoreClockUpdate(void)
{
    uint32_t tmp, pllvco, pllp, pllm;
    uint32_t pllsource;

    tmp = RCC->CFGR & RCC_CFGR_SWS;

    switch (tmp)
    {
        case 0x00U:
            SystemCoreClock = CONFIG_HSI_VALUE;
            break;

        case 0x04U:
            SystemCoreClock = CONFIG_HSE_VALUE;
            break;

        case 0x08U:
            pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22U;
            pllm      =  RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

            pllvco = (pllsource != 0U ? CONFIG_HSE_VALUE : CONFIG_HSI_VALUE);
            pllvco = (pllvco / pllm) *
                     ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6U);

            pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16U) + 1U) * 2U;
            SystemCoreClock = pllvco / pllp;
            break;

        default:
            SystemCoreClock = CONFIG_HSI_VALUE;
            break;
    }

    /* Apply AHB prescaler */
    tmp = AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE) >> 4U];
    SystemCoreClock >>= tmp;
}

/* ══════════════════════════════════════════════════════════════════════════
 * drv_rcc HAL ops — PLL + bus clock configuration.
 * Called after HAL_Init() via drv_rcc_clock_init() from main().
 * ══════════════════════════════════════════════════════════════════════════ */

static int32_t _stm32_clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* Scale 1 required for SYSCLK > 84 MHz on F411 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* HSI → PLL */
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM            = RCC_PLLM_VAL;
    osc.PLL.PLLN            = RCC_PLLN_VAL;
    osc.PLL.PLLP            = RCC_PLLP_VAL;
    osc.PLL.PLLQ            = RCC_PLLQ_VAL;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        return OS_ERR_OP;

    /* Bus dividers */
    clk.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, RCC_FLASH_LATENCY) != HAL_OK)
        return OS_ERR_OP;

    return OS_ERR_NONE;
}

static uint32_t _stm32_get_sysclk_hz(void) { return HAL_RCC_GetSysClockFreq(); }
static uint32_t _stm32_get_apb1_hz(void)   { return HAL_RCC_GetPCLK1Freq();    }
static uint32_t _stm32_get_apb2_hz(void)   { return HAL_RCC_GetPCLK2Freq();    }

/* ── Registration ────────────────────────────────────────────────────────── */

static const drv_rcc_hal_ops_t _stm32_rcc_ops = {
    .clock_init    = _stm32_clock_init,
    .get_sysclk_hz = _stm32_get_sysclk_hz,
    .get_apb1_hz   = _stm32_get_apb1_hz,
    .get_apb2_hz   = _stm32_get_apb2_hz,
};

void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out)
{
    *ops_out = &_stm32_rcc_ops;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
