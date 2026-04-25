/**
 * @file    iic_mgmt.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   I2C management service interface
 *
 * @details
 * This module provides a centralized I2C transaction manager running as a
 * FreeRTOS service thread.
 *
 * All I2C operations (transmit, receive, probe, re-init) are serialized
 * through a single queue to ensure:
 *
 *   - Bus arbitration safety across multiple tasks
 *   - Deterministic execution order of I2C transactions
 *   - Optional synchronous or asynchronous execution modes
 *
 * Architecture overview:
 *
 *   Application Task
 *        │
 *        ▼
 *   iic_mgmt_* API (async / sync)
 *        │
 *        ▼
 *   FreeRTOS Queue (iic_mgmt_msg_t)
 *        │
 *        ▼
 *   I2C Management Thread
 *        │
 *        ├── Blocking HAL (fallback)
 *        └── Interrupt-driven HAL (preferred)
 *                │
 *                ▼
 *         ISR completion callback
 *                │
 *                ▼
 *     task notification (vTaskNotifyGive)
 *
 * Synchronous API uses task notifications to wait for completion.
 * Asynchronous API only queues the request without blocking.
 */

#ifndef DRIVERS_MGMT_IIC_MGMT_H_
#define DRIVERS_MGMT_IIC_MGMT_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <config/conf_os.h>
#include <services/iic_mgmt.h>
#include <os/kernel.h>
#include <os/kernel_mem.h>

#include <drivers/drv_iic.h>
#include <board/board_config.h>
#include <board/mcu_config.h>
#include <irq/irq_notify.h>

/* ────────────────────────────────────────────────────────────────────────── */
/*                          I2C Management Commands                         */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief I2C operation types supported by the management service.
 */
typedef enum {
    IIC_MGMT_CMD_TRANSMIT = 0,  /**< Write data to I2C device              */
    IIC_MGMT_CMD_RECEIVE,       /**< Read data from I2C device             */
    IIC_MGMT_CMD_PROBE,         /**< Check device ACK response             */
    IIC_MGMT_CMD_REINIT,        /**< Reinitialize I2C peripheral           */
} iic_mgmt_cmd_t;

/* ────────────────────────────────────────────────────────────────────────── */
/*                           I2C Request Message                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @struct iic_mgmt_msg_t
 * @brief  Message descriptor for I2C management queue.
 *
 * @details
 * Each message represents a single I2C transaction request. The management
 * thread processes these sequentially to ensure bus safety.
 *
 * @note
 * - If result_notify is non-NULL, the calling task is notified on completion.
 * - If result_code is non-NULL, it receives the driver return status.
 * - For async APIs, both fields are NULL.
 */
typedef struct {
    iic_mgmt_cmd_t   cmd;            /**< I2C operation type                 */
    uint8_t          bus_id;         /**< Logical I2C bus index              */
    uint16_t         dev_addr;       /**< 7-bit device address               */
    uint8_t          reg_addr;       /**< Register address (optional)        */
    uint8_t          use_reg;        /**< Enable register-based access       */
    uint8_t         *data;           /**< TX buffer / RX buffer              */
    uint16_t         len;            /**< Number of bytes                    */
    TaskHandle_t     result_notify;  /**< Task to notify on completion       */
    int32_t         *result_code;    /**< Pointer to store operation result  */
} iic_mgmt_msg_t;


/* ────────────────────────────────────────────────────────────────────────── */
/*                              Public API                                   */
/* ────────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start I2C management thread.
 * @return OS_ERR_NONE on success, error code otherwise.
 */
int32_t iic_mgmt_start(void);

/**
 * @brief Get underlying FreeRTOS queue handle.
 * @return QueueHandle_t for direct message posting.
 */
QueueHandle_t iic_mgmt_get_queue(void);

/**
 * @brief Submit asynchronous I2C transmit request.
 *
 * @note Non-blocking. Caller does not receive completion status.
 */
int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len);

/**
 * @brief Submit synchronous I2C transmit request.
 *
 * @details Blocks until operation completes or timeout occurs.
 * @return Driver execution result.
 */
int32_t iic_mgmt_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);

/**
 * @brief Submit synchronous I2C receive request.
 *
 * @details Blocks until data is received or timeout occurs.
 * @return Driver execution result.
 */
int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);

/**
 * @brief Probe I2C device presence (blocking).
 *
 * @return OS_ERR_NONE if device ACKs, otherwise OS_ERR_OP.
 */
int32_t iic_mgmt_sync_probe(uint8_t bus_id, uint16_t dev_addr,
                             uint32_t timeout_ms);

/**
 * @brief Submit asynchronous I2C receive request.
 *
 * @note Buffer must remain valid until transaction is processed.
 */
int32_t iic_mgmt_async_receive(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_IIC_MGMT_H_ */