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

#include <drivers/drv_uart.h>

#if (NO_OF_UART > 0)

/* Local driver handle reference*/
static type_drv_hw_handle 		*uart_handle_ref;

/* Operational Variables*/
static uint8_t 					temp_rx_char_buff;

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void	drv_serial_update_handle(__TYPE_HW_UART_HANDLE_TYPE * handle, uint8_t dev_id)
{
	if( (handle != NULL) && (dev_id < NO_OF_UART) )
	{
		uart_handle_ref->handle_hw_uart[dev_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type	drv_serial_init(uint8_t dev_id)
{
	status_type drv_staus = ERROR_NONE;

	/* Check if the handles are NULL or not */
	if((uart_handle_ref->handle_hw_uart[dev_id] != NULL) && (dev_id < NO_OF_UART) )
	{
		/*Start the communication */
		drv_staus |= UART_Start_Receive_IT(uart_handle_ref->handle_hw_uart[dev_id], &temp_rx_char_buff, 1);
	}
	else
	{
		drv_staus |= ERROR_OP;
	}

	return drv_staus;
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type	drv_serial_transmit(uint8_t dev_id, uint8_t* data, uint16_t len)
{
	status_type status = ERROR_NONE;

	/* Send one by one character if the handle is not null */
	if((uart_handle_ref->handle_hw_uart[dev_id] != NULL) && (dev_id < NO_OF_UART) )
	{
		/* Timeout 10mS*/
		for (int i = 0; i<len; i++)
		{
			status |= HAL_UART_Transmit(uart_handle_ref->handle_hw_uart[dev_id]
										,&data[i]
										,1
										,CONF_UART_TXN_TIMEOUT_MS
										);
		}
	}
	else
	{
		status |= ERROR_OP;

	}

	return status;
}







/* *****************************************************
 *
 *	Call Backs DEVICE_VARINAT 1
 *
 * *****************************************************/

#if (DEVICE_VARINAT == 1)

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Not planned for the implementation */
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Read the character*/
	HAL_UART_Receive_IT(huart, &temp_rx_char_buff, 1);

	/* Put the character in queue [ Multiplexed here according the handle type */
	/* NO context switch will happen here so no thread safe needed */

	if(huart == uart_handle_ref->handle_hw_uart[HW_ID_UART_1])
	{
#if(CONF_INC_PROC_OS_SHELL_MGMT == 1)
		ringbuffer_putchar(&ipc_handle_uart_1_drv_rx_handle, temp_rx_char_buff);
#endif
	}
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

}

#endif // (DEVICE_VARINAT == 1)



/* *****************************************************
 *
 *	Call Backs DEVICE_VARINAT 1
 *
 * *****************************************************/



#endif
