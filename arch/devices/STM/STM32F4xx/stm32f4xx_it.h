/*
File:        stm32f4xx_it.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       arch/device
Info:         all system level functions before main() entry             
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

#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H



#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void DebugMon_Handler(void);
void TIM1_UP_TIM10_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif




#endif /* __STM32F4xx_IT_H */
