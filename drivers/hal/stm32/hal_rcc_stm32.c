/**
 * @file    hal_rcc_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   STM32 RCC + CMSIS system initialization backend
 *
 * @ingroup hal_rcc
 *
 * @details
 * This module implements the Reset and Clock Control (RCC) backend for STM32,
 * replacing the default CMSIS system file (e.g. system_stm32f4xx.c).
 *
 * It provides a clean separation between:
 * - Early boot clock setup (pre-scheduler)
 * - Runtime clock configuration (post HAL_Init)
 *
 * The implementation integrates:
 * - CMSIS SystemCoreClock model
 * - STM32 HAL RCC APIs
 * - Project-specific driver abstraction (drv_rcc)
 *
 * -----------------------------------------------------------------------------
 * @section boot_phase Pre-Scheduler Initialization
 *
 * Executed before C runtime initialization (from Reset_Handler):
 *
 * - SystemInit()
 *      - Enables FPU (if present)
 *      - Optionally relocates vector table (VTOR)
 *
 * - SystemCoreClockUpdate()
 *      - Reads RCC registers
 *      - Updates global SystemCoreClock
 *
 * - CMSIS globals (required by HAL):
 *      - SystemCoreClock
 *      - AHBPrescTable[]
 *      - APBPrescTable[]
 *
 * -----------------------------------------------------------------------------
 * @section runtime_phase Post-HAL Initialization
 *
 * Executed after HAL_Init() from application layer:
 *
 * - _stm32_clock_init()
 *      Configure PLL, bus clocks, and flash latency
 *
 * - Clock query APIs:
 *      _stm32_get_sysclk_hz()
 *      _stm32_get_apb1_hz()
 *      _stm32_get_apb2_hz()
 *
 * -----------------------------------------------------------------------------
 * @section clock_tree Configured Clock Tree
 *
 * Default configuration:
 *
 * @code
 * HSI (16 MHz)
 *   └─ /PLLM → VCO input
 *       └─ ×PLLN → VCO output
 *           ├─ /PLLP → SYSCLK
 *           └─ /PLLQ → USB/SDIO
 *
 * AHB  = SYSCLK / 1
 * APB1 = HCLK   / 2
 * APB2 = HCLK   / 1
 * @endcode
 *
 * -----------------------------------------------------------------------------
 * @section design_constraints Design Constraints
 *
 * - All RCC register access is encapsulated in this file
 * - Upper layers must not directly use HAL_RCC_* APIs
 * - Peripheral clock enables are centralized here
 *
 * -----------------------------------------------------------------------------
 * @note
 * Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 */

/* ────────────────────────────────────────────────────────────────────────── */

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <stdint.h>
#include <device.h>
#include <def_attributes.h>
#include <def_err.h>
#include <drivers/hal/stm32/hal_rcc_stm32.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* CMSIS Clock Update                                                        */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Update SystemCoreClock from RCC registers
 *
 * @details
 * Computes the current system clock frequency based on RCC configuration.
 * This function is required by CMSIS and is used by STM32 HAL internally.
 *
 * @note
 * Must be called whenever clock configuration changes.
 *
 * @warning
 * Assumes PLL and prescaler configuration follow STM32 reference manual.
 */
__SECTION_OS __USED
void SystemCoreClockUpdate(void)
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

/* ────────────────────────────────────────────────────────────────────────── */
/* Clock Configuration                                                       */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize system clock tree
 *
 * @retval OS_ERR_NONE Success
 * @retval OS_ERR_OP   HAL configuration failure
 *
 * @details
 * Configures:
 * - PLL source and multipliers
 * - Bus prescalers (AHB, APB1, APB2)
 * - Flash latency
 *
 * @pre HAL_Init() must be called before this function
 */
__SECTION_OS __USED
static int32_t _stm32_clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    hal_rcc_stm32_pwr_clk_en();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM            = BOARD_RCC_PLLM;
    osc.PLL.PLLN            = BOARD_RCC_PLLN;
    osc.PLL.PLLP            = BOARD_RCC_PLLP;
    osc.PLL.PLLQ            = BOARD_RCC_PLLQ;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        return OS_ERR_OP;

    clk.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, BOARD_FLASH_LATENCY) != HAL_OK)
        return OS_ERR_OP;

    return OS_ERR_NONE;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Clock Query APIs                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/** @brief Get SYSCLK frequency (Hz) */
__SECTION_OS __USED
static uint32_t _stm32_get_sysclk_hz(void)
{
    return HAL_RCC_GetSysClockFreq();
}

/** @brief Get APB1 frequency (Hz) */
__SECTION_OS __USED
static uint32_t _stm32_get_apb1_hz(void)
{
    return HAL_RCC_GetPCLK1Freq();
}

/** @brief Get APB2 frequency (Hz) */
__SECTION_OS __USED
static uint32_t _stm32_get_apb2_hz(void)
{
    return HAL_RCC_GetPCLK2Freq();
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Peripheral Clock Control                                                  */
/* ────────────────────────────────────────────────────────────────────────── */

/* ── Named per-peripheral clock enables ──────────────────────────────────── */
__SECTION_OS __USED
void hal_rcc_stm32_pwr_clk_en(void)    
{ 
    __HAL_RCC_PWR_CLK_ENABLE(); 
}

__SECTION_OS __USED
void hal_rcc_stm32_syscfg_clk_en(void) 
{ 
    __HAL_RCC_SYSCFG_CLK_ENABLE(); 
}

__SECTION_OS __USED
void hal_rcc_stm32_usart1_clk_en(void) 
{
    __HAL_RCC_USART1_CLK_ENABLE(); 
}

__SECTION_OS __USED
void hal_rcc_stm32_usart2_clk_en(void) 
{
    __HAL_RCC_USART2_CLK_ENABLE();
}

__SECTION_OS __USED
void hal_rcc_stm32_i2c1_clk_en(void)   
{ 
    __HAL_RCC_I2C1_CLK_ENABLE(); 
}

__SECTION_OS __USED
void hal_rcc_stm32_spi1_clk_en(void)   
{ 
    __HAL_RCC_SPI1_CLK_ENABLE(); 
}

__SECTION_OS __USED
void hal_rcc_stm32_tim1_clk_en(void)   
{ 
    __HAL_RCC_TIM1_CLK_ENABLE(); 
}

/**
 * @brief Enable peripheral clock
 *
 * @param periph Peripheral identifier
 *
 * @details
 * Centralized control for all peripheral clock enables.
 * Prevents direct HAL_RCC_* usage in upper layers.
 */
__SECTION_OS __USED
void hal_rcc_stm32_periph_clk_en(drv_rcc_periph_t periph)
{
    switch (periph)
    {
        case DRV_RCC_PERIPH_USART1: __HAL_RCC_USART1_CLK_ENABLE(); break;
        case DRV_RCC_PERIPH_USART2: __HAL_RCC_USART2_CLK_ENABLE(); break;
        case DRV_RCC_PERIPH_I2C1:   __HAL_RCC_I2C1_CLK_ENABLE();   break;
        case DRV_RCC_PERIPH_SPI1:   __HAL_RCC_SPI1_CLK_ENABLE();   break;
        case DRV_RCC_PERIPH_TIM1:   __HAL_RCC_TIM1_CLK_ENABLE();   break;
        case DRV_RCC_PERIPH_SYSCFG: __HAL_RCC_SYSCFG_CLK_ENABLE(); break;
        case DRV_RCC_PERIPH_PWR:    __HAL_RCC_PWR_CLK_ENABLE();    break;
        default: break;
    }
}

/**
 * @brief Enable GPIO port clock
 *
 * @param port GPIO port base address (GPIOA, GPIOB, ...)
 */
__SECTION_OS __USED
void hal_rcc_stm32_gpio_clk_en(void *port)
{
    if      (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
}

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL Registration                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief RCC HAL operations table
 */
static const drv_rcc_hal_ops_t _stm32_rcc_ops = {
    .clock_init    = _stm32_clock_init,
    .get_sysclk_hz = _stm32_get_sysclk_hz,
    .get_apb1_hz   = _stm32_get_apb1_hz,
    .get_apb2_hz   = _stm32_get_apb2_hz,
};

/**
 * @brief Register RCC HAL backend
 *
 * @param ops_out Output pointer to HAL ops table
 */
__SECTION_OS __USED
void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out)
{
    *ops_out = &_stm32_rcc_ops;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */