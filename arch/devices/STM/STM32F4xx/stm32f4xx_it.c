/*
File:        stm32f4xx_it.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       arch/device
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

#include "device.h"



/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/


void NMI_Handler(void)
{
  
}



void DebugMon_Handler(void)
{


}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/


void TIM1_UP_TIM10_IRQHandler(void)
{

  // HAL_TIM_IRQHandler(&htim1);

}


void USART1_IRQHandler(void)
{

  // HAL_UART_IRQHandler(&huart1);

}


void USART2_IRQHandler(void)
{

  // HAL_UART_IRQHandler(&huart2);

}




