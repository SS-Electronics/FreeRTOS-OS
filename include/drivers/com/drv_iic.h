/*
 * drv_iic.h — Generic I2C driver types, ops table, handle and public API
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef INCLUDE_DRIVERS_COM_DRV_IIC_H_
#define INCLUDE_DRIVERS_COM_DRV_IIC_H_

#include <def_std.h>
#include <def_err.h>
#include <board/mcu_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */
#include <FreeRTOS.h>           /* TaskHandle_t for IT-based notify  */
#include <task.h>

/* ── Forward declaration ───────────────────────────────────────────────── */
typedef struct drv_iic_handle drv_iic_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>
#ifdef HAL_I2C_MODULE_ENABLED
typedef struct { I2C_HandleTypeDef hi2c; } drv_hw_iic_ctx_t;
#else
typedef struct { uint32_t _reserved; } drv_hw_iic_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
typedef struct {
    uint32_t channel_base;
    uint32_t clock_hz;
} drv_hw_iic_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
typedef struct drv_iic_hal_ops {
    int32_t (*hw_init)        (drv_iic_handle_t *h);
    int32_t (*hw_deinit)      (drv_iic_handle_t *h);
    int32_t (*transmit)       (drv_iic_handle_t *h, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);
    int32_t (*receive)        (drv_iic_handle_t *h, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);
    int32_t (*is_device_ready)(drv_iic_handle_t *h, uint16_t dev_addr,
                                uint32_t timeout_ms);
    /* IT variants — completion signalled via IRQ_ID_I2C_TX/RX_DONE */
    int32_t (*transmit_it)    (drv_iic_handle_t *h, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                const uint8_t *data, uint16_t len);
    int32_t (*receive_it)     (drv_iic_handle_t *h, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                uint8_t *data, uint16_t len);
} drv_iic_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
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
#define DRV_IIC_HANDLE_INIT(_id, _clk, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .clock_hz = (_clk), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

#if (BOARD_IIC_COUNT > 0)

int32_t           drv_iic_register     (uint8_t dev_id,
                                         const drv_iic_hal_ops_t *ops,
                                         uint32_t clock_hz,
                                         uint32_t timeout_ms);
drv_iic_handle_t *drv_iic_get_handle  (uint8_t dev_id);

int32_t           drv_iic_transmit     (uint8_t dev_id, uint16_t dev_addr,
                                         uint8_t reg_addr,
                                         const uint8_t *data, uint16_t len);
int32_t           drv_iic_receive      (uint8_t dev_id, uint16_t dev_addr,
                                         uint8_t reg_addr,
                                         uint8_t *data, uint16_t len);
int32_t           drv_iic_is_device_ready(uint8_t dev_id, uint16_t dev_addr,
                                           uint32_t timeout_ms);

#endif /* BOARD_IIC_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_COM_DRV_IIC_H_ */
