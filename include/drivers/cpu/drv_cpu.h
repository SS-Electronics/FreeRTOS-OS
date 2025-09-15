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

#ifndef OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_
#define OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_

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

void 					drv_cpu_interrupt_prio_set(void);
void 					reset_mcu(void);

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/

#endif /* OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_ */
