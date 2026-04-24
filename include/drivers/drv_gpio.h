/*
 * drv_gpio.h — Generic GPIO driver types, ops table, handle and public API
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef INCLUDE_DRIVERS_DRV_GPIO_H_
#define INCLUDE_DRIVERS_DRV_GPIO_H_

#include <def_std.h>
#include <def_err.h>
#include <board/mcu_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */

/* ── Forward declaration ───────────────────────────────────────────────── */
typedef struct drv_gpio_handle drv_gpio_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      mode;         /* GPIO_MODE_OUTPUT_PP, GPIO_MODE_AF_PP, … */
    uint32_t      pull;         /* GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN */
    uint32_t      speed;        /* GPIO_SPEED_FREQ_LOW … _VERY_HIGH */
    uint8_t       active_state; /* GPIO_PIN_SET or GPIO_PIN_RESET */
} drv_hw_gpio_ctx_t;

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
typedef struct {
    uint32_t port_base;
    uint32_t pin;
    uint32_t mode;
    uint32_t pull;
    uint32_t speed;
    uint8_t  active_state;
} drv_hw_gpio_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
typedef struct drv_gpio_hal_ops {
    int32_t (*hw_init)  (drv_gpio_handle_t *h);
    int32_t (*hw_deinit)(drv_gpio_handle_t *h);
    void    (*set)      (drv_gpio_handle_t *h);
    void    (*clear)    (drv_gpio_handle_t *h);
    void    (*toggle)   (drv_gpio_handle_t *h);
    uint8_t (*read)     (drv_gpio_handle_t *h);
    void    (*write)    (drv_gpio_handle_t *h, uint8_t state);
} drv_gpio_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
struct drv_gpio_handle {
    uint8_t                   dev_id;
    uint8_t                   initialized;
    drv_hw_gpio_ctx_t         hw;
    const drv_gpio_hal_ops_t *ops;
    int32_t                   last_err;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
#define DRV_GPIO_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

int32_t            drv_gpio_register  (uint8_t dev_id,
                                        const drv_gpio_hal_ops_t *ops);
drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_GPIO_H_ */
