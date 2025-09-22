/*
 * File:        irq.h
 * Author:      Subhajit Roy  
 *              subhajitroy005@gmail.com 
 *
 * Moudle:      Modeule Kernel [ Local Build ] 
 * Info:        Interrupt registration and constrols from user and kernel space. 
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
#ifndef __IRQ_H__
#define __IRQ_H__

/* Stabdard include */
#include "../include/std/std_types.h"

/* Device interrupt include */
#include "../devices/device_irq.h"

/*
 * FreeRTOS kernel Includes
 */
#include "./FreeRTOS-Kernel/include/FreeRTOS.h"
/*
 * END: FreeRTOS kernel Includes
 */



enum __kernel_interrupts_list
{
	KERN_IRQ_SYS_HARDFAULT = 0,
	KERN_IRQ_SYS_BUSFAULT,
	KERN_IRQ_SYS_USAGEFAULT,
	IRQ_COMM_1,
	IRQ_COMM_2,
	IRQ_COMM_3,
	MAX_IRQ_SERV
};




















/**
 * struct irqstat - interrupt statistics
 * @counter:	real-time interrupt count
 * @ref:	snapshot of interrupt count
 */
typedef struct  
{
	uint32_t				counter;
	uint32_t				service_ts;
}type_irq_irqstat;



/**
 * struct irq_common_data - per irq data shared by all irqchips
 * @id: 			Identifier between interrupts.
 * @handler_data: 	Handler Passed data 
 * @sem:			Counting semaphore
 */
typedef struct 
{
	uint32_t				id;
	void					*handler_data;
	uint32_t 				sem;
}type_irq_common_data;



/**
 * struct irq_data - per irq chip data passed down to chip functions
 * @mask:			precomputed bitmask for accessing the chip registers
 * @hw_irq_no:		HW interrupt number
 * @common_data:	Common data between different callbacks registration
 * @ll_drv_data:	Hardware interrupt status and memory allocation pointers
 */
typedef struct  
{
	uint32_t				mask;
	uint32_t				hw_irq_no;
	type_irq_common_data	*common_data;
	void					*ll_drv_data;
}type_irq_data;



/**
 * struct irq_desc -    interrupt descriptor
 * @irq_id:				Kernel mapped id for OS operation
 * @kstat_irqs:		    irq stats per cpu
 * @irq_data:		    Irq data User space to kernel_space
 * @irq_handle:			Handler registration from user space
 * @error_status: 		Error status on hardware status
 * @error_counter: 		Error counter value
 * @handled_ts:			last handled time stamp
 * @event_generated:	Generated events
 */
typedef struct  
{
	int32_t					irq_id;
	type_irq_irqstat		irq_stat;
	type_irq_data			irq_data;
	irq_handler_t			irq_handle;
	uint32_t				error_status;
	uint32_t				error_counter;
	uint32_t				handled_ts;
	uint32_t				event_generated;
}type_irq_desc;













#ifdef __cplusplus
extern "C" {
#endif


type_irq_desc * register_hw_cb(uint32_t interrupt_id, const irq_handler_t irq_handle_reg, uint32_t irq_idx); 









#ifdef __cplusplus
}
#endif





#endif