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


#include <os/kernel_syscall.h>


static char temp_time_buffer[10]; // Max value of uint32_t
static char temp_char_buffer[CONF_MAX_CHAR_IN_PRINTK]; // Max value of uint32_t
static struct ringbuffer* printk_ringbuffer_handle;



/* *****************************************************
 *
 *
 *
 * *****************************************************/
void printk_init(void)
{
#ifdef COMM_PRINTK_HW_ID

	if(COMM_PRINTK_HW_ID < NO_OF_UART)
	{
		printk_ringbuffer_handle = (struct ringbuffer*)ipc_mqueue_get_handle(global_uart_tx_mqueue_list[COMM_PRINTK_HW_ID]);
	}

#else
#error 'printk support serial device not defined define COMM_PRINTK_HW_ID <hw id > !'
#endif
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
int32_t	printk(char* ch)
{
#if (NO_OF_UART > 0)
	int 	len = 0, DataIdx;
	char 	temp_byte_buff;

	ATOMIC_ENTER_CRITICAL();

	memset(temp_time_buffer, 0, 10);
	memset(temp_char_buffer, 0, CONF_MAX_CHAR_IN_PRINTK);

	/*
	 * Print the time-stamp
	 * */

	sprintf(temp_time_buffer,"[%d] ",(int)drv_time_get_ticks());
	len = strlen((char*)temp_time_buffer);

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		temp_byte_buff = temp_time_buffer[DataIdx];

		if(printk_ringbuffer_handle != NULL)
		{
			ringbuffer_putchar(printk_ringbuffer_handle, temp_byte_buff);
		}
	}

	/*
	 * Print actual message
	 * */
	len = strlen(ch);

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		temp_byte_buff = ch[DataIdx];

		if(printk_ringbuffer_handle != NULL)
		{
			ringbuffer_putchar(printk_ringbuffer_handle, temp_byte_buff);
		}
	}

	ATOMIC_EXIT_CRITICAL();
	return len;
#else
	(void)ch;
	return 0;
#endif
}


/* __io_putchar / __io_getchar — weak hooks that kernel/syscalls.c calls.
 * Override in board HAL to redirect stdio to a UART. */
int __io_putchar(int ch)
{
	(void)ch;
	return 0;
}

int __io_getchar(void)
{
	return 0;
}




