/*
File:        irq.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       include/os
Info:         generic interrupt controler description and api              
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

#ifndef __IRQ_H__
#define __IRQ_H__

/* Stabdard include */
#include "def_std.h"

/* Device interrupt include */

/*
 * FreeRTOS kernel Includes
 */

/*
 * END: FreeRTOS kernel Includes
 */



/**
 * struct irq_data - per irq chip data passed down to chip functions
 * @mask:		precomputed bitmask for accessing the chip registers
 * @irq:		interrupt number
 * @hwirq:		hardware interrupt number, local to the interrupt domain
 * @common:		point to data shared by all irqchips
 * @chip:		low level interrupt hardware access
 * @domain:		Interrupt translation domain; responsible for mapping
 *			between hwirq number and linux irq number.
 * @parent_data:	pointer to parent struct irq_data to support hierarchy
 *			irq_domain
 * @chip_data:		platform-specific per-chip private data for the chip
 *			methods, to allow shared chip implementations
 */
struct irq_data {
	uint32_t			mask;
	uint32_t			irq;
	irq_hw_id_t			hwirq_id;
	struct irq_chip		*chip;
	struct irq_domain	*domain;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_data		*parent_data;
#endif
	void			*chip_data;
};



/**
 * struct irq_chip - hardware interrupt chip descriptor
 * @irq_enable:		enable the interrupt (defaults to chip->unmask if NULL)
 * @irq_disable:	disable the interrupt
 * @irq_ack:		start of a new interrupt
 * @irq_mask:		mask an interrupt source
 * @irq_mask_ack:	ack and mask an interrupt source
 * @irq_unmask:		unmask an interrupt source
 * @irq_eoi:		end of interrupt
 * @irq_retrigger:	resend an IRQ to the CPU
 * @irq_set_type:	set the flow type (IRQ_TYPE_LEVEL/etc.) of an IRQ
 * @irq_set_wake:	enable/disable power-management wake-on of an IRQ
 * @irq_suspend:	function called from core code on suspend once per
 *			chip, when one or more interrupts are installed
 * @irq_resume:		function called from core code on resume once per chip,
 *			when one ore more interrupts are installed
 * @irq_pm_shutdown:	function called from core code on shutdown once per chip
 * @irq_get_irqchip_state:	return the internal state of an interrupt
 * @irq_set_irqchip_state:	set the internal state of a interrupt
 * @flags:		chip specific flags
 */
struct irq_chip 
{
	const char		*name;
	void			(*irq_enable)(struct irq_data *data);
	void			(*irq_disable)(struct irq_data *data);
	void			(*irq_ack)(struct irq_data *data);
	void			(*irq_mask)(struct irq_data *data);
	void			(*irq_mask_ack)(struct irq_data *data);
	void			(*irq_unmask)(struct irq_data *data);
	void			(*irq_eoi)(struct irq_data *data);
	int				(*irq_retrigger)(struct irq_data *data);
	int				(*irq_set_type)(struct irq_data *data, unsigned int flow_type);
	int				(*irq_set_wake)(struct irq_data *data, unsigned int on);
	void			(*irq_suspend)(struct irq_data *data);
	void			(*irq_resume)(struct irq_data *data);
	void			(*irq_pm_shutdown)(struct irq_data *data);
	int				(*irq_get_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool *state);
	int				(*irq_set_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool state);

	unsigned long	flags;
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