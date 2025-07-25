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

#ifndef OS_IPC_MQUEUE_MQUEUE_H_
#define OS_IPC_MQUEUE_MQUEUE_H_

#include <def_std.h>
#include <def_compiler.h>
#include <def_env_macros.h>
#include <def_hw.h>
#include <conf_os.h>
#include <ipc/ringbuffer.h>
#include <os/kernel_mem.h>

#include <FreeRTOS.h>
#include <queue.h>



typedef enum
{
	IPC_MQUEUE_TYPE_UART_HW = 0,
	IPC_MQUEUE_TYPE_IIC_RX,
	IPC_MQUEUE_TYPE_IIC_TX,
	IPC_MQUEUE_TYPE_SPI_RX,
	IPC_MQUEUE_TYPE_SPI_TX,
	IPC_MQUEUE_TYPE_CAN_RX,
	IPC_MQUEUE_TYPE_CAN_TX,
	IPC_MQUEUE_TYPE_PROC_TXN
}type_mqueue;






typedef struct
{
	uint32_t			queue_id;
	int32_t				queue_type; 				// hardware / software
	int32_t				linked_hw_peripheral_id;	// IIC
	void *				handle;						// Implemented handle reference ringbuffer / mqueue etc
	QueueHandle_t		free_rtos_queue_handle;		// FreeRtos Queue handle
}type_mqueue_descriptor_content;




typedef struct message_queue_descriptor
{
	struct message_queue_descriptor		*prev_node;
	struct message_queue_descriptor		*next_node;

	type_mqueue_descriptor_content 		mqueue;

}type_message_queue_descriptor;





/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

status_type 		ipc_mqueue_init(void);



int32_t				ipc_mqueue_register(type_mqueue queue_type, int32_t hardware_id, int32_t item_size, int32_t queue_size);
status_type			ipc_mqueue_unregister(int32_t mqueue_id);
void*				ipc_mqueue_get_handle(int32_t mqueue_id);
status_type			ipc_mqueue_send_item(uint32_t queue_id, const void * item_ptr);
status_type			ipc_mqueue_receive_item(uint32_t queue_id, void * item_ptr);




type_message_queue_descriptor* ipc_get_mqueue_head(void);




#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* OS_IPC_MQUEUE_MQUEUE_H_ */
