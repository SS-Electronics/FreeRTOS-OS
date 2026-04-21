/*
 * hal_it_stm32.c — STM32F411 peripheral interrupt handlers
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Every handler in the vector table that the project uses is implemented here.
 * Cortex-M4 core exception handlers (NMI, HardFault, etc.) live in
 * stm32_exceptions.c.
 *
 * Dispatch model
 * ──────────────
 * UART (USART1/2/6)
 *   Reads SR/CR1/CR3 registers directly via hal_uart_stm32_irq_handler().
 *   Dispatches RX byte, TX done, and error events to the drv_irq framework
 *   without going through the STM32 HAL callback chain.
 *
 * I2C (I2C1-3 event + error)
 *   Delegates to HAL_I2C_EV_IRQHandler / HAL_I2C_ER_IRQHandler. The HAL
 *   fires HAL_I2C_MasterTxCpltCallback etc., which call drv_irq_dispatch_from_isr.
 *
 * SPI (SPI1-5)
 *   Delegates to HAL_SPI_IRQHandler. HAL fires completion callbacks that call
 *   drv_irq_dispatch_from_isr.
 *
 * DMA (DMA1/2 streams)
 *   Empty stubs.  Populate with HAL_DMA_IRQHandler(&hdma_xxx) when a
 *   peripheral's DMA handle is managed by the driver layer.
 *
 * Timers
 *   TIM1_UP_TIM10 → hal_timebase_stm32_irq_handler() (FreeRTOS tick).
 *   Others are empty stubs; populate when a timer driver is added.
 *
 * GPIO EXTI
 *   Calls HAL_GPIO_EXTI_IRQHandler() for every pin in the shared group.
 *   HAL checks EXTI_PR internally and only processes pending pins.
 *
 * Adding a new peripheral ISR:
 *   1. Add the prototype to hal_it_stm32.h.
 *   2. Add the body here.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <drivers/hal/stm32/hal_it_stm32.h>
#include <drivers/hal/stm32/hal_timebase_stm32.h>
#include <drivers/com/hal/stm32/hal_uart_stm32.h>
#include <drivers/com/hal/stm32/hal_iic_stm32.h>
#include <drivers/com/hal/stm32/hal_spi_stm32.h>
#include <drivers/drv_irq.h>

/* ── Watchdog ─────────────────────────────────────────────────────────────── */

void WWDG_IRQHandler(void) {}

/* ── Power / voltage detection ────────────────────────────────────────────── */

void PVD_IRQHandler(void) {}

/* ── Tamper / RTC ─────────────────────────────────────────────────────────── */

void TAMP_STAMP_IRQHandler(void) {}

void RTC_WKUP_IRQHandler(void) {}

void RTC_Alarm_IRQHandler(void) {}

/* ── Flash ────────────────────────────────────────────────────────────────── */

void FLASH_IRQHandler(void) {}

/* ── RCC ──────────────────────────────────────────────────────────────────── */

void RCC_IRQHandler(void) {}

/* ── EXTI — individual lines ─────────────────────────────────────────────── */

/*
 * EXTI0-4: dispatch directly through the irq_desc chain so pxHPT is
 * propagated correctly (HAL_GPIO_EXTI_Callback has no pxHPT parameter).
 * The pin bitmask is passed as the event payload.
 */
#define _EXTI_DISPATCH(pin_mask, exti_line_idx)          \
    do {                                                 \
        BaseType_t _hpt = pdFALSE;                       \
        if (__HAL_GPIO_EXTI_GET_IT(pin_mask)) {          \
            __HAL_GPIO_EXTI_CLEAR_IT(pin_mask);          \
            uint16_t _pin = (pin_mask);                  \
            drv_irq_dispatch_from_isr(                   \
                IRQ_ID_EXTI(exti_line_idx), &_pin, &_hpt); \
        }                                                \
        portYIELD_FROM_ISR(_hpt);                        \
    } while (0)

void EXTI0_IRQHandler(void) { _EXTI_DISPATCH(GPIO_PIN_0,  0); }
void EXTI1_IRQHandler(void) { _EXTI_DISPATCH(GPIO_PIN_1,  1); }
void EXTI2_IRQHandler(void) { _EXTI_DISPATCH(GPIO_PIN_2,  2); }
void EXTI3_IRQHandler(void) { _EXTI_DISPATCH(GPIO_PIN_3,  3); }
void EXTI4_IRQHandler(void) { _EXTI_DISPATCH(GPIO_PIN_4,  4); }

/* EXTI9_5: shared handler for GPIO pins 5-9.
   HAL_GPIO_EXTI_IRQHandler() checks EXTI_PR internally. */
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

/* EXTI15_10: shared handler for GPIO pins 10-15. */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

/* ── DMA1 streams ─────────────────────────────────────────────────────────── */
/* Populate with HAL_DMA_IRQHandler(&hdma_xxx) when DMA is used by a driver. */

void DMA1_Stream0_IRQHandler(void) {}
void DMA1_Stream1_IRQHandler(void) {}
void DMA1_Stream2_IRQHandler(void) {}
void DMA1_Stream3_IRQHandler(void) {}
void DMA1_Stream4_IRQHandler(void) {}
void DMA1_Stream5_IRQHandler(void) {}
void DMA1_Stream6_IRQHandler(void) {}
void DMA1_Stream7_IRQHandler(void) {}

/* ── DMA2 streams ─────────────────────────────────────────────────────────── */

void DMA2_Stream0_IRQHandler(void) {}
void DMA2_Stream1_IRQHandler(void) {}
void DMA2_Stream2_IRQHandler(void) {}
void DMA2_Stream3_IRQHandler(void) {}
void DMA2_Stream4_IRQHandler(void) {}
void DMA2_Stream5_IRQHandler(void) {}
void DMA2_Stream6_IRQHandler(void) {}
void DMA2_Stream7_IRQHandler(void) {}

/* ── ADC ──────────────────────────────────────────────────────────────────── */

void ADC_IRQHandler(void) {}

/* ── Timers ───────────────────────────────────────────────────────────────── */

/* TIM1 break shared with TIM9 */
void TIM1_BRK_TIM9_IRQHandler(void) {}

/* TIM1 update shared with TIM10 — used as FreeRTOS timebase */
void TIM1_UP_TIM10_IRQHandler(void)
{
    hal_timebase_stm32_irq_handler();
}

/* TIM1 trigger/commutation shared with TIM11 */
void TIM1_TRG_COM_TIM11_IRQHandler(void) {}

/* TIM1 capture/compare */
void TIM1_CC_IRQHandler(void) {}

/* General-purpose timers — populate with HAL_TIM_IRQHandler(&htimN) when used */
void TIM2_IRQHandler(void) {}
void TIM3_IRQHandler(void) {}
void TIM4_IRQHandler(void) {}
void TIM5_IRQHandler(void) {}

/* ── UART ─────────────────────────────────────────────────────────────────── */

/*
 * Each handler identifies the owning drv_uart_handle via the instance pointer,
 * reads USART SR/CR1/CR3 directly, and dispatches events to the drv_irq
 * framework.  See hal_uart_stm32.c — hal_uart_stm32_irq_handler() for details.
 */

void USART1_IRQHandler(void) { hal_uart_stm32_irq_handler(USART1); }
void USART2_IRQHandler(void) { hal_uart_stm32_irq_handler(USART2); }
void USART6_IRQHandler(void) { hal_uart_stm32_irq_handler(USART6); }

/* ── I2C ──────────────────────────────────────────────────────────────────── */

#ifdef HAL_I2C_MODULE_ENABLED

void I2C1_EV_IRQHandler(void) { hal_iic_stm32_ev_irq_handler(I2C1); }
void I2C1_ER_IRQHandler(void) { hal_iic_stm32_er_irq_handler(I2C1); }

void I2C2_EV_IRQHandler(void) { hal_iic_stm32_ev_irq_handler(I2C2); }
void I2C2_ER_IRQHandler(void) { hal_iic_stm32_er_irq_handler(I2C2); }

void I2C3_EV_IRQHandler(void) { hal_iic_stm32_ev_irq_handler(I2C3); }
void I2C3_ER_IRQHandler(void) { hal_iic_stm32_er_irq_handler(I2C3); }

#endif /* HAL_I2C_MODULE_ENABLED */

/* ── SPI ──────────────────────────────────────────────────────────────────── */

#ifdef HAL_SPI_MODULE_ENABLED

void SPI1_IRQHandler(void) { hal_spi_stm32_irq_handler(SPI1); }
void SPI2_IRQHandler(void) { hal_spi_stm32_irq_handler(SPI2); }
void SPI3_IRQHandler(void) { hal_spi_stm32_irq_handler(SPI3); }
void SPI4_IRQHandler(void) { hal_spi_stm32_irq_handler(SPI4); }
void SPI5_IRQHandler(void) { hal_spi_stm32_irq_handler(SPI5); }

#endif /* HAL_SPI_MODULE_ENABLED */

/* ── SDIO ─────────────────────────────────────────────────────────────────── */

void SDIO_IRQHandler(void) {}

/* ── USB OTG FS ───────────────────────────────────────────────────────────── */

void OTG_FS_WKUP_IRQHandler(void) {}
void OTG_FS_IRQHandler(void)      {}

/* ── FPU ──────────────────────────────────────────────────────────────────── */

void FPU_IRQHandler(void) {}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
