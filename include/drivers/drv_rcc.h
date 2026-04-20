/*
 * drv_rcc.h — Reset and Clock Control driver public API
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Portable interface to the clock initialisation subsystem.  The vendor-
 * specific HAL backend (hal_rcc_stm32.c, etc.) is selected at compile time
 * via CONFIG_DEVICE_VARIANT and registers its ops table inside
 * drv_rcc_clock_init().  Callers never need to include a vendor header.
 *
 * Call sequence (from main, before the scheduler):
 *   HAL_Init();
 *   drv_rcc_clock_init();   ← configures PLL, flash latency, bus dividers
 */

#ifndef OS_DRIVERS_DRV_RCC_H_
#define OS_DRIVERS_DRV_RCC_H_

#include <stdint.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── HAL ops table — implemented by each vendor backend ─────────────────── */

typedef struct {
    int32_t  (*clock_init)(void);      /* configure PLL, flash latency, dividers */
    uint32_t (*get_sysclk_hz)(void);   /* return current SYSCLK in Hz            */
    uint32_t (*get_apb1_hz)(void);     /* return current APB1 clock in Hz         */
    uint32_t (*get_apb2_hz)(void);     /* return current APB2 clock in Hz         */
} drv_rcc_hal_ops_t;

/* ── Driver API ──────────────────────────────────────────────────────────── */

/* Register a vendor ops table.  Called internally by drv_rcc_clock_init(). */
int32_t drv_rcc_register(const drv_rcc_hal_ops_t *ops);

/* Configure system clocks.  Must be called once from main() after HAL_Init().
 * Internally selects and registers the vendor backend, then runs clock init. */
int32_t drv_rcc_clock_init(void);

/* Query current clock rates (valid after drv_rcc_clock_init() returns). */
uint32_t drv_rcc_get_sysclk_hz(void);
uint32_t drv_rcc_get_apb1_hz(void);
uint32_t drv_rcc_get_apb2_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DRV_RCC_H_ */
