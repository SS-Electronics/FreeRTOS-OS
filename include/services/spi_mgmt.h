/*
 * spi_mgmt.h — SPI management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The SPI management thread serialises all SPI bus transactions.  Because
 * SPI is full-duplex the message carries both a TX and RX buffer; callers
 * may set either to NULL for transmit-only or receive-only operations.
 *
 * Chip-select (CS) control is the caller's responsibility — the SPI
 * management layer does not manage CS GPIO pins.  Assert CS before posting
 * a message and deassert it after receiving the completion notification.
 */

#ifndef DRIVERS_MGMT_SPI_MGMT_H_
#define DRIVERS_MGMT_SPI_MGMT_H_

#include <def_std.h>
#include <def_err.h>
#include <config/mcu_config.h>
#include <config/os_config.h>
#include <drivers/drv_handle.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* ── Message types ────────────────────────────────────────────────────── */

typedef enum {
    SPI_MGMT_CMD_TRANSMIT = 0,
    SPI_MGMT_CMD_RECEIVE,
    SPI_MGMT_CMD_TRANSFER,   /**< Full-duplex TX+RX simultaneously        */
    SPI_MGMT_CMD_REINIT,
} spi_mgmt_cmd_t;

typedef struct {
    spi_mgmt_cmd_t   cmd;
    uint8_t          bus_id;
    const uint8_t   *tx_data;       /**< TX buffer (TRANSMIT / TRANSFER)   */
    uint8_t         *rx_data;       /**< RX buffer (RECEIVE / TRANSFER)    */
    uint16_t         len;
    TaskHandle_t     result_notify;
    int32_t         *result_code;
} spi_mgmt_msg_t;

#ifndef SPI_MGMT_QUEUE_DEPTH
#define SPI_MGMT_QUEUE_DEPTH  8
#endif

/* Stack / priority for this service (add to os_config.h if not present) */
#ifndef PROC_SERVICE_SPI_MGMT_STACK_SIZE
#define PROC_SERVICE_SPI_MGMT_STACK_SIZE   512
#endif
#ifndef PROC_SERVICE_SPI_MGMT_PRIORITY
#define PROC_SERVICE_SPI_MGMT_PRIORITY     1
#endif
#ifndef TIME_OFFSET_SPI_MANAGEMENT
#define TIME_OFFSET_SPI_MANAGEMENT         5500
#endif

/* ── Public API ───────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

int32_t       spi_mgmt_start(void);
QueueHandle_t spi_mgmt_get_queue(void);

int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms);

int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_SPI_MGMT_H_ */
