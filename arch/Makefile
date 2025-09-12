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

# CMSIS Include in general always 

INCLUDES += -I$(CURDIR)/arch/arm/CMSIS_6





################### Device STM32F401 ############################
ifeq ($(CONFIG_TARGET_MCU),"STM32F401")

#---------- Include paths -------------- 
INCLUDES += -I$(CURDIR)/arch/devices/STM

#---------- Symbol Definition to compiler -------------- 
SYMBOL_DEF += 


#---------- System files -------------- 
obj-y += arch/devices/STM/STM32F401/stm32_f401_system.o
obj-y += arch/devices/STM/STM32F401/stm32_f401_startup.o

#---------- Driver files -------------- 





#---------- Associate Linker script for target MCU -------------- 
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32F401/stm32_f401_D_FLASH.ld

endif

#################################################################





################### Device STM32F411 ############################
ifeq ($(CONFIG_TARGET_MCU),"STM32F411")

#---------- Include paths -------------- 
INCLUDES += -I$(CURDIR)/arch/devices/STM

#---------- Symbol Definition to compiler -------------- 
SYMBOL_DEF += 

#---------- System files -------------- 
obj-y += arch/devices/STM/STM32F411/stm32_f411_system.o
obj-y += arch/devices/STM/STM32F411/stm32_f411_startup.o

#---------- Driver files -------------- 





#---------- Associate Linker script for target MCU -------------- 
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32F411/stm32_f411_FLASH.ld

endif

#################################################################