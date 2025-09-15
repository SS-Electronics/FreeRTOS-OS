/*
File:        system_stm32f4xx.h
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


#ifndef __SYSTEM_STM32F4XX_H
#define __SYSTEM_STM32F4XX_H

#include "device.h"





#ifdef __cplusplus
extern "C" {
#endif


extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);


#ifdef __cplusplus
}
#endif






#endif /*__SYSTEM_STM32F4XX_H */

