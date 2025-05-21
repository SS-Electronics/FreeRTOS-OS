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


#ifndef FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_
#define FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_

#include <def_std.h>

#include <FreeRTOS.h>
#include <atomic.h>
#include <queue.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include <event_groups.h>
#include <stream_buffer.h>




#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void*  		kmaloc(size_t size);
int32_t 	kfree(void *p);













#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_ */
