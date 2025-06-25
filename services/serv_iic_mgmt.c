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

#include <os/kernel_services.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/



/* *****************************************************
 *
 *
 *
 * *****************************************************/

static status_type thread_iic_mgmt_init(void);


/* *****************************************************
 *
 *
 *
 * *****************************************************/
void therad_iic_mgmt(void * arg)
{
	/* Startup offset delay */
	vTaskDelay(pdMS_TO_TICKS(CONF_THREAD_IIC_MGMT_STARTUP_OFFSET_MS));

	/* Thread init */
	if(thread_iic_mgmt_init() != ERROR_NONE)
	{
		printk("[ ERR ] IIC Management suspended!\n\r");
		vTaskSuspend(NULL);
	}
	else
	{
		printk("[ OK ] IIC Management started!\n\r");
	}

	/* Thread loop */
	while(1)
	{
		/* Printk related prints */

//		printk("[ OK ] IIC Management started!\n\r");
//
//		vTaskDelay(pdMS_TO_TICKS(1000));


	}
}


/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type thread_iic_mgmt_init(void)
{
	status_type status = ERROR_NONE;

	/* Initialize all the UART drivers */
//	for(int i = 0; i < NO_OF_IIC; i++)
//	{
//		status |= drv_iic_init( i );
//	}




	return status;
}

