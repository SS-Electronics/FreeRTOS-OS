/*
File:        kernel.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       kernel
Info:         Main thread info related definitions and APIs              
Dependency:   FreeRTOS-Kernel

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

#ifndef OS_KERNEL_KERNEL_H_
#define OS_KERNEL_KERNEL_H_

#include <def_attributes.h>
#include <def_std.h>
#include <def_err.h>
#include <FreeRTOS.h>



#include "mm/list.h"

#include "kernel_mem.h"
// #include "kernel_syscall.h"
// #include "kernel_services.h"





/**
 * @section Data type definitions
 * @brief   This section contains all data structure 
 *          definitions used in link list.
 */

/**
 * @typedef thread_func_t
 * @brief Function pointer type for generic threads.
 * @param parm a container pointer that will be passed 
 * 			   to the thread when schedular starts it
 *
 * This type is used to define the body of a thread.
 * The function must match the signature: void function(void* parm)
 * @warning The function should never return
 */
typedef void (*thread_func_t)(void* parm);


/**
 * @struct thread_handle_t
 * @brief  Holds handles, id, priority and parameter data of a thread.
 * @note   Intrusive list node (list) follows the Linux kernel pattern:
 *         embed struct list_node inside the object so the generic list
 *         primitives work without extra allocation.
 */
typedef struct
{
	struct list_node	list;			/**> Intrusive doubly-circular list node */
	uint32_t			thread_id;		/**> Unique thread ID assigned at creation */
	uint32_t			priority;		/**> Scheduling priority (higher = more urgent) */
	TaskHandle_t  		thread_handle;	/**> Underlying FreeRTOS task handle */
	void*				init_parameter;	/**> Opaque parameter passed to the thread entry */
}thread_handle_t;











/**
 * @defgroup Thread apis
 * @brief Functions to operate on threads.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Register a new thread; it enters the READY state immediately.
 * @param priority  Scheduling priority (0 = idle, higher = more urgent).
 * @return          Positive thread_id on success, negative OS_ERR_* on failure.
 */
int32_t		 os_thread_create(thread_func_t thread_func,
		                    const char * const thread_name,
							uint32_t thread_stack_depth,
							uint32_t priority,
							void * const thread_parameter);

/** Destroy a thread and release its resources. */
int32_t		 os_thread_delete(uint32_t thread_id);

/** Block the calling thread for @p ms milliseconds. */
void		 os_thread_delay(uint32_t ms);

/** Suspend the calling thread indefinitely. */
void		 os_suspend_this_thread(void);

/** Suspend an arbitrary thread by its ID. */
void		 os_suspend_thread(uint32_t thread_id);

/** Resume a previously suspended thread by its ID. */
void		 os_resume_thread(uint32_t thread_id);



#ifdef __cplusplus
}
#endif
/** @} */ // end of list_api




#endif /* OS_KERNEL_KERNEL_H_ */
