/*
File:        device.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:      arch  
Info:        Exports necessary header and handles              
Dependency:  CMSIS + HAL Driver repos + Device specific files
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

#ifndef __DEVICE_H__
#define __DEVICE_H__




/************************************************************
 * 
 * 
 *   Device : STM32F411xE
 *   
 *   Necessary Headers need to export to the system 
 *   and handles of the peripheral
 * 
 ***********************************************************/


#if defined(STM32F411xE) 

#include "def_attributes.h"

/* Generic device specifc include  */
#include "stm32f4xx.h"
#include "STM32F411/stm32_f411xe.h"
#include "system_stm32f4xx.h"
#include "stm32f4xx_it.h"

/* HAL Driver level include */
#include "stm32f4xx_hal_conf.h"
#include <stm32f4xx_hal.h>
#include "stm32f4xx_hal_tim.h"


/*  Exported system variables */
extern uint32_t             SystemCoreClock;      /*!< System Clock Frequency (Core Clock) */
extern const uint8_t        AHBPrescTable[16];    /*!< AHB prescalers table values */
extern const uint8_t        APBPrescTable[8];     /*!< APB prescalers table values */


/*  Exported Device handles */
extern UART_HandleTypeDef       huart1;
extern UART_HandleTypeDef       huart2;
extern TIM_HandleTypeDef        htim1;



#endif // ENDOF #if defined(STM32F411xE) 






#endif
