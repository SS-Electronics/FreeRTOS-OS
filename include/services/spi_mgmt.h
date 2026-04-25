/**
 * @file    spi_mgmt.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  services
 * @brief   SPI management service thread for serialized SPI bus access
 *
 * @details
 * This module defines the public interface for the SPI management service
 * thread used in FreeRTOS-OS Project. It provides a centralized mechanism
 * to serialize SPI transactions across multiple tasks and peripherals.
 *
 * The SPI management layer ensures that:
 * - Only one SPI transaction is active per bus at a time
 * - Interrupt-driven HAL transfers are used for efficiency
 * - Blocking and non-blocking APIs are supported
 * - Full-duplex transfers are handled via TX/RX buffers
 *
 * Important design note:
 * Chip Select (CS) GPIO control is NOT handled by this layer.
 * The caller must assert CS before issuing a transaction and deassert it
 * after completion notification is received.
 *
 * Initialization flow:
 * @code
 * spi_mgmt_start()
 *      ↓
 * Create FreeRTOS queue
 *      ↓
 * Spawn spi_mgmt_thread()
 *      ↓
 * Register SPI buses from board configuration
 *      ↓
 * Subscribe to IRQ notifications (TX/RX/ERROR)
 *      ↓
 * Enter request processing loop
 * @endcode
 *
 * Runtime operation:
 * - SPI requests are posted via FreeRTOS queue
 * - Interrupt-driven HAL APIs are preferred when available
 * - Completion is signaled using FreeRTOS task notifications
 * - Optional synchronous wrappers block until completion or timeout
 *
 * Supported commands:
 * - TRANSMIT (TX only)
 * - RECEIVE  (RX only)
 * - TRANSFER (full-duplex TX+RX)
 * - REINIT   (hardware reset + reinitialization)
 *
 * @dependencies
 * def_attributes.h, def_compiler.h, def_std.h, def_err.h,
 * services/spi_mgmt.h, os/kernel.h,
 * drivers/drv_spi.h, board/board_config.h,
 * irq/irq_notify.h
 *
 * @note
 * - This module is part of FreeRTOS-OS Project
 * - Runs as a dedicated system service thread
 * - Uses FreeRTOS queue + task notifications for synchronization
 *
 * @warning
 * - CS (chip select) is NOT managed by this layer
 * - Buffers passed to async APIs must remain valid until completion
 * - Queue overflow results in request loss
 * - SPI operations are not ISR-safe (must be posted from task context)
 *
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DRIVERS_MGMT_SPI_MGMT_H_
#define DRIVERS_MGMT_SPI_MGMT_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <services/spi_mgmt.h>
#include <os/kernel.h>
#include <drivers/drv_spi.h>
#include <board/board_config.h>
#include <irq/irq_notify.h>

/* ── Message types ────────────────────────────────────────────────────── */

/**
 * @brief SPI management command types.
 *
 * Defines the supported SPI operations that can be requested through
 * the SPI management queue.
 */
typedef enum {
    SPI_MGMT_CMD_TRANSMIT = 0,  /**< TX only operation                    */
    SPI_MGMT_CMD_RECEIVE,       /**< RX only operation                    */
    SPI_MGMT_CMD_TRANSFER,      /**< Full-duplex TX + RX operation       */
    SPI_MGMT_CMD_REINIT,        /**< Reinitialize SPI peripheral         */
} spi_mgmt_cmd_t;

/**
 * @struct spi_mgmt_msg_t
 * @brief  Message structure posted to SPI management queue.
 *
 * @details
 * This structure represents a single SPI transaction request. It may
 * represent transmit-only, receive-only, or full-duplex transfer.
 *
 * Completion handling:
 * - If result_notify is non-NULL, the task is notified on completion
 * - If result_code is non-NULL, the operation return code is stored
 */
typedef struct {
    spi_mgmt_cmd_t   cmd;           /**< SPI operation type                */
    uint8_t          bus_id;        /**< SPI bus index                    */
    const uint8_t   *tx_data;       /**< TX buffer (NULL if RX only)     */
    uint8_t         *rx_data;       /**< RX buffer (NULL if TX only)     */
    uint16_t         len;           /**< Transfer length in bytes         */
    TaskHandle_t     result_notify; /**< Task to notify on completion     */
    int32_t         *result_code;   /**< Optional return code storage     */
} spi_mgmt_msg_t;

/* ── Public API ───────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start SPI management service thread.
 *
 * @return OS_ERR_NONE on success, negative error code otherwise.
 */
int32_t       spi_mgmt_start(void);

/**
 * @brief Get SPI management queue handle.
 *
 * @return FreeRTOS queue handle used for SPI requests.
 */
QueueHandle_t spi_mgmt_get_queue(void);

/**
 * @brief Blocking full-duplex SPI transfer.
 *
 * @param bus_id       SPI bus index
 * @param tx           TX buffer (can be NULL)
 * @param rx           RX buffer (can be NULL)
 * @param len          Number of bytes
 * @param timeout_ms   Timeout for completion
 *
 * @return Operation result code
 */
int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms);

/**
 * @brief Blocking SPI transmit-only operation.
 *
 * @param bus_id       SPI bus index
 * @param data         TX buffer
 * @param len          Number of bytes
 * @param timeout_ms   Timeout for completion
 *
 * @return Operation result code
 */
int32_t spi_mgmt_sync_transmit(uint8_t bus_id,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);

/**
 * @brief Blocking SPI receive-only operation.
 *
 * @param bus_id       SPI bus index
 * @param data         RX buffer
 * @param len          Number of bytes
 * @param timeout_ms   Timeout for completion
 *
 * @return Operation result code
 */
int32_t spi_mgmt_sync_receive(uint8_t bus_id,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);

/**
 * @brief Non-blocking SPI transmit-only operation.
 *
 * @note Buffer must remain valid until transaction completes.
 *
 * @return OS_ERR_NONE if queued successfully
 */
int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len);

/**
 * @brief Non-blocking full-duplex SPI transfer.
 *
 * @note TX and RX buffers must remain valid until completion.
 *
 * @return OS_ERR_NONE if queued successfully
 */
int32_t spi_mgmt_async_transfer(uint8_t bus_id,
                                 const uint8_t *tx, uint8_t *rx,
                                 uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_SPI_MGMT_H_ */