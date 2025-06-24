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
static char data[5] = "abcde";
static struct ringbuffer*		uart_device_tx_char_buffer_handles[NO_OF_UART];
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
		printk("[ ERR ] UART Management suspended!\n\r");
		vTaskSuspend(NULL);
	}
	{
		printk("[ OK ] UART Management started!\n\r");
	}

	while(1)
	{
		/* Based on the TX queues transmit info to the drivers */
		for(int i = 0; i < NO_OF_UART; i++)
		{
			if( ringbuffer_getchar(uart_device_tx_char_buffer_handles[i], (uint8_t*)&temp_char) == FLAG_SET )
			{
				drv_serial_transmit(i, (uint8_t*)&temp_char, 1);
			}
		}

	}
}

status_type thread_uart_mgmt_init(void)
{
	status_type status = ERROR_NONE;

	/* Link the mqueue to corresponding hardware callbacks  */
#if (NO_OF_UART > 0)
	global_uart_rx_mqueue_list[HW_ID_UART_1] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_1, 1, CONF_IPC_UART_1_RX_SIZE);

	if(global_uart_rx_mqueue_list[HW_ID_UART_1] != 0 )
	{
		status |= ERROR_OP;
	}

	global_uart_tx_mqueue_list[HW_ID_UART_1] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_1, 1, CONF_IPC_UART_1_TX_SIZE);

	if(global_uart_tx_mqueue_list[HW_ID_UART_1] != 0 )
	{
		status |= ERROR_OP;
	}

	uart_device_tx_char_buffer_handles[HW_ID_UART_1] = (struct ringbuffer*)ipc_mqueue_get_handle(global_uart_tx_mqueue_list[HW_ID_UART_1]);
#endif

#if (NO_OF_UART > 1)
	global_uart_rx_mqueue_list[HW_ID_UART_2] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_2, 1, CONF_IPC_UART_2_RX_SIZE);

	if(global_uart_rx_mqueue_list[HW_ID_UART_2] != 0 )
	{
		status |= ERROR_OP;
	}

	global_uart_tx_mqueue_list[HW_ID_UART_2] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_2, 1, CONF_IPC_UART_2_TX_SIZE);

	if(global_uart_tx_mqueue_list[HW_ID_UART_2] != 0 )
	{
		status |= ERROR_OP;
	}

	uart_device_tx_char_buffer_handles[HW_ID_UART_2] = (struct ringbuffer*)ipc_mqueue_get_handle(global_uart_tx_mqueue_list[HW_ID_UART_2]);
#endif

#if (NO_OF_UART > 2)
	global_uart_rx_mqueue_list[HW_ID_UART_3] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_3, 1, CONF_IPC_UART_3_RX_SIZE);

	if(global_uart_rx_mqueue_list[HW_ID_UART_3] != 0 )
	{
		status |= ERROR_OP;
	}


	global_uart_tx_mqueue_list[HW_ID_UART_3] =  ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, HW_ID_UART_3, 1, CONF_IPC_UART_3_TX_SIZE);

	if(global_uart_tx_mqueue_list[HW_ID_UART_3] != 0 )
	{
		status |= ERROR_OP;
	}

	uart_device_tx_char_buffer_handles[HW_ID_UART_3] = (struct ringbuffer*)ipc_mqueue_get_handle(global_uart_tx_mqueue_list[HW_ID_UART_3]);
#endif

	/* Init printk to serial driver */
	printk_init();

	/* Initialize all the UART drivers */
	for(int i = 0; i < NO_OF_UART; i++)
	{
		status |= drv_serial_init( i );
	}


	return status;
}

