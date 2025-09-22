/*
File:        kernel_mem.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       memory management
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

#include <os/kernel_mem.h>




/* *****************************************************
 *
 * Kernel malloc api wrapper from FreeRTOS
 *
 * *****************************************************/
 __SECTION_OS void *  kmaloc(size_t size)
 {
    void *ret;

	ret = pvPortMalloc(size);

	return ret;
}

/* *****************************************************
 *
 * Kernel free api wrapper from FreeRTOS
 *
 * *****************************************************/
__SECTION_OS int32_t kfree(void *p)
{
    if(p != NULL)
    {
        vPortFree(p);
        return OS_ERR_NONE;
    }
    else
    {
        return OS_ERR_MEM_OP;
    }
}

/* *****************************************************
 *
 * Get the free memory in heap
 *
 * *****************************************************/
__SECTION_OS uint32_t    os_get_free_mem(void)
{
    uint32_t free_mem;

    free_mem = xPortGetFreeHeapSize();

    return free_mem;
}