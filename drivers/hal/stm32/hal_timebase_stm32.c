/*
 * hal_timebase_stm32.c — HAL tick/timebase via TIM1 (STM32)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Overrides the default SysTick-based HAL_InitTick() so that TIM1 drives the
 * 1 ms HAL tick counter, leaving SysTick free for FreeRTOS.
 *
 * IRQ dispatch follows the irq_desc/irq_chip pattern used by UART, SPI, I2C.
 * The NVIC chip (irq_chip_nvic_get()) is bound to IRQ_ID_TIM_TICK in
 * irq_hw_init_all() with handle_edge_irq as the flow handler.
 *
 * Call chain:
 *   TIM1_UP_TIM10_IRQHandler  (irq_periph_dispatch_generated.c)
 *     → hal_timebase_stm32_irq_handler()
 *        ├─ __HAL_TIM_CLEAR_FLAG        — clears TIM1 update pending flag
 *        ├─ g_ms_ticks++ / HAL_IncTick()— advance global ms counter + HAL tick
 *        └─ __do_IRQ_from_isr(IRQ_ID_TIM_TICK)
 *              → handle_edge_irq → nvic_irq_ack (no-op) → irqaction chain
 *                                                          (optional subscribers)
 *
 * The hardware flag clear and tick increments run unconditionally so the
 * system is fully functional before irq_hw_init_all() populates handle_irq.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <irq/irq_desc.h>
#include <drivers/hal/stm32/hal_timebase_stm32.h>
#include <drivers/timer/drv_time.h>
#include <drivers/drv_rcc.h>

static TIM_HandleTypeDef _htim1;

/* ── HAL_InitTick override ──────────────────────────────────────────────── */

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t           uwTimclock;
    uint32_t           uwPrescalerValue;
    uint32_t           pFLatency;
    HAL_StatusTypeDef  status;

    drv_rcc_periph_clk_en(DRV_RCC_PERIPH_TIM1);

    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwTimclock       = 2U * HAL_RCC_GetPCLK2Freq();
    uwPrescalerValue = (uwTimclock / 1000000U) - 1U;

    _htim1.Instance               = TIM1;
    _htim1.Init.Period            = (1000000U / 1000U) - 1U;
    _htim1.Init.Prescaler         = uwPrescalerValue;
    _htim1.Init.ClockDivision     = 0;
    _htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    _htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&_htim1);
    if (status == HAL_OK)
    {
        status = HAL_TIM_Base_Start_IT(&_htim1);
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

    return status;
}

/* ── HAL tick suspend / resume ─────────────────────────────────────────── */

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&_htim1, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&_htim1, TIM_IT_UPDATE);
}

/* ── IRQ handler ────────────────────────────────────────────────────────── */

void hal_timebase_stm32_irq_handler(void)
{
    BaseType_t pxHPT = pdFALSE;

    /* Service hardware first — clear pending flag and advance tick counters.
     * Done unconditionally so the system works before irq_hw_init_all(). */
    __HAL_TIM_CLEAR_FLAG(&_htim1, TIM_FLAG_UPDATE);
    g_ms_ticks++;
    HAL_IncTick();

    /* Notify any irq_desc subscribers (populated after irq_hw_init_all). */
    struct irq_desc *desc = irq_to_desc(IRQ_ID_TIM_TICK);
    if (desc != NULL && desc->handle_irq != NULL)
        __do_IRQ_from_isr(IRQ_ID_TIM_TICK, NULL, &pxHPT);

    portYIELD_FROM_ISR(pxHPT);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
