/*
File:        kernel_thread.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       kernel
Info:         thread related function definitions           
Dependency:   

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>. */

#include <os/kernel.h>


/* *****************************************************
 *
 * Priate variables
 *
 * *****************************************************/

__SECTION_OS_DATA	__KEEP 
	static	struct list_node thread_hanldes;

__SECTION_OS_DATA __KEEP
	static int32_t	thread_counter = 0;




/* *****************************************************
 *
 * OS Thread Create
 *
 * *****************************************************/
int32_t	__SECTION_OS os_thread_create(thread_func_t thread_func,
		                    const char * const thread_name,
							uint32_t thread_stack_depth,
							void * thread_parameter)
{
	if( (thread_func != NULL) && \
		(thread_stack_depth != 0)
	  )
	{
		/* createh the thread handle */
		thread_handle_t* handle = (thread_handle_t *) kmaloc(sizeof(thread_handle_t));
		
		if(handle != NULL)
		{
			handle->init_parameter = thread_parameter;
			handle->next_handle	= NULL;
			
			BaseType_t returned = xTaskCreate(thread_func, 
											thread_name, 
											thread_stack_depth, 
											thread_parameter, 
											1, 
											&handle->thread_handle);
			
			if(returned == pdTRUE) /* Task created successfully */
			{
				thread_counter++; // increment the task_id
				handle->thread_id	= thread_counter;
			}
			else
			{
				/* Free the pointer */
				kfree(handle);

				return OS_ERR_OP;
			}

			/* Push into the thread list */
			if(thread_list.thred_list_head == NULL) // If the head is empty assign to head
			{
				thread_list.thred_list_head = handle;
			}
			else // insert in the node
			{
				thread_handle_t * temp = thread_list.thred_list_head;

				while(temp->next_handle != NULL)
				{
					temp = temp->next_handle;
				}

				/* Once last node found insert into the last node */
				temp->next_handle = handle;
			}

			/* Thread created successfully and return the thread ID */
			return handle->thread_id;

		}
		else
		{
			return OS_ERR_MEM_OP;
		}
	}
	else
	{
		return OS_ERR_OP;
	}
}


/* *****************************************************
 *
 * OS Thread delete
 *
 * *****************************************************/
int32_t		__SECTION_OS  os_thread_delete(uint32_t thread_id)
{
	if(thread_id != 0)
	{
		thread_handle_t* temp = thread_list.thred_list_head;
		thread_handle_t* previous_node;

		/* If the thread_id is in head */
		if( (temp != NULL) && (temp->thread_id == thread_id) )
		{
			/*  delete the task with the handle */
			vTaskDelete(temp->thread_handle);
			
			/* Change the head to the next node */
			thread_list.thred_list_head = temp->next_handle;

			/* free  the memory */
			kfree(temp);

			return OS_ERR_NONE;
		}
		
		/* Find the thread ID in the list  */
		while( (temp != NULL) && (temp->thread_id != thread_id) )
		{
			previous_node = temp;

			temp = temp->next_handle;
		}

		if(temp == NULL) // No thread ID found so return with err
		{
			return OS_ERR_OP;
		}
		else
		{
			/*  delete the task with the handle */
			vTaskDelete(temp->thread_handle);

			/* link prev node->next to curr node -> next */
			previous_node->next_handle = temp->next_handle;

			/* free  the memory */
			kfree(temp);

			return OS_ERR_NONE;
		}
	}
	else
	{
		return OS_ERR_OP;
	}
}

/* *****************************************************
 *
 * OS delay by calling thread
 *
 * *****************************************************/
void __SECTION_OS os_thread_delay(uint32_t ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}

/* *****************************************************
 *
 * OS Suspend by calling thread
 *
 * *****************************************************/
void __SECTION_OS os_suspend_this_thread(void)
{
	vTaskSuspend(NULL);
}

/* *****************************************************
 *
 * OS suspend the thread by thread ID
 *
 * *****************************************************/
void	__SECTION_OS	 os_suspend_thread(uint32_t thread_id)
{

}

/* *****************************************************
 *
 * OS reusme thread by the thread ID
 *
 * *****************************************************/
void	__SECTION_OS	 os_resume_thread(int32_t thread_id)
{

}








