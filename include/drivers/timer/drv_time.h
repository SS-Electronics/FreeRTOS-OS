/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS OS project
#
# RTOS OS project is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS OS project is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#ifndef OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_
#define OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_

#include <def_std.h>
#include <def_compiler.h>
#include <def_env_macros.h>
#include <def_hw.h>
#include <conf_os.h>
#include <lib/ringbuffer.h>
#include <ipc/mqueue.h>



/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif





uint32_t 	drv_time_get_ticks(void);
void		drv_time_delay_ms(uint32_t ms);

#if (NO_OF_TIMER > 0)
uint32_t	drv_get_tim_3_encoder_ticks(void);
uint32_t	drv_get_tim_2_encoder_ticks(void);
void 		drv_set_tim_3_encoder_ticks(uint32_t tim_val);
#endif






#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* OS_DRIVERS_DEVICE_TIME_DRV_TIME_H_ */
