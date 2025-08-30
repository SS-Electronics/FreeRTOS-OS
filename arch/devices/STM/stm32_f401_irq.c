/*
 * File:        stm32_f401cdu6.c
 * Author:      Subhajit Roy  
 *              subhajitroy005@gmail.com 
 *
 * Moudle:      Modeule Device [ Local Build ] 
 * Info:        STM32F401CUD6 Vectors callback implementation  
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
#include "device_irq.h"

// #define NO_OF_CPU_INTERRUPTS    4
// #define NO_OF_COMM_INTERRUPTS   3





// #define NO_OF_TOTAL_INTERRUPTS  (NO_OF_CPU_INTERRUPTS + NO_OF_COMM_INTERRUPTS)



// static type_device_cpu_irq_handle  cache_device_cpu_irq_handle[NO_OF_CPU_INTERRUPTS];





// type_device_cpu_irq_handle * dev_get_irq_cpu_handle(uint32_t irq_idx)
// {
//     if(irq_idx < NO_OF_TOTAL_INTERRUPTS )
//     {
//         /* Reset the interrupt counters upon handle call / init */
//         cache_device_cpu_irq_handle[irq_idx].hw_irq_counter = 0;

//         return &cache_device_cpu_irq_handle[irq_idx];
//     }
//     else
//     {
//         return NULL;
//     }
// }


// void RTC_WKUP_IRQHandler(void)
// {
//     if (cache_device_cpu_irq_handle[0].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[0].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[0].hw_irq_counter++;
//     }
// }

// void RCC_IRQHandler(void)
// {
//     if (cache_device_cpu_irq_handle[1].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[1].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[1].hw_irq_counter++;
//     }
// }

// void TIM2_IRQHandler(void)
// {
//     if (cache_device_cpu_irq_handle[2].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[2].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[2].hw_irq_counter++;
//     }
// }

// // void WWDG_IRQHandler(void)
// // {
// //     int a = 10;
// // }

// // void FLASH_IRQHandler(void)
// // {
// //     int a = 10;
// // }

// // void NMI_Handler(void)
// // {
// //     int a = 10;
// // }

// void HardFault_Handler(void)
// {
//     if (cache_device_cpu_irq_handle[3].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[3].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[3].hw_irq_counter++;
//     }
// }

// void MemManage_Handler(void)
// {
//     if (cache_device_cpu_irq_handle[4].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[4].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[4].hw_irq_counter++;
//     }
// }

// void BusFault_Handler(void)
// {
//     if (cache_device_cpu_irq_handle[5].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[5].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[5].hw_irq_counter++;
//     }
// }

// void UsageFault_Handler(void)
// {
//     if (cache_device_cpu_irq_handle[6].irq_handler != NULL)
//     {
//         (*cache_device_cpu_irq_handle[6].irq_handler)(NULL);
//         cache_device_cpu_irq_handle[6].hw_irq_counter++;
//     }
// }