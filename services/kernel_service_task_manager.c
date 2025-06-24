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


#include <os/kernel_services.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/
static type_message_queue_descriptor 						*mqueue_handles;

static status_type 											status;









/* *****************************************************
 *
 *
 *
 * *****************************************************/
static 	status_type thread_task_mgmt_init(void);











/* *****************************************************
 *
 *
 *
 * *****************************************************/
void thread_task_mgmt(void* arg)
{
	status = ERROR_NONE;
	/* Start after offset */
//	status |= thread_task_mgmt_init() != N


	if(status != ERROR_NONE)
	{

	}


	while(1)
	{












	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type thread_task_mgmt_init(void)
{
	status_type status = ERROR_NONE;


	/* Get the mqueue handles*/
	mqueue_handles = ipc_get_mqueue_head();

	if(mqueue_handles == NULL)
	{
		status |= ERROR_OP;
	}




	/* Create mqueus as per of the services */





}
