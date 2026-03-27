/*
 * iic_mgmt.h — I2C management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The I2C management thread serialises all I2C bus transactions through a
 * single FreeRTOS queue, ensuring that concurrent tasks do not corrupt bus
 * state.  Callers post iic_mgmt_msg_t requests and optionally provide a
 * notification handle to receive the result asynchronously.
 */

#ifndef DRIVERS_MGMT_IIC_MGMT_H_
#define DRIVERS_MGMT_IIC_MGMT_H_

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
    IIC_MGMT_CMD_TRANSMIT = 0,  /**< Write bytes to a device              */
    IIC_MGMT_CMD_RECEIVE,       /**< Read bytes from a device             */
    IIC_MGMT_CMD_PROBE,         /**< Check if a device responds           */
    IIC_MGMT_CMD_REINIT,        /**< Re-initialise the bus                */
} iic_mgmt_cmd_t;

/**
 * @struct iic_mgmt_msg_t
 * @brief  Message posted to the I2C management thread queue.
 *
 * @note   result_notify — if non-NULL the management thread calls
 *         xTaskNotifyGive() on this handle after completing the operation,
 *         and stores the return code in *result_code (if non-NULL).
 *         The caller may then ulTaskNotifyTake() to synchronise.
 */
typedef struct {
    iic_mgmt_cmd_t   cmd;
    uint8_t          bus_id;        /**< I2C bus index (0-based)           */
    uint16_t         dev_addr;      /**< 7-bit I2C device address          */
    uint8_t          reg_addr;      /**< Register address (if use_reg)     */
    uint8_t          use_reg;       /**< Non-zero to perform a reg access  */
    uint8_t         *data;          /**< TX data (TRANSMIT) / RX buf (RX) */
    uint16_t         len;           /**< Byte count                        */
    TaskHandle_t     result_notify; /**< Task to notify on completion      */
    int32_t         *result_code;   /**< Pointer to store the return code  */
} iic_mgmt_msg_t;

#ifndef IIC_MGMT_QUEUE_DEPTH
#define IIC_MGMT_QUEUE_DEPTH  8
#endif

/* ── Public API ───────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

int32_t       iic_mgmt_start(void);
QueueHandle_t iic_mgmt_get_queue(void);

/**
 * @brief  Post an I2C write request (non-blocking).
 * @return OS_ERR_NONE if queued, OS_ERR_OP if queue full.
 */
int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len);

/**
 * @brief  Post an I2C read request and wait for completion (blocking).
 * @return Return code from the HAL operation.
 */
int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_IIC_MGMT_H_ */
