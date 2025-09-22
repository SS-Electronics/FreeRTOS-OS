# File:        Makefile
# Author:      Subhajit Roy  
#              subhajitroy005@gmail.com 
#
# Moudle:      Build  
# Info:        Build executables of Kernel              
# Dependency:  Configuration generation
#
# This file is part of FreeRTOS-OS Project.
# 
# FreeRTOS-OS is free software: you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation, either version 
# 3 of the License, or (at your option) any later version.
#
# FreeRTOS-OS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.

include autoconf.mk


INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Kernel/include



# obj-y += kernel/FreeRTOS-Kernel/croutine.o
# obj-y += kernel/FreeRTOS-Kernel/list.o
# obj-y += kernel/FreeRTOS-Kernel/queue.o
# obj-y += kernel/FreeRTOS-Kernel/stream_buffer.o
# obj-y += kernel/FreeRTOS-Kernel/tasks.o
# obj-y += kernel/FreeRTOS-Kernel/timers.o
# obj-y += kernel/FreeRTOS-Kernel/event_groups.o
# obj-y += kernel/kernel_thread.o
# obj-y += kernel/FreeRTOS-Kernel/portable/MemMang/heap_4.o



#####################################################################
#                     Device STM32F401xE 
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")
# Portable part 

INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F

# obj-y += kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.o


endif