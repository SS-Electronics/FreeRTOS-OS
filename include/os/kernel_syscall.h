/*
File:        stm32f4xx_it.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       kernel
Info:         all interrrupt handlers descriptions              
Dependency:   CMSIS + HAL Driver repos + Device specific files
              Dvice varinat need to be defined in compiele time

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
#ifndef FREERTOS_OS_INCLUDE_OS_KERNEL_SYSCALL_H_
#define FREERTOS_OS_INCLUDE_OS_KERNEL_SYSCALL_H_


#include <def_std.h>
#include <conf_os.h>
#include <ipc/ringbuffer.h>
#include <drivers/timer/drv_time.h>
#include <ipc/global_var.h>
#include <FreeRTOS.h>
#include <atomic.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void    printk_init(void);
int32_t printk(const char *fmt, ...);

void    printk_enable(void);
void    printk_disable(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */








#endif /* FREERTOS_OS_INCLUDE_OS_KERNEL_SYSCALL_H_ */
