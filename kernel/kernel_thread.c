/**
 * @file   	kernel_thread.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   All kernel thread operation api definition
 * @details Thread create delete resume pause delay
 * @date    2025-09-26
 *
 * @note This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.
 */

#include <os/kernel.h>


/**
 * @section Static variables declaration
 * @brief   This section contains all the static varibles
 * 			used in this file
 */

 /**
  * @brief Sentinel list node for the doubly-circular thread list.
  * @note  Follows the Linux kernel intrusive list pattern:
  *        thread_handle_t embeds a struct list_node so no separate
  *        pointer bookkeeping is needed.  An empty list has
  *        thread_list.next == thread_list.prev == &thread_list.
  */
__SECTION_OS_DATA	__KEEP
	LIST_NODE_HEAD(thread_list);

 /**
  * @brief Monotonically increasing counter used as thread IDs.
  * @note  Starts at 0; first created thread receives ID 1.
  */
__SECTION_OS_DATA __KEEP
	static int32_t	thread_counter = 0;


/**
 * @brief A generic thread create api.
 * @note  Creates a dynamic FreeRTOS task and inserts the new
 *        thread_handle_t into the intrusive doubly-circular thread_list
 *        using list_add_tail(), mirroring the Linux kernel task list.
 * @param thread_func        Thread entry function pointer.
 * @param thread_name        Name string for debugging.
 * @param thread_stack_depth Stack size in words.
 * @param priority           Scheduling priority (0 = idle).
 * @param thread_parameter   Opaque pointer forwarded to the thread.
 * @return Positive thread_id on success, negative OS_ERR_* on failure.
 */
int32_t	__SECTION_OS os_thread_create(thread_func_t thread_func,
		                    const char * const thread_name,
							uint32_t thread_stack_depth,
							uint32_t priority,
							void * thread_parameter)
{
	if( (thread_func != NULL) && (thread_stack_depth != 0) )
	{
		thread_handle_t* handle = (thread_handle_t *) kmaloc(sizeof(thread_handle_t));

		if(handle != NULL)
		{
			handle->init_parameter	= thread_parameter;
			handle->priority		= priority;

			BaseType_t returned = xTaskCreate(thread_func,
											thread_name,
											thread_stack_depth,
											thread_parameter,
											(UBaseType_t)priority,
											&handle->thread_handle);

			if(returned == pdTRUE)
			{
				thread_counter++;
				handle->thread_id = thread_counter;
			}
			else
			{
				kfree(handle);
				return OS_ERR_OP;
			}

			/* Append to the tail of the thread list (Linux task_list style) */
			list_add_tail(&handle->list, &thread_list);

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


/**
 * @brief Delete a thread by ID and free its resources.
 * @note  Walks the intrusive list with list_for_each_entry(),
 *        calls vTaskDelete(), unlinks via list_delete(), then frees.
 * @param thread_id ID returned by os_thread_create().
 * @return OS_ERR_NONE on success, OS_ERR_OP if the ID is not found.
 */
int32_t	__SECTION_OS os_thread_delete(uint32_t thread_id)
{
	if(thread_id == 0)
		return OS_ERR_OP;

	thread_handle_t *pos;

	list_for_each_entry(pos, &thread_list, list)
	{
		if(pos->thread_id == thread_id)
		{
			vTaskDelete(pos->thread_handle);
			list_delete(&pos->list);
			kfree(pos);
			return OS_ERR_NONE;
		}
	}

	return OS_ERR_OP;
}


/**
 * @brief Block the calling thread for @p ms milliseconds.
 */
void __SECTION_OS os_thread_delay(uint32_t ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}


/**
 * @brief Suspend the calling thread indefinitely.
 * @note  Equivalent to vTaskSuspend(NULL) — the thread must be
 *        resumed externally via os_resume_thread().
 */
void __SECTION_OS os_suspend_this_thread(void)
{
	vTaskSuspend(NULL);
}


/**
 * @brief Suspend an arbitrary thread by its ID.
 * @note  Walks the intrusive thread list to locate the handle then
 *        calls vTaskSuspend(), mirroring how the Linux kernel
 *        iterates task_struct via the tasks list.
 * @param thread_id ID returned by os_thread_create().
 */
void __SECTION_OS os_suspend_thread(uint32_t thread_id)
{
	if(thread_id == 0)
		return;

	thread_handle_t *pos;

	list_for_each_entry(pos, &thread_list, list)
	{
		if(pos->thread_id == thread_id)
		{
			vTaskSuspend(pos->thread_handle);
			return;
		}
	}
}


/**
 * @brief Resume a previously suspended thread by its ID.
 * @note  Walks the intrusive thread list and calls vTaskResume().
 * @param thread_id ID returned by os_thread_create().
 */
void __SECTION_OS os_resume_thread(uint32_t thread_id)
{
	if(thread_id == 0)
		return;

	thread_handle_t *pos;

	list_for_each_entry(pos, &thread_list, list)
	{
		if(pos->thread_id == thread_id)
		{
			vTaskResume(pos->thread_handle);
			return;
		}
	}
}
