/*
 * hal_timebase_stm32.c — HAL tick/timebase via TIM1 (STM32)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Overrides the default SysTick-based HAL_InitTick() so that TIM1 drives the
 * 1 ms HAL tick counter, leaving SysTick free for FreeRTOS.
 *
 * htim1 is a module-level static — no other file needs to reference it
 * directly.  The IRQ handler calls hal_timebase_stm32_irq_handler() which is
 * declared in hal_timebase_stm32.h.
 */

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/hal_timebase_stm32.h>

static TIM_HandleTypeDef _htim1;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t           uwTimclock;
    uint32_t           uwPrescalerValue;
    uint32_t           pFLatency;
    HAL_StatusTypeDef  status;

    __HAL_RCC_TIM1_CLK_ENABLE();

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

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&_htim1, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&_htim1, TIM_IT_UPDATE);
}

void hal_timebase_stm32_irq_handler(void)
{
    HAL_TIM_IRQHandler(&_htim1);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
