/**
 * @file        drv_adc.h
 * @brief       Generic ADC driver interface, handle definition, and HAL abstraction
 * @ingroup     drivers
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver Layer
 * @info        Vendor-agnostic driver vtables; concrete backends live under drivers/hal/<vendor>/.
 * @dependency  HAL backend (selected by CONFIG_DEVICE_VARIANT)
 *
 * @details
 * Vendor-agnostic ADC driver following the same 3-layer pattern as drv_uart:
 *   generic driver (drv_adc) ↔ HAL ops vtable ↔ vendor HAL (hal_adc_stm32)
 *
 * Supports interrupt-driven single-ended ADC with software decimation.
 * ADC_IRQn fires on end-of-conversion; the ISR dispatches
 * IRQ_ID_ADC_DATA_READY(n) into the irq_notify framework.
 *
 * @dependencies  def_std.h, def_err.h, board/board_config.h
 *
 * @note  This file is part of FreeRTOS-OS Project.
 *
 * @license  GPLv3 — see <https://www.gnu.org/licenses/>.
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

#ifndef INCLUDE_DRIVERS_DRV_ADC_H_
#define INCLUDE_DRIVERS_DRV_ADC_H_

#include <def_attributes.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <board/board_handles.h>

/* ── Forward declaration ───────────────────────────────────────────────── */
typedef struct drv_adc_handle drv_adc_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

typedef struct {
    ADC_HandleTypeDef hadc;
    uint32_t          channel;      /* ADC_CHANNEL_10 etc. — set before hw_init */
    uint32_t          sample_time;  /* ADC_SAMPLETIME_* — set before hw_init   */
} drv_hw_adc_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported for ADC driver."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
typedef struct drv_adc_hal_ops {

    /**
     * @brief Initialize ADC hardware (GPIO, clock, peripheral, calibration).
     * @return OS_ERR_NONE on success
     */
    int32_t (*hw_init)(drv_adc_handle_t *h);

    /**
     * @brief De-initialize ADC hardware.
     */
    int32_t (*hw_deinit)(drv_adc_handle_t *h);

    /**
     * @brief Start continuous ADC conversion with EOC interrupt enabled.
     * @return OS_ERR_NONE on success
     */
    int32_t (*start)(drv_adc_handle_t *h);

    /**
     * @brief Stop ADC conversion.
     */
    int32_t (*stop)(drv_adc_handle_t *h);

} drv_adc_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
struct drv_adc_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_adc_ctx_t         hw;
    const drv_adc_hal_ops_t *ops;
    int32_t                  last_err;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
#define DRV_ADC_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(BOARD_ADC_COUNT) && (BOARD_ADC_COUNT > 0)

/**
 * @brief Register ADC device and initialize hardware.
 */
int32_t drv_adc_register(uint8_t dev_id,
                         const drv_adc_hal_ops_t *ops);

/**
 * @brief Get ADC handle by device ID.
 */
drv_adc_handle_t *drv_adc_get_handle(uint8_t dev_id);

/**
 * @brief Start ADC continuous conversion.
 */
int32_t drv_adc_start(uint8_t dev_id);

/**
 * @brief Stop ADC conversion.
 */
int32_t drv_adc_stop(uint8_t dev_id);

#endif /* BOARD_ADC_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_ADC_H_ */
