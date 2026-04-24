/*
 * drv_time.h — Timer driver types, ops table, handle and public API
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_
#define OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_

#include <def_std.h>
#include <def_err.h>
#include <board/mcu_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */

/* ── Forward declaration ───────────────────────────────────────────────── */
typedef struct drv_timer_handle drv_timer_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#include <device.h>
#ifdef HAL_TIM_MODULE_ENABLED
typedef struct { TIM_HandleTypeDef htim; } drv_hw_timer_ctx_t;
#else
typedef struct { uint32_t _reserved; } drv_hw_timer_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
typedef struct {
    uint32_t timer_base;
    uint32_t period;
} drv_hw_timer_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
typedef struct drv_timer_hal_ops {
    int32_t  (*hw_init)    (drv_timer_handle_t *h);
    int32_t  (*hw_deinit)  (drv_timer_handle_t *h);
    uint32_t (*get_counter)(drv_timer_handle_t *h);
    void     (*set_counter)(drv_timer_handle_t *h, uint32_t val);
} drv_timer_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
struct drv_timer_handle {
    uint8_t                    dev_id;
    uint8_t                    initialized;
    drv_hw_timer_ctx_t         hw;
    const drv_timer_hal_ops_t *ops;
    int32_t                    last_err;
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
#define DRV_TIMER_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

/* Tick utilities — always available, no handle required */
extern volatile uint32_t g_ms_ticks;
uint32_t drv_time_get_ticks(void);
void     drv_time_delay_ms(uint32_t ms);

#if (NO_OF_TIMER > 0)
int32_t             drv_timer_register  (uint8_t dev_id,
                                          const drv_timer_hal_ops_t *ops);
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id);
uint32_t            drv_timer_get_counter(uint8_t dev_id);
void                drv_timer_set_counter(uint8_t dev_id, uint32_t val);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_ */
