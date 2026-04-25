/**
 * @file    uart_mgmt.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   UART management service thread interface
 *
 * @details
 * This module defines the UART management service layer for FreeRTOS-OS.
 * The UART management thread owns and serialises access to all UART
 * peripherals in the system through a single message queue.
 *
 * Responsibilities:
 *   - Peripheral registration and hardware initialisation at startup.
 *   - Asynchronous UART transmission requests via queue-based messaging.
 *   - UART recovery handling through reinitialisation on error detection.
 *   - RX byte retrieval through per-device ring buffers.
 *
 * Execution model:
 *   - Upper layers never access drv_uart directly for TX.
 *   - All UART operations are routed via uart_mgmt_msg_t queue messages.
 *   - RX data is pushed via ISR into ringbuffers and consumed non-blocking.
 *
 * Typical usage:
 *   - Call uart_mgmt_start() during system initialisation.
 *   - Use uart_mgmt_get_queue() to submit UART requests.
 */

#ifndef DRIVERS_MGMT_UART_MGMT_H_
#define DRIVERS_MGMT_UART_MGMT_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <services/uart_mgmt.h>
#include <os/kernel.h>
#include <drivers/drv_uart.h>
#include <board/board_config.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>
#include <irq/irq_notify.h>

/* ────────────────────────────────────────────────────────────────────────────
 * Message types
 * ────────────────────────────────────────────────────────────────────────── */

/**
 * @enum uart_mgmt_cmd_t
 * @brief UART management command types processed by the service thread.
 */
typedef enum {
    UART_MGMT_CMD_TRANSMIT = 0,  /**< Transmit data asynchronously */
    UART_MGMT_CMD_REINIT,        /**< Reinitialize UART peripheral  */
    UART_MGMT_CMD_DEINIT,        /**< Deinitialize UART peripheral  */
} uart_mgmt_cmd_t;

/* ────────────────────────────────────────────────────────────────────────────
 * Message structure
 * ────────────────────────────────────────────────────────────────────────── */

/**
 * @struct uart_mgmt_msg_t
 * @brief Message structure posted to the UART management queue.
 *
 * @details
 * This structure represents a request sent to the UART management thread.
 *
 * For UART_MGMT_CMD_TRANSMIT:
 *   - dev_id must specify the UART instance.
 *   - data points to a valid buffer that must remain valid until transmission.
 *   - len defines the number of bytes to transmit.
 *
 * Optional synchronous support:
 *   - result_notify: task to notify upon completion.
 *   - result_code: pointer where the return status is stored.
 */
typedef struct {
    uart_mgmt_cmd_t  cmd;            /**< Command type */
    uint8_t          dev_id;         /**< UART device index */
    const uint8_t   *data;           /**< TX buffer pointer */
    uint16_t         len;            /**< TX length in bytes */
    TaskHandle_t     result_notify;   /**< Task to notify (NULL = async) */
    int32_t         *result_code;     /**< Pointer to return status */
} uart_mgmt_msg_t;

/* ────────────────────────────────────────────────────────────────────────────
 * Configuration
 * ────────────────────────────────────────────────────────────────────────── */

#ifndef UART_MGMT_QUEUE_DEPTH
#define UART_MGMT_QUEUE_DEPTH   16   /**< Depth of UART management queue */
#endif

/* ────────────────────────────────────────────────────────────────────────────
 * Public API
 * ────────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the UART management service thread.
 *
 * @details
 * This function creates the UART management thread and its queue.
 * Hardware initialization is performed after the scheduler starts,
 * following the configured startup delay.
 *
 * @return OS_ERR_NONE on success, or OS_ERR_* on failure.
 */
int32_t uart_mgmt_start(void);

/**
 * @brief Get UART management queue handle.
 *
 * @details
 * Used by upper layers to submit uart_mgmt_msg_t requests to the
 * UART management thread.
 *
 * @return FreeRTOS queue handle.
 */
QueueHandle_t uart_mgmt_get_queue(void);

/**
 * @brief Submit an asynchronous UART transmit request.
 *
 * @details
 * Non-blocking API. The message is queued and processed later by the
 * UART management thread.
 *
 * @param dev_id UART device index.
 * @param data   Pointer to transmit buffer (must remain valid until sent).
 * @param len    Number of bytes to transmit.
 *
 * @return OS_ERR_NONE if queued successfully, OS_ERR_OP otherwise.
 */
int32_t uart_mgmt_async_transmit(uint8_t dev_id,
                                 const uint8_t *data,
                                 uint16_t len);

/**
 * @brief Read one byte from UART RX ring buffer.
 *
 * @details
 * Non-blocking access to the per-UART RX buffer filled by ISR callbacks.
 *
 * @param dev_id UART device index.
 * @param byte   Output pointer to store received byte.
 *
 * @return OS_ERR_NONE if a byte is available, OS_ERR_OP otherwise.
 */
int32_t uart_mgmt_read_byte(uint8_t dev_id, uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_UART_MGMT_H_ */