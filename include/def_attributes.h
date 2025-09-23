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
along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>. */

/* Bootloader placed section */
#define __SECTION_BOOT          __attribute__((section(".boot")))
#define __SECTION_BOOT_DATA     __attribute__((section(".boot_data")))

/* All OS and kernel related functions need to be placed in this section */
#define __SECTION_OS            __attribute__((section(".os")))
#define __SECTION_OS_DATA       __attribute__((section(".os_data")))

/* Application section  */
#define __SECTION_APP            __attribute__((section(".app")))
#define __SECTION_APP_DATA       __attribute__((section(".app_data")))
