/*
File:        def_std.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       include
Info:         standard definition to the project              
Dependency:   none

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

#ifndef FREERTOS_OS_INCLUDE_DEF_STD_H_
#define FREERTOS_OS_INCLUDE_DEF_STD_H_

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>



typedef int32_t status_t;    


/**************************************
 * 
 *  Data structure definitions
 * 
 *************************************/

/* Maintain each node */
struct list_node 
{
    struct list_node *next, *prev;
};

/* Maintain the head */
struct list_head 
{
    struct list_head * head;
};


/**************************************
 * 
 *  Error definitions
 * 
 *************************************/

#define OS_ERR_NONE         0
#define OS_ERR_OP           -1
#define OS_ERR_MEM_OF       -2
#define OS_ERR_MEM_OP       -3



/**************************************
 * 
 *  Interrupt definitions
 * 
 *************************************/
typedef int32_t irq_hw_id_t;





#endif /* FREERTOS_OS_INCLUDE_DEF_STD_H_ */
