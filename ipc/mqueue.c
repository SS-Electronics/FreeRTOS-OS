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

#include <ipc/mqueue.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_UART > 0)

struct ringbuffer				ipc_handle_uart_1_drv_rx_handle;
struct ringbuffer				ipc_handle_uart_2_drv_rx_handle;

#ifndef CONF_IPC_RD_UART_1_RX_SIZE
#error "[ OS ] UART 1 CONF_IPC_RD_UART_1_RX_SIZE not defined...!"
#else
static uint8_t					ipc_handle_uart_1_drv_rx_storage[CONF_IPC_RD_UART_1_RX_SIZE];
#endif

struct ringbuffer				ipc_handle_printk_buffer;
static uint8_t					ipc_handle_printk_buffer_storage[CONF_IPC_PRINTK_BUFFER_SIZE];

#endif


#if (NO_OF_UART > 1)
struct ringbuffer				ipc_handle_uart_2_drv_rx_handle;
struct ringbuffer				ipc_handle_uart_2_drv_tx_handle;

#ifndef CONF_IPC_RD_UART_1_RX_SIZE
#error "[ OS ] UART 2 CONF_IPC_RD_UART_2_RX_SIZE not defined...!"
#else
static uint8_t					ipc_handle_uart_2_drv_rx_storage[CONF_IPC_RD_UART_2_RX_SIZE];
static uint8_t					ipc_handle_uart_2_drv_tx_storage[CONF_IPC_RD_UART_2_TX_SIZE];
#endif

#endif




/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type 	ipc_mqueue_init(void)
{
	type_kernel_status status = ERROR_NONE;


#if(NO_OF_UART > 0)
	ringbuffer_init(&ipc_handle_uart_1_drv_rx_handle, ipc_handle_uart_1_drv_rx_storage, CONF_IPC_RD_UART_1_RX_SIZE);
	ringbuffer_init(&ipc_handle_printk_buffer, ipc_handle_printk_buffer_storage, CONF_IPC_PRINTK_BUFFER_SIZE);
#endif

#if(NO_OF_UART > 1)
	ringbuffer_init(&ipc_handle_uart_2_drv_rx_handle, ipc_handle_uart_2_drv_rx_storage, CONF_IPC_RD_UART_2_RX_SIZE);
	ringbuffer_init(&ipc_handle_uart_2_drv_tx_handle, ipc_handle_uart_2_drv_tx_storage, CONF_IPC_RD_UART_2_TX_SIZE);
#endif
	/* USB DRIVER 1 ISR to canopen read funciton linkage */



	return status;
}



