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

static type_message_queue_descriptor   			*mqueue_list_head;
static int32_t									mqueue_id_couter = 1;
static BaseType_t 								temp_flag;

/* *****************************************************
 *
 *
 *
 * *****************************************************/
type_message_queue_descriptor* ipc_get_mqueue_head(void)
{
	return mqueue_list_head;
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type 	ipc_mqueue_init(void)
{
	type_kernel_status status = ERROR_NONE;

	/* Make all the pointer to NULL */
	mqueue_list_head 	= NULL;
	mqueue_id_couter	= 1;

	return status;
}


/* *****************************************************
 *
 *
 *
 * *****************************************************/
int32_t	ipc_mqueue_register(type_mqueue queue_type, int32_t hardware_id, int32_t item_size, int32_t queue_size)
{
	int32_t mqueue_id = 0;

	ATOMIC_ENTER_CRITICAL();

	type_message_queue_descriptor* new_node = ( type_message_queue_descriptor* )kmaloc( sizeof(type_message_queue_descriptor) );

	/*
	 * update mqueue descriptor field and handles
	 * */
	if(new_node != NULL)
	{
		/* initialize to NULL */
		new_node->next_node = NULL;
		new_node->prev_node = NULL;

		switch(queue_type)
		{
			/*
			 * Queues between two thread data txn
			 *  */
			case IPC_MQUEUE_TYPE_PROC_TXN:
				/* create the queue ptr and assign the handle value */
				QueueHandle_t temp_queue_handle = xQueueCreate( queue_size, item_size );

				if(temp_queue_handle != NULL)
				{
					new_node->mqueue.queue_id 					= mqueue_id_couter++;
					mqueue_id									= new_node->mqueue.queue_id;
					new_node->mqueue.queue_type 				= IPC_MQUEUE_TYPE_PROC_TXN;
					new_node->mqueue.linked_hw_peripheral_id	= -1;
					new_node->mqueue.free_rtos_queue_handle		= temp_queue_handle;
				}
			break;

			/*
			 * Byte stream butter from hardware
			 * */
			case IPC_MQUEUE_TYPE_UART_HW:
				/* create a mqueue descriptor node in the list */
				/* for UART its character ringbuffer */
				struct ringbuffer* temp_buffer_handle = ( struct ringbuffer* )kmaloc( sizeof(struct ringbuffer) );

				if(temp_buffer_handle != NULL)
				{
					/* create the ring buffer storage */
					uint8_t* rb_storage_ptr =  ( uint8_t* )kmaloc( queue_size );

					if(rb_storage_ptr != NULL)
					{
						ringbuffer_init(temp_buffer_handle, rb_storage_ptr, queue_size);

						/* Update the mqueue descriptor */
						new_node->mqueue.queue_id 					= mqueue_id_couter++;
						mqueue_id									= new_node->mqueue.queue_id;
						new_node->mqueue.queue_type 				= IPC_MQUEUE_TYPE_UART_HW;
						new_node->mqueue.linked_hw_peripheral_id	= hardware_id;
						new_node->mqueue.handle						= (void*)temp_buffer_handle;

					}
					else
					{
						/* Free all the memory */
						kfree(temp_buffer_handle);
					}
				}
			break;



		}

		/* Get the first node descriptor */
		type_message_queue_descriptor* temp = mqueue_list_head;

		// if the first node is null
		if(mqueue_list_head == NULL)
		{
			mqueue_list_head = new_node;
		}
		else
		{
			/* Traverse to the last */
			 while (temp->next_node != NULL)
			 {
					temp = temp->next_node;
			 }
		}

		//append the newly created node to the last item next
		temp->next_node = new_node;
		// current temp node is the previous of new node item
		new_node->prev_node = temp;
	}

	ATOMIC_EXIT_CRITICAL();

	return mqueue_id;
}


/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type	ipc_mqueue_unregister(int32_t mqueue_id)
{
	status_type status = ERROR_NONE;

	ATOMIC_ENTER_CRITICAL();

	if(mqueue_id < mqueue_id_couter)
	{
		/* Get the first node descriptor */
		type_message_queue_descriptor* temp = mqueue_list_head;

		/* Find the value until reach last node*/
	    while( (temp != NULL) && (temp->mqueue.queue_id != mqueue_id) )
	    {
	        temp = temp->next_node;
	    }

	    /* if not null, Current temp holds the node which have id = mqueue_id*/
	    if(temp != NULL)
	    {
	    	/* Free the associate handle and related buffers */
	    	switch(temp->mqueue.queue_type)
			{
				case IPC_MQUEUE_TYPE_PROC_TXN:

				break;

				case IPC_MQUEUE_TYPE_UART_HW:

					struct ringbuffer* ptr = (struct ringbuffer*)temp->mqueue.handle;

					/* Buffer memory pool free*/
					kfree(ptr->buffer_ptr);
					/* handle memory free */
					kfree(ptr);
				break;



			}

	    	/* Link the next nodes  */
	        if (temp->prev_node != NULL)
	        {
	        	/* Update the previous nodes next as current nodes next */
	        	 temp->prev_node->next_node = temp->next_node;
	        }
	        else // there is no previous node means this is first node so update the head
	        {
	        	mqueue_list_head = temp->next_node;
	        }

	        /* Link the previous nodes  */

	        if (temp->next_node != NULL)
	            temp->next_node->prev_node = temp->prev_node;

	    }
	    else
	    {
	    	status |= ERROR_OP;
	    }
	}
	else
	{
		status |= ERROR_OP;
	}

	ATOMIC_EXIT_CRITICAL();

	return status;
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void*	ipc_mqueue_get_handle(int32_t mqueue_id)
{

	void * mqueu_ptr = NULL;

	ATOMIC_ENTER_CRITICAL();

	if(mqueue_id < mqueue_id_couter)
	{
		/* Get the first node descriptor */
		type_message_queue_descriptor* temp = mqueue_list_head;

		/* Find the value until reach last node*/
		while( (temp != NULL) && (temp->mqueue.queue_id != mqueue_id) )
		{
			temp = temp->next_node;
		}

		/* if not null, Current temp holds the node which have id = mqueue_id*/
		if(temp != NULL)
		{
			mqueu_ptr = temp->mqueue.handle;
		}
	}

	ATOMIC_EXIT_CRITICAL();

	return mqueu_ptr;
}


/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type	ipc_mqueue_send_item(uint32_t queue_id, const void * item_ptr)
{
	status_type status = ERROR_NONE;

	ATOMIC_ENTER_CRITICAL();

	/* check for valid queue id */
	if(queue_id < mqueue_id_couter)
	{
		/* Get the first node descriptor */
		type_message_queue_descriptor* temp = mqueue_list_head;

		/* Find the value until reach last node*/
		while( (temp != NULL) && (temp->mqueue.queue_id != queue_id) )
		{
			temp = temp->next_node;
		}

		/* if not null, Current temp holds the node which have id = mqueue_id*/
		if( (temp != NULL) && (temp->mqueue.queue_type == IPC_MQUEUE_TYPE_PROC_TXN) )
		{
			/* send the item from queue */
			BaseType_t queue_send_status;

			queue_send_status =  xQueueSendFromISR ( temp->mqueue.free_rtos_queue_handle,
													 item_ptr,
													 &temp_flag );

			if(queue_send_status |= pdTRUE)
			{
				status |= ERROR_OP;
			}
		}
		else
		{
			status |= ERROR_OP;
		}
	}
	else
	{
		status |= ERROR_OP;
	}

	ATOMIC_EXIT_CRITICAL();

	return status;
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type	ipc_mqueue_receive_item(uint32_t queue_id, void * item_ptr)
{
	status_type status = ERROR_NONE;

	ATOMIC_ENTER_CRITICAL();

	/* check for valid queue id */
	if(queue_id < mqueue_id_couter)
	{
		/* Get the first node descriptor */
		type_message_queue_descriptor* temp = mqueue_list_head;

		/* Find the value until reach last node*/
		while( (temp != NULL) && (temp->mqueue.queue_id != queue_id) )
		{
			temp = temp->next_node;
		}

		/* if not null, Current temp holds the node which have id = mqueue_id*/
		if( (temp != NULL) && (temp->mqueue.queue_type == IPC_MQUEUE_TYPE_PROC_TXN) )
		{
			/* send the item from queue */
			BaseType_t queue_send_status;

			queue_send_status =  xQueueReceiveFromISR ( temp->mqueue.free_rtos_queue_handle,
													 	item_ptr,
														&temp_flag );

			if(queue_send_status |= pdTRUE)
			{
				status |= ERROR_OP;
			}
		}
		else
		{
			status |= ERROR_OP;
		}
	}
	else
	{
		status |= ERROR_OP;
	}

	ATOMIC_EXIT_CRITICAL();

	return status;
}















