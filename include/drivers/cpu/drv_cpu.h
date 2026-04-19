/*
 * drv_cpu.h — CPU-level driver public API
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_
#define OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_

#include <drivers/drv_handle.h>
#include <config/mcu_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CPU / NVIC utilities */
void drv_cpu_interrupt_prio_set(void);
void reset_mcu(void);

/* Watchdog — always declared; stubs compile to nothing when WDG is disabled */
int32_t           drv_wdg_register(const drv_wdg_hal_ops_t *ops);
drv_wdg_handle_t *drv_wdg_get_handle(void);
void              drv_wdg_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_ */
