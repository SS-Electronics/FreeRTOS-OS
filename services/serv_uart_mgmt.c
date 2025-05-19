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
static char temp_char;

/* *****************************************************
 *
 *
 *
 * *****************************************************/

static status_type thread_uart_mgmt_init(void);


/* *****************************************************
 *
 *
 *
 * *****************************************************/
void thread_uart_mgmt(void * arg)
{
	if(thread_uart_mgmt_init() != ERROR_NONE)
	{
		vTaskSuspend(NULL);
	}

	while(1)
	{
		/* Printk related prints */
#if defined(COMM_PRINTK_HW_ID)
		if( ringbuffer_getchar(&ipc_handle_printk_buffer, (uint8_t*)&temp_char) == FLAG_SET)
		{
			drv_serial_transmit(COMM_PRINTK_HW_ID, (uint8_t*)&temp_char, 1);
		}
#else
#warning "Printk hardware not configured...! Define COMM_PRINTK_HW_ID"
#endif





	}
}

status_type thread_uart_mgmt_init(void)
{
	status_type status = ERROR_NONE;

	/* Initialize all the UART drivers */
	for(int i = 0; i < NO_OF_UART; i++)
	{
		status |= drv_serial_init( i );
	}




	return status;
}

