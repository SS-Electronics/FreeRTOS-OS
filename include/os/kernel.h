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

#ifndef OS_KERNEL_KERNEL_H_
#define OS_KERNEL_KERNEL_H_


#include <FreeRTOS.h>
#include <atomic.h>
#include <queue.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include <event_groups.h>
#include <stream_buffer.h>


#include "kernel_mem.h"
#include "kernel_syscall.h"
#include "kernel_services.h"




typedef struct
{
	uint32_t			thread_id;
	TaskHandle_t  		thread_handle;
	uint32_t			thread_stack_depth;
	void*				init_parameter;
	int32_t				thread_priority;
}type_thread_handles;









/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif




int32_t		os_create_thread(type_os_thread thread_func,
		                      const char * const thread_name,
							  const uint32_t thread_stack_depth,
							  void * const thread_parameters);










#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* OS_KERNEL_KERNEL_H_ */
