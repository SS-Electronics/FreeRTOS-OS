/*
 * File:        device_irq.h
 * Author:      Subhajit Roy  
 *              subhajitroy005@gmail.com 
 *
 * Moudle:      Modeule Device [ Local Build ] 
 * Info:        Device specific irq handlers 
 *                            
 * Dependency:  None
 *
 * This file is part of FreeRTOS-KERNEL Project.
 *
 * FreeRTOS-KERNEL is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-KERNEL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 *along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __DEVICE_IRQ_H__
#define __DEVICE_IRQ_H__

// #include "../include/std/std_types.h"


// /**
//  * Function pointer typedefs
//  */
// typedef	void (*irq_handler_t)(void* des);



// enum __irq_descriptions
// {
//     IRQ_CPU_KERN_TICK = 0,
//     IRQ_CPU_SW_INT_0,
//     IRQ_CPU_PENDSV_0
// };

// /**
//  * Struct Desc: Hold all the callback functions registered in kernel
//  * @irq_id:         Unique Identifier of the IRQ
//  * @irq_cb:         irq_callback function set
//  * @irq_status:     Status after callback operation
//  */
// typedef struct 
// {
//     irq_handler_t   irq_handler;
//     int             hw_irq_counter;
// }type_device_cpu_irq_handle;












#ifdef __cplusplus
extern "C" {
#endif


// type_device_cpu_irq_handle * dev_get_irq_cpu_handle(uint32_t irq_idx);     






#ifdef __cplusplus
}
#endif













#endif