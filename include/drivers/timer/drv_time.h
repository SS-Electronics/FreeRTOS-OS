/*
 * drv_time.h — Generic timer / systick driver public API
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_
#define OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_

#include <drivers/drv_handle.h>
#include <config/mcu_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Systick-based utilities — always available, no handle required */
uint32_t drv_time_get_ticks(void);
void     drv_time_delay_ms(uint32_t ms);

#if (NO_OF_TIMER > 0)
int32_t             drv_timer_register  (uint8_t dev_id, const drv_timer_hal_ops_t *ops);
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id);
uint32_t            drv_timer_get_counter(uint8_t dev_id);
void                drv_timer_set_counter(uint8_t dev_id, uint32_t val);
#endif

#ifdef __cplusplus
}
#endif

#endif /* OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_ */
