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

#include <os/kernel.h>


/* *****************************************************
 *
 *
 *
 * *****************************************************/
static type_thread_handles 	app_thread_handles[CONF_NO_OF_MAX_APP_THREAD];
static int32_t				registered_thread_index = 0;
static int32_t				thread_counter			= 1;



/* *****************************************************
 *
 *
 *
 * *****************************************************/
int32_t		os_create_thread(type_os_thread thread_func,
		                      const char * const thread_name,
							  const uint32_t thread_stack_depth,
							  void * const thread_parameters)
{
	if( (thread_func != NULL) && \
		(registered_thread_index < CONF_NO_OF_MAX_APP_THREAD) && \
		(thread_stack_depth != 0)
	  )
	{
		xTaskCreate(thread_func, thread_name, thread_stack_depth, thread_parameters, 1, &app_thread_handles[registered_thread_index].thread_handle);

		int32_t current_thread_id = thread_counter++;
		registered_thread_index++;

		app_thread_handles[registered_thread_index].thread_id				= current_thread_id;
		app_thread_handles[registered_thread_index].thread_priority 		= 1;
		app_thread_handles[registered_thread_index].thread_stack_depth		= thread_stack_depth;

		return current_thread_id;
	}
	else
	{
		return 0;
	}
}












