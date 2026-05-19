/**
 * @file        adc_mgmt.h
 * @brief       ADC management service thread interface
 * @ingroup     services
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Services
 * @info        FreeRTOS service threads that own peripherals (UART/I2C/SPI/GPIO/ADC) and accept commands.
 * @dependency  Driver layer, ipc/mqueue, board config
 *
 * @details
 * Manages all ADC peripherals as a FreeRTOS service thread.
 * Follows the same pattern as uart_mgmt.
 *
 * Responsibilities:
 *   - Peripheral registration and hardware init at startup.
 *   - Subscription to IRQ_ID_ADC_DATA_READY(n) for each channel.
 *   - Software decimation to target sample rate.
 *   - Ring-buffer storage for application consumption.
 *
 * Execution model:
 *   - ISR fires ADC EOC → irq_notify → _adc_data_cb pushes sample to queue
 *   - adc_mgmt_thread decimates and stores into ring buffer
 *   - App reads via adc_mgmt_read_sample()
 *
 * @note  This file is part of FreeRTOS-OS Project.
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

#ifndef INCLUDE_SERVICES_ADC_MGMT_H_
#define INCLUDE_SERVICES_ADC_MGMT_H_

#include <def_attributes.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <os/kernel.h>
#include <board/board_config.h>

#ifndef ADC_MGMT_QUEUE_DEPTH
#define ADC_MGMT_QUEUE_DEPTH   64
#endif

#ifndef ADC_MGMT_RING_BUF_SIZE
#define ADC_MGMT_RING_BUF_SIZE 1024
#endif

/* Decimate to this rate from continuous ADC (samples/sec) */
#ifndef ADC_TARGET_SAMPLE_RATE_HZ
#define ADC_TARGET_SAMPLE_RATE_HZ  500U
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BOARD_ADC_COUNT) && (BOARD_ADC_COUNT > 0)

/**
 * @brief Create and start the ADC management service thread.
 * @return OS_ERR_NONE on success
 */
int32_t adc_mgmt_start(void);

/**
 * @brief Read one decimated sample from the ADC ring buffer.
 *
 * @param dev_id  ADC device index
 * @param sample  Output: 12-bit ADC value (0–4095)
 * @return OS_ERR_NONE if sample available, OS_ERR_OP if empty
 */
int32_t adc_mgmt_read_sample(uint8_t dev_id, uint16_t *sample);

/**
 * @brief Number of samples available in the ring buffer.
 *
 * @param dev_id  ADC device index
 * @return Count of unread samples
 */
uint32_t adc_mgmt_available(uint8_t dev_id);

#endif /* BOARD_ADC_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_SERVICES_ADC_MGMT_H_ */
