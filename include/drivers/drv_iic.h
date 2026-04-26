/*
File:        drv_iic.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:      drivers
Info:        Generic I2C driver interface with HAL abstraction              
Dependency:  def_std.h, def_err.h, board/mcu_config.h, FreeRTOS.h, task.h

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file    drv_iic.h
 * @brief   Generic I2C driver interface with HAL abstraction
 *
 * @details
 * This file defines the interface for a vendor-agnostic I2C driver used
 * in the FreeRTOS-OS project.
 *
 * Features:
 * - Blocking and interrupt-driven (IT) transfers
 * - Register-based and raw transfers
 * - Device probing (ACK check)
 * - HAL abstraction via drv_iic_hal_ops_t
 *
 * Architecture:
 * Application → Driver API → HAL Ops → Vendor HAL → Hardware
 *
 * Each I2C instance is represented by drv_iic_handle_t and bound
 * to a hardware peripheral through the HAL layer.
 */

#ifndef INCLUDE_DRIVERS_COM_DRV_IIC_H_
#define INCLUDE_DRIVERS_COM_DRV_IIC_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <board/board_handles.h>
#include <os/kernel.h>

/* ── Forward declaration ───────────────────────────────────────────────── */
/**
 * @brief I2C driver handle
 */
typedef struct drv_iic_handle drv_iic_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
/**
 * @brief Vendor-specific I2C hardware context
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef HAL_I2C_MODULE_ENABLED
typedef struct {
    I2C_HandleTypeDef hi2c;
} drv_hw_iic_ctx_t;
#else
typedef struct {
    uint32_t _reserved;
} drv_hw_iic_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

typedef struct {
    uint32_t channel_base;
    uint32_t clock_hz;
} drv_hw_iic_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check board_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
/**
 * @brief I2C HAL operations table
 *
 * @details
 * Abstracts all hardware-specific I2C operations.
 */
typedef struct drv_iic_hal_ops {

    int32_t (*hw_init)(drv_iic_handle_t *h);
    int32_t (*hw_deinit)(drv_iic_handle_t *h);

    int32_t (*transmit)(drv_iic_handle_t *h,
                        uint16_t dev_addr,
                        uint8_t reg_addr,
                        uint8_t use_reg,
                        const uint8_t *data,
                        uint16_t len,
                        uint32_t timeout_ms);

    int32_t (*receive)(drv_iic_handle_t *h,
                       uint16_t dev_addr,
                       uint8_t reg_addr,
                       uint8_t use_reg,
                       uint8_t *data,
                       uint16_t len,
                       uint32_t timeout_ms);

    int32_t (*is_device_ready)(drv_iic_handle_t *h,
                               uint16_t dev_addr,
                               uint32_t timeout_ms);

    /* Interrupt-driven variants */
    int32_t (*transmit_it)(drv_iic_handle_t *h,
                           uint16_t dev_addr,
                           uint8_t reg_addr,
                           uint8_t use_reg,
                           const uint8_t *data,
                           uint16_t len);

    int32_t (*receive_it)(drv_iic_handle_t *h,
                          uint16_t dev_addr,
                          uint8_t reg_addr,
                          uint8_t use_reg,
                          uint8_t *data,
                          uint16_t len);

} drv_iic_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
/**
 * @brief I2C driver handle structure
 */
struct drv_iic_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_iic_ctx_t         hw;
    const drv_iic_hal_ops_t *ops;
    uint32_t                 clock_hz;
    uint32_t                 timeout_ms;
    int32_t                  last_err;
    TaskHandle_t             notify_task;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
/**
 * @brief Initialize I2C handle
 */
#define DRV_IIC_HANDLE_INIT(_id, _clk, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .clock_hz = (_clk), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

#if (BOARD_IIC_COUNT > 0)

/**
 * @brief Register I2C device
 */
int32_t drv_iic_register(uint8_t dev_id,
                         const drv_iic_hal_ops_t *ops,
                         uint32_t clock_hz,
                         uint32_t timeout_ms);

/**
 * @brief Get I2C handle
 */
drv_iic_handle_t *drv_iic_get_handle(uint8_t dev_id);

/**
 * @brief Transmit data to device register
 */
int32_t drv_iic_transmit(uint8_t dev_id,
                         uint16_t dev_addr,
                         uint8_t reg_addr,
                         const uint8_t *data,
                         uint16_t len);

/**
 * @brief Receive data from device register
 */
int32_t drv_iic_receive(uint8_t dev_id,
                        uint16_t dev_addr,
                        uint8_t reg_addr,
                        uint8_t *data,
                        uint16_t len);

/**
 * @brief Check if device is ready (ACK)
 */
int32_t drv_iic_device_ready(uint8_t dev_id,
                             uint16_t dev_addr);

#endif /* BOARD_IIC_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_COM_DRV_IIC_H_ */