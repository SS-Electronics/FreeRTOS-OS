/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS Basic Software
#
# RTOS Basic Software is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS Basic Software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#ifndef OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_
#define OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_

#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
/* kernel include for event usage */
#include <bsw/kernel/kernel.h>
#include <bsw/kernel/COMM_STACK/canopen-stack/src/core/co_core.h>
/*IPC include */
#include <bsw/ipc/events/signal.h>
/* Global data include  */
#include <bsw/ipc/global/global_var.h>
/* Low level driver include */
#include <bsw/drivers/device/driver_port.h>


#define TIMER_1_PWM_PERIOD_VAL 100  //

extern const CO_IF_TIMER_DRV co_timer_handle_1;
extern const CO_IF_TIMER_DRV co_timer_handle_2;
extern const CO_IF_TIMER_DRV co_timer_handle_3;


/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

uint32_t 	drv_time_get_ticks(void);
void		drv_time_delay_ms(uint32_t ms);
uint32_t	drv_get_tim_3_encoder_ticks(void);
uint32_t	drv_get_tim_2_encoder_ticks(void);
void 		drv_set_tim_3_encoder_ticks(uint32_t tim_val);



#if(INC_DRIVER_TIMER == 1)

drv_timer_handle_type * drv_timer_get_handle (void);

/* PWM related timer api */
drv_status_type	 drv_timer_pwm_output(uint8_t timer_id, float duty_cycle, uint32_t driver_channel, uint8_t n_ch_ctrl_en);

#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_ */
