/*
 * hal_it_stm32.h — STM32F411 peripheral interrupt handler declarations
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Cortex-M4 core exception handlers (NMI, HardFault, etc.) are defined in
 * stm32_exceptions.c and are not declared here — they are called only by the
 * hardware vector table, never by software.
 */

#ifndef DRIVERS_HAL_STM32_HAL_IT_STM32_H_
#define DRIVERS_HAL_STM32_HAL_IT_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ── Watchdog / power ─────────────────────────────────────────────────────── */
void WWDG_IRQHandler(void);
void PVD_IRQHandler(void);

/* ── RTC / tamper ─────────────────────────────────────────────────────────── */
void TAMP_STAMP_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);

/* ── Flash / RCC ──────────────────────────────────────────────────────────── */
void FLASH_IRQHandler(void);
void RCC_IRQHandler(void);

/* ── EXTI ─────────────────────────────────────────────────────────────────── */
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

/* ── DMA1 ─────────────────────────────────────────────────────────────────── */
void DMA1_Stream0_IRQHandler(void);
void DMA1_Stream1_IRQHandler(void);
void DMA1_Stream2_IRQHandler(void);
void DMA1_Stream3_IRQHandler(void);
void DMA1_Stream4_IRQHandler(void);
void DMA1_Stream5_IRQHandler(void);
void DMA1_Stream6_IRQHandler(void);
void DMA1_Stream7_IRQHandler(void);

/* ── DMA2 ─────────────────────────────────────────────────────────────────── */
void DMA2_Stream0_IRQHandler(void);
void DMA2_Stream1_IRQHandler(void);
void DMA2_Stream2_IRQHandler(void);
void DMA2_Stream3_IRQHandler(void);
void DMA2_Stream4_IRQHandler(void);
void DMA2_Stream5_IRQHandler(void);
void DMA2_Stream6_IRQHandler(void);
void DMA2_Stream7_IRQHandler(void);

/* ── ADC ──────────────────────────────────────────────────────────────────── */
void ADC_IRQHandler(void);

/* ── Timers ───────────────────────────────────────────────────────────────── */
void TIM1_BRK_TIM9_IRQHandler(void);
void TIM1_UP_TIM10_IRQHandler(void);
void TIM1_TRG_COM_TIM11_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);

/* ── UART ─────────────────────────────────────────────────────────────────── */
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART6_IRQHandler(void);

/* ── I2C ──────────────────────────────────────────────────────────────────── */
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);
void I2C3_EV_IRQHandler(void);
void I2C3_ER_IRQHandler(void);

/* ── SPI ──────────────────────────────────────────────────────────────────── */
void SPI1_IRQHandler(void);
void SPI2_IRQHandler(void);
void SPI3_IRQHandler(void);
void SPI4_IRQHandler(void);
void SPI5_IRQHandler(void);

/* ── SDIO / USB OTG FS ────────────────────────────────────────────────────── */
void SDIO_IRQHandler(void);
void OTG_FS_WKUP_IRQHandler(void);
void OTG_FS_IRQHandler(void);

/* ── FPU ──────────────────────────────────────────────────────────────────── */
void FPU_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_STM32_HAL_IT_STM32_H_ */
