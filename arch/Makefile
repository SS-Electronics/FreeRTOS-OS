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





#####################################################################
#                     Device STM32F401xE 
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32F401xE")

#---------- Include paths -------------- 
INCLUDES += -I$(CURDIR)/arch/devices/STM

#---------- Symbol Definition to compiler -------------- 
SYMBOL_DEF += -DUSE_HAL_DRIVER


#---------- System files -------------- 
obj-y += arch/devices/STM/STM32F401/stm32_f401_system.o
obj-y += arch/devices/STM/STM32F401/stm32_f401_startup.o

#---------- Driver files -------------- 





#---------- Associate Linker script for target MCU -------------- 
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32F401/stm32_f401_D_FLASH.ld

endif

#------------------------------------------------------------------




#####################################################################
#                     Device STM32F411xE 
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")

#---------- Include paths --------------
INCLUDES += -I$(CURDIR)/arch/devices
INCLUDES += -I$(CURDIR)/arch/devices/device_conf
INCLUDES += -I$(CURDIR)/arch/devices/STM/STM32F4xx
INCLUDES += -I$(CURDIR)/arch/devices/STM/stm32f4xx-hal-driver/Inc


#---------- Symbol Definition to compiler -------------- 
SYMBOL_DEF += -DUSE_HAL_DRIVER

#---------- Device-level files (startup only — CMSIS system init is in hal_rcc_stm32.c) ------
obj-y += arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.o


#---------- Driver files -------------- 
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_rcc.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_gpio.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_cortex.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_flash.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_flash_ex.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_pwr.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_dma.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_tim.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_tim_ex.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_uart.o
# obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_uart_ex.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_i2c.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_spi.o


#---------- Associate Linker script for target MCU --------------
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32F4xx/STM32F411/stm32_f411_FLASH.ld

endif

#------------------------------------------------------------------




#####################################################################
#                     Device STM32H723xx
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32H723xx")

#---------- Include paths --------------
INCLUDES += -I$(CURDIR)/arch/devices
INCLUDES += -I$(CURDIR)/arch/devices/STM/STM32H7xx
INCLUDES += -I$(CURDIR)/arch/devices/STM/stm32h7xx-hal-driver/Inc
INCLUDES += -I$(CURDIR)/arch/arm/CMSIS_6/CMSIS/Core/Include

#---------- Symbol Definition to compiler --------------
SYMBOL_DEF += -DUSE_HAL_DRIVER

#---------- Device-level files (startup + H7 CMSIS system) ------
obj-y += arch/devices/STM/STM32H7xx/STM32H723/startup_stm32h723zx.o
obj-y += arch/devices/STM/STM32H7xx/system_stm32h7xx.o

#---------- Driver files --------------
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rcc.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_rcc_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_gpio.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_cortex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_flash.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_flash_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_pwr.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_pwr_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_dma.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_dma_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_tim.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_tim_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_uart.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_uart_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_i2c.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_i2c_ex.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_spi.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_spi_ex.o

# ADC — enabled when CONFIG_HAL_ADC_MODULE_ENABLED=y
ifeq ($(CONFIG_HAL_ADC_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_adc.o
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_adc_ex.o
endif

# IWDG — enabled when CONFIG_HAL_IWDG_MODULE_ENABLED=y
ifeq ($(CONFIG_HAL_IWDG_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32h7xx-hal-driver/Src/stm32h7xx_hal_iwdg.o
endif

#---------- Associate Linker script for target MCU --------------
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32H7xx/STM32H723/stm32_h723_FLASH.ld

endif

#------------------------------------------------------------------




#####################################################################
#                     Device STM32U575xx (Cortex-M33 + TrustZone)
#
# Board: Nucleo-U575ZI-Q (STM32U575ZITxQ)
#
# Drop the upstream HAL driver as a submodule to compile end-to-end:
#   git submodule add \
#       https://github.com/STMicroelectronics/stm32u5xx-hal-driver.git \
#       arch/devices/STM/stm32u5xx-hal-driver
#   git submodule add \
#       https://github.com/STMicroelectronics/cmsis-device-u5.git \
#       arch/devices/STM/cmsis-device-u5
#
# Without the HAL submodule the build still links the kernel, startup,
# system_stm32u5xx.c and the trustcore stub — useful for CI validation of
# the arch dispatch + linker.
#####################################################################
ifeq ($(CONFIG_TARGET_MCU),"STM32U575xx")

#---------- Include paths --------------
INCLUDES += -I$(CURDIR)/arch/devices
INCLUDES += -I$(CURDIR)/arch/devices/STM/STM32U5xx
# CMSIS device package (full chip header — GPIO_TypeDef, I2C_TypeDef, IRQn).
# Prefer the upstream cmsis-device-u5 when checked out; fall back to our
# in-tree minimal stub under STM32U575/ otherwise.
ifneq ($(wildcard $(CURDIR)/arch/devices/STM/cmsis-device-u5/Include/.),)
INCLUDES += -I$(CURDIR)/arch/devices/STM/cmsis-device-u5/Include
else
INCLUDES += -I$(CURDIR)/arch/devices/STM/STM32U5xx/STM32U575
endif
INCLUDES += -I$(CURDIR)/arch/arm/CMSIS_6/CMSIS/Core/Include
# HAL driver includes — present only when the submodule is checked out.
ifneq ($(wildcard $(CURDIR)/arch/devices/STM/stm32u5xx-hal-driver/Inc/.),)
INCLUDES += -I$(CURDIR)/arch/devices/STM/stm32u5xx-hal-driver/Inc
endif

#---------- Symbol Definition to compiler --------------
SYMBOL_DEF += -DUSE_HAL_DRIVER

#---------- Device-level files (startup + CMSIS system) ------
obj-y += arch/devices/STM/STM32U5xx/STM32U575/startup_stm32u575zitxq.o
obj-y += arch/devices/STM/STM32U5xx/system_stm32u5xx.o

#---------- Driver files (only when stm32u5xx-hal-driver submodule present)
ifneq ($(wildcard $(CURDIR)/arch/devices/STM/stm32u5xx-hal-driver/Src/.),)
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_rcc.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_rcc_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_gpio.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_cortex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_flash.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_flash_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_pwr.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_pwr_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_dma.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_dma_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_tim.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_tim_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_uart.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_uart_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_i2c.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_i2c_ex.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_spi.o
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_spi_ex.o

ifeq ($(CONFIG_HAL_ICACHE_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_icache.o
endif

ifeq ($(CONFIG_HAL_GTZC_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_gtzc.o
endif

ifeq ($(CONFIG_HAL_PKA_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_pka.o
endif

ifeq ($(CONFIG_HAL_IWDG_MODULE_ENABLED),1)
obj-y += arch/devices/STM/stm32u5xx-hal-driver/Src/stm32u5xx_hal_iwdg.o
endif
endif

#---------- Associate Linker script for target MCU --------------
LINKER_SCRIPT += $(CURDIR)/arch/devices/STM/STM32U5xx/STM32U575/stm32_u575_FLASH.ld

endif

#------------------------------------------------------------------