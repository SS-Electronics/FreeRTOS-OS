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
INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Plus-CLI



# Newlib bare-metal glue — architecture-agnostic, required by all targets
obj-y += kernel/system.o
obj-y += kernel/syscalls.o
obj-y += kernel/sysmem.o

obj-y += kernel/FreeRTOS-Kernel/croutine.o
obj-y += kernel/FreeRTOS-Kernel/list.o
obj-y += kernel/FreeRTOS-Kernel/queue.o
obj-y += kernel/FreeRTOS-Kernel/stream_buffer.o
obj-y += kernel/FreeRTOS-Kernel/tasks.o
obj-y += kernel/FreeRTOS-Kernel/timers.o
obj-y += kernel/FreeRTOS-Kernel/event_groups.o
obj-y += kernel/kernel_thread.o
obj-y += kernel/FreeRTOS-Kernel/portable/MemMang/heap_4.o
obj-y += kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.o



#####################################################################
#                     Device STM32F411xE
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")

INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F

obj-y += kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.o

endif

#------------------------------------------------------------------




#####################################################################
#                     Device STM32H723xx
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32H723xx")

# ARM_CM7/r0p1 portable — handles all Cortex-M7 silicon (r0p1 and r1p1).
# The "r0p1" label refers to the errata workaround in port.c, not a
# silicon restriction; this port is the correct choice for H723 (r1p1).
INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1

obj-y += kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1/port.o

endif

#####################################################################
#                     Device STM32U575xx  (Cortex-M33 + TrustZone)
#
# ARM_CM33_NTZ/non_secure — non-TrustZone (single-image) port. Used when
# CONFIG_U5_TRUSTZONE_ENABLED=n (the example default).
# Switch to ARM_CM33/non_secure + ARM_CM33/secure when building the
# TZ-split firmware.
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32U575xx")

INCLUDES += -I$(CURDIR)/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure

obj-y += kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.o
obj-y += kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.o

endif