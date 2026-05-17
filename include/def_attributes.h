/*
File:        def_attributes.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       include
Info:         all attributes definitions              
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
along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>. */

/* Section placement macros — resolved to empty: all code goes into the flat
 * .text / .data sections so --gc-sections can eliminate dead code. */
#define __SECTION_BOOT
#define __SECTION_BOOT_DATA
#define __SECTION_OS
#define __SECTION_OS_DATA
#define __SECTION_APP
#define __SECTION_APP_DATA

#ifndef __USED
#define __USED                   __attribute__((used))
#endif

#ifndef __KEEP
#define __KEEP                   __attribute__((keep))
#endif

/* Not initialised at startup — persists across soft (NVIC) resets.
 * Requires .noinit section in linker script placed outside [__bss_start__, __bss_end__]. */
#define __SECTION_NOINIT         __attribute__((section(".noinit")))