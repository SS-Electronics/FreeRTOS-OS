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
 *	Task handles
 *
 *
 * *****************************************************/
static TaskHandle_t thread_handle_service_uart_mgmt = NULL;
static StaticTask_t thread_buff_service_uart_mgmt;
static StackType_t  thread_stack_service_uart_mgmt[ CONF_THREAD_UART_MGMT_STACK_SIZE ];

static TaskHandle_t thread_handle_service_iic_mgmt = NULL;
static StaticTask_t thread_buff_service_iic_mgmt;
static StackType_t  thread_stack_service_iic_mgmt[ CONF_THREAD_IIC_MGMT_STACK_SIZE ];
/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type os_kernel_thread_register(void)
{
	status_type status = ERROR_NONE;

	/* Register UART management task */
	thread_handle_service_uart_mgmt = xTaskCreateStatic( thread_uart_mgmt,
													    "THREAD_SERIAL_MGMT",
														CONF_THREAD_UART_MGMT_STACK_SIZE,
													    NULL,
														CONF_THREAD_UART_MGMT_PRIORITY,
														thread_stack_service_uart_mgmt,
													    &thread_buff_service_uart_mgmt
		                							   );
	if( thread_handle_service_uart_mgmt == NULL )
	{
		/* Exception mechanism */
		status |= ERROR_OP;
	}

	/* Register IIC management task */
	thread_handle_service_iic_mgmt = xTaskCreateStatic( therad_iic_mgmt,
													    "THREAD_IIC_MGMT",
														CONF_THREAD_IIC_MGMT_STACK_SIZE,
													    NULL,
														CONF_THREAD_IIC_MGMT_PRIORITY,
														thread_stack_service_iic_mgmt,
													    &thread_buff_service_iic_mgmt
		                							   );
	if( thread_handle_service_iic_mgmt == NULL )
	{
		/* Exception mechanism */
		status |= ERROR_OP;
	}



	return status;
}




