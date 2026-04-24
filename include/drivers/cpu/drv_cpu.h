/*
 * drv_cpu.h — Watchdog driver types, ops table, handle and public API
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_
#define OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_

#include <def_std.h>
#include <def_err.h>
#include <board/mcu_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */

/* ── Forward declaration ───────────────────────────────────────────────── */
typedef struct drv_wdg_handle drv_wdg_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>
#ifdef HAL_IWDG_MODULE_ENABLED
typedef struct { IWDG_HandleTypeDef hiwdg; } drv_hw_wdg_ctx_t;
#else
typedef struct { uint32_t _reserved; } drv_hw_wdg_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
typedef struct {
    uint32_t wdg_base;
    uint32_t timeout_ms;
} drv_hw_wdg_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
typedef struct drv_wdg_hal_ops {
    int32_t (*hw_init)  (drv_wdg_handle_t *h);
    int32_t (*hw_deinit)(drv_wdg_handle_t *h);
    void    (*refresh)  (drv_wdg_handle_t *h);
} drv_wdg_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
struct drv_wdg_handle {
    uint8_t                  initialized;
    drv_hw_wdg_ctx_t         hw;
    const drv_wdg_hal_ops_t *ops;
    int32_t                  last_err;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
#define DRV_WDG_HANDLE_INIT() \
    { .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

int32_t           drv_wdg_register  (const drv_wdg_hal_ops_t *ops);
drv_wdg_handle_t *drv_wdg_get_handle(void);
void              drv_wdg_refresh   (void);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_ */
