/*
 * uart_mgmt.h — UART management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The UART management thread owns all drv_uart_handle_t instances for the
 * system.  It:
 *   • Registers and initialises each enabled UART peripheral at startup.
 *   • Accepts asynchronous transmit requests via a FreeRTOS queue.
 *   • Monitors the last_err field and attempts peripheral recovery on error.
 *
 * Usage
 * ─────
 *   Include this header and call uart_mgmt_start() from the OS init sequence.
 *   Upper layers post uart_mgmt_msg_t messages to the management queue
 *   (uart_mgmt_get_queue()) to request transmissions without blocking the
 *   calling thread on the UART bus.
 */

#ifndef DRIVERS_MGMT_UART_MGMT_H_
#define DRIVERS_MGMT_UART_MGMT_H_

#include <def_std.h>
#include <def_err.h>
#include <config/mcu_config.h>
#include <config/os_config.h>
#include <drivers/drv_handle.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* ── Message types ────────────────────────────────────────────────────── */

typedef enum {
    UART_MGMT_CMD_TRANSMIT = 0,  /**< Async transmit request               */
    UART_MGMT_CMD_REINIT,        /**< Re-initialise peripheral after error  */
    UART_MGMT_CMD_DEINIT,        /**< Graceful shutdown of a UART device    */
} uart_mgmt_cmd_t;

/**
 * @struct uart_mgmt_msg_t
 * @brief  Message posted to the management thread queue.
 *
 * For UART_MGMT_CMD_TRANSMIT: fill dev_id, data pointer, and len.
 * The data buffer must remain valid until the management thread drains the
 * message (use a static buffer or a heap allocation the caller frees later).
 */
typedef struct {
    uart_mgmt_cmd_t  cmd;
    uint8_t          dev_id;
    const uint8_t   *data;
    uint16_t         len;
    TaskHandle_t     result_notify; /**< Task to notify on completion (NULL = async) */
    int32_t         *result_code;   /**< Where to store the return code (NULL = ignore) */
} uart_mgmt_msg_t;

/* ── Management queue depth ───────────────────────────────────────────── */

#ifndef UART_MGMT_QUEUE_DEPTH
#define UART_MGMT_QUEUE_DEPTH   16
#endif

/* ── Public API ───────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Create and start the UART management thread.
 *
 * Call this once from the OS init sequence, before the FreeRTOS scheduler
 * starts.  The thread will perform hardware initialisation after the
 * scheduler starts (after the startup delay defined in os_config.h).
 *
 * @return OS_ERR_NONE on success, negative OS_ERR_* on failure.
 */
int32_t uart_mgmt_start(void);

/**
 * @brief  Return the management queue handle.
 *
 * Upper layers post uart_mgmt_msg_t items to this queue to request
 * asynchronous UART operations.
 */
QueueHandle_t uart_mgmt_get_queue(void);

/**
 * @brief  Post an async transmit request to the management queue.
 *
 * Non-blocking.  Returns OS_ERR_OP if the queue is full.
 *
 * @param  dev_id  UART device index.
 * @param  data    Pointer to the data buffer (must remain valid until sent).
 * @param  len     Number of bytes.
 */
int32_t uart_mgmt_async_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);

/**
 * @brief  Read one byte from the UART RX ring buffer.
 *
 * Non-blocking.  Returns OS_ERR_NONE and stores the byte at *byte when one
 * is available, or OS_ERR_OP when the buffer is empty.
 *
 * @param  dev_id  UART device index.
 * @param  byte    Output pointer for the received byte.
 */
int32_t uart_mgmt_read_byte(uint8_t dev_id, uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_UART_MGMT_H_ */
