/**
 * @file        hal_timebase_stm32.c
 * @brief       HAL timebase implementation using TIM1 for STM32 platforms
 * @ingroup     hal_stm32
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      STM32 HAL
 * @info        STM32-specific HAL backend implementing the generic driver vtables for STM32F4 / STM32H7.
 * @dependency  stm32f4xx-hal-driver, stm32h7xx-hal-driver
 *
 * @details
 * This module overrides the default SysTick-based HAL timebase and instead
 * uses TIM1 to generate the 1 ms system tick required by the STM32 HAL.
 *
 * This design allows:
 * - FreeRTOS to exclusively use SysTick
 * - HAL to operate independently via a hardware timer
 *
 * The implementation integrates with the generic IRQ subsystem:
 * - IRQ dispatch follows irq_desc / irq_chip abstraction
 * - NVIC is used as the interrupt controller backend
 *
 * -----------------------------------------------------------------------------
 * @section timebase_flow Timebase Execution Flow
 *
 * @code
 * TIM1 Update Interrupt
 *     → TIM1_UP_TIM10_IRQHandler (vector table)
 *         → hal_timebase_stm32_irq_handler()
 *              ├─ Clear TIM1 update flag
 *              ├─ Increment system tick counters
 *              │     ├─ g_ms_ticks++
 *              │     └─ HAL_IncTick()
 *              └─ Dispatch IRQ (optional subscribers)
 *                    → __do_IRQ_from_isr()
 *                        → irq_desc → irq_chip → handlers
 * @endcode
 *
 * -----------------------------------------------------------------------------
 * @section design_notes Design Notes
 *
 * - SysTick is not used (reserved for FreeRTOS scheduler)
 * - Timer runs at 1 MHz base, generating 1 ms interrupts
 * - Tick increment is unconditional to ensure early boot correctness
 * - IRQ dispatch is optional and depends on irq_hw_init_all()
 *
 * -----------------------------------------------------------------------------
 * @dependencies
 * board/mcu_config.h, device.h, irq/irq_desc.h,
 * drivers/hal/stm32/hal_timebase_stm32.h,
 * drivers/drv_time.h, drivers/drv_rcc.h
 *
 * @note
 * Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 *
 * @license
 *
 * See <https://www.gnu.org/licenses/>.
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <irq/irq_desc.h>
#include <drivers/hal/stm32/hal_timebase_stm32.h>
#include <drivers/drv_time.h>
#include <drivers/drv_rcc.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* Static TIM handle                                                         */
/* ────────────────────────────────────────────────────────────────────────── */

/* H7: TIM6 (APB1 basic timer) — dispatched via TIM6_DAC_IRQHandler in generated code.
 * F4: TIM1 (APB2 advanced timer) — dispatched via TIM1_UP_TIM10_IRQHandler. */
__SECTION_OS_DATA __USED
static TIM_HandleTypeDef _htim_base;

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL Tick Initialization                                                   */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize HAL tick using TIM1
 *
 * @param TickPriority Interrupt priority for TIM1 update IRQ
 *
 * @retval HAL_OK      Initialization successful
 * @retval HAL_ERROR   Invalid priority or initialization failure
 *
 * @details
 * Configures TIM1 to generate a 1 ms periodic interrupt:
 * - Timer clock derived from APB2 (doubled if prescaled)
 * - Prescaler configured for 1 MHz timer clock
 * - Auto-reload set for 1 ms period
 *
 * Also:
 * - Enables TIM1 interrupt in NVIC
 * - Sets interrupt priority
 *
 * @note
 * Overrides the weak HAL_InitTick() implementation.
 */
__SECTION_OS __USED
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    uint32_t          uwTimclock;
    uint32_t          uwPrescalerValue;
    HAL_StatusTypeDef status;

#if defined(STM32H7)
    /* TIM6 on D2APB1. When D2PPRE1 == /1 (reset default), TIM6 clock = PCLK1.
     * When D2PPRE1 > /1, TIM6 clock = 2 × PCLK1. */
    drv_rcc_periph_clk_en(DRV_RCC_PERIPH_TIM6);
    uwTimclock = (READ_BIT(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1) != 0U)
                 ? (2U * HAL_RCC_GetPCLK1Freq())
                 : HAL_RCC_GetPCLK1Freq();
    uwPrescalerValue = (uwTimclock / 1000000U) - 1U;

    _htim_base.Instance               = TIM6;
    _htim_base.Init.Period            = (1000000U / 1000U) - 1U;
    _htim_base.Init.Prescaler         = uwPrescalerValue;
    _htim_base.Init.ClockDivision     = 0;
    _htim_base.Init.CounterMode       = TIM_COUNTERMODE_UP;
    _htim_base.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&_htim_base);
    if (status == HAL_OK)
    {
        status = HAL_TIM_Base_Start_IT(&_htim_base);
        if (status == HAL_OK)
        {
            HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
            if (TickPriority < (1UL << __NVIC_PRIO_BITS))
            {
                HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0U);
                uwTickPrio = TickPriority;
            }
            else
            {
                status = HAL_ERROR;
            }
        }
    }

#else /* F4: TIM1 on APB2 */

    RCC_ClkInitTypeDef clkconfig;
    uint32_t           pFLatency;

    drv_rcc_periph_clk_en(DRV_RCC_PERIPH_TIM1);
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwTimclock       = 2U * HAL_RCC_GetPCLK2Freq();
    uwPrescalerValue = (uwTimclock / 1000000U) - 1U;

    _htim_base.Instance               = TIM1;
    _htim_base.Init.Period            = (1000000U / 1000U) - 1U;
    _htim_base.Init.Prescaler         = uwPrescalerValue;
    _htim_base.Init.ClockDivision     = 0;
    _htim_base.Init.CounterMode       = TIM_COUNTERMODE_UP;
    _htim_base.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&_htim_base);
    if (status == HAL_OK)
    {
        status = HAL_TIM_Base_Start_IT(&_htim_base);
        if (status == HAL_OK)
        {
            HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
            if (TickPriority < (1UL << __NVIC_PRIO_BITS))
            {
                HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, TickPriority, 0U);
                uwTickPrio = TickPriority;
            }
            else
            {
                status = HAL_ERROR;
            }
        }
    }

#endif /* STM32H7 */

    return status;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Tick Control                                                              */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Suspend HAL tick increment
 *
 * @details
 * Disables TIM1 update interrupt, effectively pausing the HAL tick.
 *
 * @note
 * Does not stop the timer counter itself.
 */
__SECTION_OS __USED
void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&_htim_base, TIM_IT_UPDATE);
}

/**
 * @brief Resume HAL tick increment
 *
 * @details
 * Re-enables TIM1 update interrupt, restoring HAL tick updates.
 */
__SECTION_OS __USED
void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&_htim_base, TIM_IT_UPDATE);
}

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Handler                                                               */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief TIM1 timebase interrupt handler
 *
 * @details
 * This function is invoked from the actual IRQ vector handler
 * (TIM1_UP_TIM10_IRQHandler).
 *
 * Responsibilities:
 * 1. Clear hardware interrupt flag (TIM update)
 * 2. Increment system time counters:
 *    - g_ms_ticks (OS-level time)
 *    - HAL tick via HAL_IncTick()
 * 3. Dispatch IRQ event to generic IRQ subsystem (if initialized)
 *
 * IRQ dispatch flow:
 * @code
 * __do_IRQ_from_isr()
 *   → irq_desc lookup
 *   → flow handler (handle_edge_irq)
 *   → irq_chip callbacks
 *   → registered handlers
 * @endcode
 *
 * @note
 * - Tick update is unconditional (works before IRQ subsystem init)
 * - IRQ dispatch is conditional (depends on irq_desc setup)
 */
__SECTION_OS __USED
void hal_timebase_stm32_irq_handler(void)
{
    BaseType_t pxHPT = pdFALSE;

    /* Always service hardware first */
    __HAL_TIM_CLEAR_FLAG(&_htim_base, TIM_FLAG_UPDATE);

    /* Update system ticks */
    g_ms_ticks++;
    HAL_IncTick();

    /* Optional IRQ dispatch */
    struct irq_desc *desc = irq_to_desc(IRQ_ID_TIM_TICK);
    if (desc != NULL && desc->handle_irq != NULL)
        __do_IRQ_from_isr(IRQ_ID_TIM_TICK, NULL, &pxHPT);

    portYIELD_FROM_ISR(pxHPT);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */