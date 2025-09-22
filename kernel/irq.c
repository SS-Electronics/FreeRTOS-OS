/*
 * File:        irq.c
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

#include "irq.h"



static type_irq_desc cache_irq_desc[ MAX_IRQ_SERV ];









type_irq_desc * register_hw_cb(uint32_t interrupt_id, const irq_handler_t irq_handle_reg, uint32_t irq_idx)
{
    /* Get the indexed hardware handles */
    type_device_cpu_irq_handle* dev_irq_handle = dev_get_irq_cpu_handle(irq_idx);

    if( (dev_irq_handle != NULL)    && \
        (interrupt_id < MAX_IRQ_SERV)   \
      )
    {
        /* Assign IRQ ID This is for dynamic time allocation */
        cache_irq_desc[interrupt_id].irq_id                 = interrupt_id;

        /* Registering callback device_interrupt */
        dev_irq_handle->irq_handler                         = irq_handle_reg;
        cache_irq_desc[interrupt_id].irq_handle             = irq_handle_reg;
        
        /* Clearing all variables */
        cache_irq_desc[interrupt_id].error_counter          = 0;
        cache_irq_desc[interrupt_id].error_status           = 0;
        cache_irq_desc[interrupt_id].event_generated        = 0;
        cache_irq_desc[interrupt_id].handled_ts             = 0;
        cache_irq_desc[interrupt_id].irq_stat.counter       = 0;
        cache_irq_desc[interrupt_id].irq_stat.service_ts    = 0;

        return &cache_irq_desc[interrupt_id];
    }
    else
    {
        return NULL;
    }
}