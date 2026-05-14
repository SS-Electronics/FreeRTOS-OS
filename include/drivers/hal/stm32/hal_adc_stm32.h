/**
 * @file    hal_adc_stm32.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL ADC backend interface for generic ADC driver
 *
 * @details
 * Implements drv_adc_hal_ops_t for STM32 targets.
 * Configures ADC in continuous conversion mode with EOC interrupt.
 * IRQ dispatch uses drv_irq_dispatch_from_isr() → IRQ_ID_ADC_DATA_READY(n).
 *
 * Typical usage:
 * @code
 * drv_adc_handle_t adc = DRV_ADC_HANDLE_INIT(ADC_ECG_CH);
 * hal_adc_stm32_set_config(&adc, ADC1, ADC_CHANNEL_10,
 *                          ADC_RESOLUTION_12B, ADC_SAMPLETIME_64CYCLES_5);
 * drv_adc_register(ADC_ECG_CH, hal_adc_stm32_get_ops());
 * @endcode
 *
 * @note  This file is part of FreeRTOS-OS Project.
 * @license  GPLv3 — see <https://www.gnu.org/licenses/>.
 */

#ifndef DRIVERS_HAL_STM32_HAL_ADC_STM32_H_
#define DRIVERS_HAL_STM32_HAL_ADC_STM32_H_

#include <def_attributes.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <board/board_device_ids.h>
#include <drivers/drv_adc.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get STM32 ADC HAL operations table.
 */
const drv_adc_hal_ops_t *hal_adc_stm32_get_ops(void);

/**
 * @brief Configure STM32 ADC peripheral parameters before registration.
 *
 * @param h           ADC handle
 * @param instance    ADC peripheral (ADC1, ADC2)
 * @param channel     ADC channel (ADC_CHANNEL_10 etc.)
 * @param resolution  ADC_RESOLUTION_12B / _10B / _8B
 * @param sample_time ADC_SAMPLETIME_64CYCLES_5 etc.
 */
void hal_adc_stm32_set_config(drv_adc_handle_t *h,
                              ADC_TypeDef      *instance,
                              uint32_t          channel,
                              uint32_t          resolution,
                              uint32_t          sample_time);

/**
 * @brief STM32 ADC IRQ dispatcher.
 *
 * Must be called from ADC_IRQHandler.
 * Routes end-of-conversion interrupt to the correct ADC handle.
 *
 * @param instance  ADC peripheral that triggered the interrupt
 */
void hal_adc_stm32_irq_handler(ADC_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */

#endif /* DRIVERS_HAL_STM32_HAL_ADC_STM32_H_ */
