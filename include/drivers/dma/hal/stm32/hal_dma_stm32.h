/*
 * hal_dma_stm32.h — STM32F4 DMA HAL backend for the DMA engine
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Exports two pre-built dma_device_t instances (one per controller):
 *
 *   hal_dma1_device  — DMA1, streams 0–7
 *   hal_dma2_device  — DMA2, streams 0–7
 *
 * Call hal_dma_stm32_init() once at boot (after clocks are on) before
 * registering them with the DMA engine.  hal_dma_stm32_init() internally
 * calls dma_register_device() for both controllers.
 *
 * STM32F411 stream → peripheral request mapping (DMA channel mux):
 *   DMA1 Stream 5, Channel 4 → USART2_RX
 *   DMA1 Stream 6, Channel 4 → USART2_TX
 *   DMA1 Stream 0, Channel 1 → I2C1_RX
 *   DMA1 Stream 6, Channel 1 → I2C1_TX
 *   DMA2 Stream 3, Channel 3 → SPI1_RX
 *   DMA2 Stream 3, Channel 3 → SPI1_TX
 */

#ifndef INCLUDE_DRIVERS_DMA_HAL_STM32_HAL_DMA_STM32_H_
#define INCLUDE_DRIVERS_DMA_HAL_STM32_HAL_DMA_STM32_H_

#include <drivers/dma/drv_dma.h>
#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>

#ifdef HAL_DMA_MODULE_ENABLED

/* ── STM32 per-channel hardware context ──────────────────────────────────── */

typedef struct {
    DMA_HandleTypeDef   hdma;    /* STM32 HAL DMA stream handle */
    IRQn_Type           irqn;    /* stream-specific NVIC IRQ    */
} drv_hw_dma_chan_ctx_t;

/* ── Pre-built controller instances ─────────────────────────────────────── */

extern dma_device_t hal_dma1_device;
extern dma_device_t hal_dma2_device;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * hal_dma_stm32_init — enable DMA clocks, wire stream instances and IRQs,
 *                      then call dma_register_device() for DMA1 and DMA2.
 * Must be called once before any dma_request_chan() calls.
 */
void hal_dma_stm32_init(void);

/*
 * hal_dma_stm32_irq_dispatch — route a DMA stream IRQ into the engine.
 * Call from irq_periph_dispatch_generated.c (via irq_table.xml dispatch=).
 *   ctrl_idx   : 1 = DMA1, 2 = DMA2
 *   stream_idx : 0–7
 */
void hal_dma_stm32_irq_dispatch(uint8_t ctrl_idx, uint8_t stream_idx);

#ifdef __cplusplus
}
#endif

#endif /* HAL_DMA_MODULE_ENABLED */
#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* INCLUDE_DRIVERS_DMA_HAL_STM32_HAL_DMA_STM32_H_ */
