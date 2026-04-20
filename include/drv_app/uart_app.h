/*
 * uart_app.h — Application-level UART API
 *
 * Thin wrappers over uart_mgmt that expose a clean sync / async interface
 * without requiring the caller to know about management queues or FreeRTOS
 * notification handles.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_APP_UART_APP_H_
#define FREERTOS_OS_INCLUDE_DRV_APP_UART_APP_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Synchronous (blocking) ───────────────────────────────────────────── */

/**
 * @brief  Transmit data over UART and block until the transfer completes.
 *
 * Posts a transmit request to the UART management queue and waits for
 * the management thread to notify completion via task notification.
 *
 * @param  dev_id      UART device index (from board_device_ids.h).
 * @param  data        Pointer to the data buffer.
 * @param  len         Number of bytes to transmit.
 * @param  timeout_ms  Maximum time to wait for queue + completion.
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure or timeout.
 */
int32_t uart_sync_transmit(uint8_t dev_id, const uint8_t *data,
                            uint16_t len, uint32_t timeout_ms);

/**
 * @brief  Receive exactly @p len bytes from the UART RX ring buffer,
 *         blocking until all bytes arrive or timeout expires.
 *
 * Polls the RX ring buffer in 1 ms ticks. Suitable for low-to-medium
 * baud rates where byte arrival is reliably within the OS tick period.
 *
 * @param  dev_id      UART device index.
 * @param  buf         Buffer to store received bytes.
 * @param  len         Number of bytes to receive.
 * @param  timeout_ms  Maximum total wait time.
 * @return OS_ERR_NONE if all bytes received, OS_ERR_OP on timeout.
 */
int32_t uart_sync_receive(uint8_t dev_id, uint8_t *buf,
                           uint16_t len, uint32_t timeout_ms);

/* ── Asynchronous (non-blocking) ─────────────────────────────────────── */

/**
 * @brief  Post a transmit request and return immediately.
 *
 * The data buffer must remain valid until the management thread drains
 * the queue (use a static or long-lived buffer).
 *
 * @return OS_ERR_NONE if queued, OS_ERR_OP if queue is full.
 */
int32_t uart_async_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);

/**
 * @brief  Read one byte from the RX ring buffer without blocking.
 *
 * @param  byte  Output: the received byte (valid only when returning OS_ERR_NONE).
 * @return OS_ERR_NONE if a byte was available, OS_ERR_OP if buffer empty.
 */
int32_t uart_async_read_byte(uint8_t dev_id, uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_APP_UART_APP_H_ */
