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




/* Sentinel node for the doubly-circular message queue list */
static LIST_NODE_HEAD(mqueue_list_head);

static int32_t mqueue_id_counter = 1;


/* *****************************************************
 *
 * Return the list sentinel so callers can iterate with
 * list_for_each_entry(pos, ipc_get_mqueue_head(), list).
 *
 * *****************************************************/
struct list_node* ipc_get_mqueue_head(void)
{
	return &mqueue_list_head;
}


/* *****************************************************
 *
 * Reset the queue list to an empty sentinel state.
 *
 * *****************************************************/
status_type ipc_mqueue_init(void)
{
	list_init(&mqueue_list_head);
	mqueue_id_counter = 1;
	return ERROR_NONE;
}


/* *****************************************************
 *
 * Allocate and register a new message queue.
 * Returns a positive queue_id on success, 0 on failure.
 *
 * *****************************************************/
int32_t ipc_mqueue_register(type_mqueue queue_type, int32_t hardware_id,
                             int32_t item_size, int32_t queue_size)
{
	int32_t mqueue_id = 0;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor *new_node =
		(type_message_queue_descriptor *)kmaloc(sizeof(type_message_queue_descriptor));

	if(new_node != NULL)
	{
		switch(queue_type)
		{
			case IPC_MQUEUE_TYPE_PROC_TXN:
			{
				QueueHandle_t temp_queue_handle = xQueueCreate(queue_size, item_size);

				if(temp_queue_handle != NULL)
				{
					new_node->mqueue.queue_id                   = mqueue_id_counter++;
					mqueue_id                                   = new_node->mqueue.queue_id;
					new_node->mqueue.queue_type                 = IPC_MQUEUE_TYPE_PROC_TXN;
					new_node->mqueue.linked_hw_peripheral_id    = -1;
					new_node->mqueue.free_rtos_queue_handle     = temp_queue_handle;
					new_node->mqueue.handle                     = NULL;
				}
				else
				{
					kfree(new_node);
					new_node = NULL;
				}
				break;
			}

			case IPC_MQUEUE_RING_BUFFER:
			{
				struct ringbuffer *temp_buffer_handle =
					ringbuffer_create((uint16_t)queue_size);

				if(temp_buffer_handle != NULL)
				{
					new_node->mqueue.queue_id                   = mqueue_id_counter++;
					mqueue_id                                   = new_node->mqueue.queue_id;
					new_node->mqueue.queue_type                 = IPC_MQUEUE_RING_BUFFER;
					new_node->mqueue.linked_hw_peripheral_id    = hardware_id;
					new_node->mqueue.handle                     = (void *)temp_buffer_handle;
					new_node->mqueue.free_rtos_queue_handle     = NULL;
				}
				else
				{
					kfree(new_node);
					new_node = NULL;
				}
				break;
			}

			default:
				kfree(new_node);
				new_node = NULL;
				break;
		}

		if(new_node != NULL)
		{
			/* Add at tail — O(1) with circular sentinel */
			list_add_tail(&new_node->list, &mqueue_list_head);
		}
	}

	ATOMIC_EXIT_CRITICAL();

	return mqueue_id;
}


/* *****************************************************
 *
 * Unregister a queue by ID: free its backend resources
 * and remove the descriptor from the list.
 *
 * *****************************************************/
status_type ipc_mqueue_unregister(int32_t mqueue_id)
{
	status_type status = ERROR_NONE;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor *pos;

	list_for_each_entry(pos, &mqueue_list_head, list)
	{
		if(pos->mqueue.queue_id == (uint32_t)mqueue_id)
		{
			switch(pos->mqueue.queue_type)
			{
				case IPC_MQUEUE_TYPE_PROC_TXN:
					vQueueDelete(pos->mqueue.free_rtos_queue_handle);
					break;

				case IPC_MQUEUE_RING_BUFFER:
					ringbuffer_destroy((struct ringbuffer *)pos->mqueue.handle);
					break;

				default:
					break;
			}

			list_delete(&pos->list);
			kfree(pos);

			ATOMIC_EXIT_CRITICAL();
			return ERROR_NONE;
		}
	}

	status = ERROR_OP;

	ATOMIC_EXIT_CRITICAL();

	return status;
}


/* *****************************************************
 *
 * Return the opaque handle associated with a queue ID.
 *
 * *****************************************************/
void* ipc_mqueue_get_handle(int32_t mqueue_id)
{
	void *mqueue_ptr = NULL;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor *pos;

	list_for_each_entry(pos, &mqueue_list_head, list)
	{
		if(pos->mqueue.queue_id == (uint32_t)mqueue_id)
		{
			mqueue_ptr = pos->mqueue.handle;
			break;
		}
	}

	ATOMIC_EXIT_CRITICAL();

	return mqueue_ptr;
}


/* *****************************************************
 *
 * Send an item to a PROC_TXN queue from task context.
 *
 * *****************************************************/
status_type ipc_mqueue_send_item(uint32_t queue_id, const void *item_ptr)
{
	status_type status = ERROR_OP;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor *pos;

	list_for_each_entry(pos, &mqueue_list_head, list)
	{
		if(pos->mqueue.queue_id == queue_id)
		{
			if(pos->mqueue.queue_type == IPC_MQUEUE_TYPE_PROC_TXN)
			{
				ATOMIC_EXIT_CRITICAL();

				/* Use task-context send; 0 timeout (non-blocking) */
				BaseType_t ret = xQueueSend(pos->mqueue.free_rtos_queue_handle,
				                            item_ptr, (TickType_t)0);
				status = (ret == pdTRUE) ? ERROR_NONE : ERROR_OP;
				return status;
			}
			break;
		}
	}

	ATOMIC_EXIT_CRITICAL();

	return status;
}


/* *****************************************************
 *
 * Receive an item from a PROC_TXN queue from task context.
 *
 * *****************************************************/
status_type ipc_mqueue_receive_item(uint32_t queue_id, void *item_ptr)
{
	status_type status = ERROR_OP;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor *pos;

	list_for_each_entry(pos, &mqueue_list_head, list)
	{
		if(pos->mqueue.queue_id == queue_id)
		{
			if(pos->mqueue.queue_type == IPC_MQUEUE_TYPE_PROC_TXN)
			{
				ATOMIC_EXIT_CRITICAL();

				/* Use task-context receive; 0 timeout (non-blocking) */
				BaseType_t ret = xQueueReceive(pos->mqueue.free_rtos_queue_handle,
				                               item_ptr, (TickType_t)0);
				status = (ret == pdTRUE) ? ERROR_NONE : ERROR_OP;
				return status;
			}
			break;
		}
	}

	ATOMIC_EXIT_CRITICAL();

	return status;
}
