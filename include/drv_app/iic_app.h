/*
 * iic_app.h — Application-level I2C API
 *
 * Thin wrappers over iic_mgmt that expose a clean sync / async interface.
 *
 * Register access convention
 * ──────────────────────────
 *   use_reg = 1   → the driver sends reg_addr as the first byte before data.
 *   use_reg = 0   → raw transfer; reg_addr is ignored.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_APP_IIC_APP_H_
#define FREERTOS_OS_INCLUDE_DRV_APP_IIC_APP_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Synchronous (blocking) ───────────────────────────────────────────── */

/**
 * @brief  Write bytes to an I2C device and block until completion.
 *
 * @param  bus_id      I2C bus index.
 * @param  dev_addr    7-bit I2C device address.
 * @param  reg_addr    Register address (used when use_reg != 0).
 * @param  use_reg     Non-zero to send reg_addr before data.
 * @param  data        Data to write.
 * @param  len         Number of bytes.
 * @param  timeout_ms  Maximum wait time (queue + HAL).
 * @return OS_ERR_NONE on success.
 */
int32_t iic_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           const uint8_t *data, uint16_t len,
                           uint32_t timeout_ms);

/**
 * @brief  Read bytes from an I2C device and block until completion.
 */
int32_t iic_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                          uint8_t reg_addr, uint8_t use_reg,
                          uint8_t *data, uint16_t len,
                          uint32_t timeout_ms);

/**
 * @brief  Check if a device ACKs on the bus (blocking).
 * @return OS_ERR_NONE if device present, OS_ERR_OP if no ACK or timeout.
 */
int32_t iic_sync_probe(uint8_t bus_id, uint16_t dev_addr, uint32_t timeout_ms);

/* ── Asynchronous (non-blocking) ─────────────────────────────────────── */

/**
 * @brief  Queue an I2C write and return immediately (no result).
 *
 * Data buffer must stay valid until the management thread processes it.
 */
int32_t iic_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                            uint8_t reg_addr, uint8_t use_reg,
                            const uint8_t *data, uint16_t len);

/**
 * @brief  Queue an I2C read and return immediately (no result).
 *
 * Data buffer must stay valid until the management thread processes it.
 * Use iic_sync_receive() when you need to act on the result.
 */
int32_t iic_async_receive(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_APP_IIC_APP_H_ */
