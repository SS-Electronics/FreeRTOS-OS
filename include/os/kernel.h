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
 * @brief  Holds handles , id and parameter data of a thread
 * @note This is abstruction over TaskHandle_t of FreeRTOS
 */
typedef struct
{
	struct list_node	list;		/**> List block */
	uint32_t			thread_id;	/**> Thread Id: All thread are controlled over this id  */
	TaskHandle_t  		thread_handle; /**> FreeRTOS task Handle */
	void*				init_parameter; /**> Init container pointer */
}thread_handle_t;











/**
 * @defgroup Thread apis
 * @brief Functions to operate on threads.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/*  Register a new thread and after add it 
will be in ready state  */
int32_t		 os_thread_create(thread_func_t thread_func,
		                    const char * const thread_name,
							uint32_t thread_stack_depth,
							void * const thread_parameter);

/* 
 Destry the thread and its associtae memories
*/
int32_t		 os_thread_delete(uint32_t thread_id);

/* 
 Block the thread at the point this func called
*/
void		 os_thread_delay(uint32_t ms);

/* 
 Suspend the current thred
*/
void		 os_suspend_this_thread(void);

/* 
 Suspend the thred by thread id
*/
void		 os_suspend_thread(uint32_t thread_id);

/* 
 resume a thread bt the ID
*/
void		 os_resume_thread(int32_t thread_id);



#ifdef __cplusplus
}
#endif
/** @} */ // end of list_api




#endif /* OS_KERNEL_KERNEL_H_ */
