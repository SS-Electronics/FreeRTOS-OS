# File:        Makefile
# Author:      Subhajit Roy  
#              subhajitroy005@gmail.com 

# Moudle:      Build  
# Info:        Build executables of Kernel              
# Dependency:  Configuration generation

# This file is part of FreeRTOS-OS Project.

# FreeRTOS-OS is free software: you can redistribute it and/or 
# modify it under the terms of the GNU General Public License 
# as published by the Free Software Foundation, either version 
# 3 of the License, or (at your option) any later version.

# FreeRTOS-OS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License 
# along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.




##############################################################
# Path to Kconfig tools
KCONFIG_MCONF 				?= kconfig-mconf   # from kconfig-frontends
KCONFIG_CONF  				?= kconfig-conf    # from kconfig-frontends
KCONFIG_FILE  				?= Kconfig
KCONFIG_CONFIG 				?= .config
AUTOCONF_MK     			:= autoconf.mk
AUTOCONF_H      			:= 
##############################################################






##############################################################
# conpiler setup and configuration
CC 							:= arm-none-eabi-gcc
CPP							:= arm-none-eabi-g++
CC_OBJDUMP					:= arm-none-eabi-objdump
CC_OBJCPY					:= arm-none-eabi-objcopy
CC_OPTIMIZATION				:= -O0 -g3 -c
CC_EXTRA_FLAGS				:= --specs=nano.specs
CC_INPUT_STD				:= -std=gnu99
CC_WARNINGS					:= -Wall
CC_TARGET_PROP				:= -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard 
CC_LINKER_INPUT				:= -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
CC_ASSEMBLER_FLAGS			:= -x assembler-with-cpp
CC_LINKER_FLAGS				:= -mcpu=cortex-m4 -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
##############################################################






##############################################################
# Directory setup
BUILD   := build

# Subdirectories
SUBDIRS := arch kernel mm init  include

INCLUDES :=

LINKER_SCRIPT :=

SYMBOL_DEF :=

TARGET_SYSMBOL_DEF +=

OPENOCD_INTERFACE :=

OPENOCD_TARGET:= 
##############################################################






##############################################################
# Object list (collected from subdir Makefiles)
include $(patsubst %, %/Makefile, $(SUBDIRS))

# Prepend build/ to all objects
OBJS := $(addprefix $(BUILD)/, $(obj-y))

export INCLUDES

export LINKER_SCRIPT

export SYMBOL_DEF


# Include generated Makefile configs if they exist
-include $(AUTOCONF_MK)

# File generation based on the input targets 
TARGET_SYSMBOL_DEF += -D$(patsubst "%",%,$(CONFIG_TARGET_MCU))

ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")
	AUTOCONF_H += arch/devices/STM/stm32f4xx_hal_conf.h
	OPENOCD_TARGET += arch/debug_cfg/stm32_f411xx_debug.cfg
	OPENOCD_INTERFACE += interface/stlink.cfg 

endif

export OPENOCD_INTERFACE

export OPENOCD_TARGET


##############################################################







##############################################################
# Kconfig integration
menuconfig:
	@if ! command -v $(KCONFIG_MCONF) >/dev/null 2>&1; then \
		echo "Error: missing '$(KCONFIG_MCONF)' (install kconfig-frontends)"; \
		exit 1; \
	fi
	$(KCONFIG_MCONF) $(KCONFIG_FILE)

oldconfig:
	$(KCONFIG_CONF) --olddefconfig $(KCONFIG_FILE)




config-outputs: $(AUTOCONF_MK)

$(AUTOCONF_MK): $(KCONFIG_CONFIG)
	@echo "### Generating $@ from $(KCONFIG_CONFIG)"
	@rm -f $@
	@sed -ne 's/^\(CONFIG_[A-Za-z0-9_]\+\)=y/\1=1/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=n/\1=0/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=\([0-9]\+\)/\1=\2/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=\"\(.*\)\"/\1="\2"/p' \
	       $(KCONFIG_CONFIG) | sort -u > $@

# $(AUTOCONF_H): $(KCONFIG_CONFIG)
# 	@echo "### Generating $@ from $(KCONFIG_CONFIG)"
# 	@rm -f $@
# 	@echo "/* Auto-generated config header */" > $@
# 	@echo "#pragma once" >> $@
# 	@sed -ne 's/^\(CONFIG_[A-Za-z0-9_]\+\)=y/#define \1 1/p' \
# 	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=n/\/\* #undef \1 \*\//p' \
# 	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=\([0-9]\+\)/#define \1 \2/p' \
# 	       -e 's/^\(CONFIG_[A-Za-z0-9_]\+\)=\"\(.*\)\"/#define \1 "\2"/p' \
# 	       $(KCONFIG_CONFIG) | sort -u >> $@

# $(AUTOCONF_H): $(AUTOCONF_MK)
# 	@echo "### Generating $@ from $(KCONFIG_CONFIG)"
# 	@rm -f $@
# 	@echo "/* Auto-generated config header */" > $@
# 	@echo "#ifndef __STM32F4xx_HAL_CONF_H" >> $@
# 	@echo "#define __STM32F4xx_HAL_CONF_H \n\n\n" >> $@
# 	@echo "#define HAL_MODULE_ENABLED \n\n\n\n" >> $@



# 	@sed -ne 's/^CONFIG_\([A-Za-z0-9_]\+\)=y/#define \1 1/p' \
# 	       -e 's/^CONFIG_\([A-Za-z0-9_]\+\)=n/\/\* #undef \1 \*\//p' \
# 	       -e 's/^CONFIG_\([A-Za-z0-9_]\+\)=\([0-9]\+\)/#define \1 \2/p' \
# 	       -e 's/^CONFIG_\([A-Za-z0-9_]\+\)=\"\(.*\)\"/#define \1 "\2"/p' \
# 	       $(KCONFIG_CONFIG) >> $@














# 	@echo "\n\n\n#endif" >> $@   


##############################################################








##############################################################
# build stages 
all: $(BUILD)/kernel.elf 

# Link final kernel
$(BUILD)/kernel.elf: $(OBJS) | $(BUILD) $(AUTOCONF)
	@echo '**********************************************'
	@echo 'Linking together...'
	@echo '**********************************************'

	@$(CPP) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_LINKER_FLAGS) -T"$(LINKER_SCRIPT)"  -o $@ $(OBJS)

	@echo '##############################################'
	@echo ' '
	@echo 'Build completed!:   $@'
	@echo ' '
	@arm-none-eabi-size -Ax $@
	@arm-none-eabi-size $@
	@echo ' '
	@echo '##############################################'
	@echo ' '
	@echo 'Generating HEX'
	@arm-none-eabi-objcopy -O ihex $@ $@.hex
	@echo ' '
	@echo '##############################################'

# Rule for compiling into build dir
$(BUILD)/%.o: %.c | $(BUILD) $(AUTOCONF)
	@echo '----------------------------------------------'
	@echo 'Building C Source $< ...'
	@echo '----------------------'
	@mkdir -p $(dir $@)
	@$(CC) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_OPTIMIZATION) $(CC_EXTRA_FLAGS) $(CC_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES) -c $< -o $@
	@echo '**********************************************'

$(BUILD)/%.o: %.s | $(BUILD) $(AUTOCONF)
	@echo '----------------------------------------------'
	@echo 'Building Assembly source: $< ...'
	@echo '----------------------'
	@mkdir -p $(dir $@)
	@$(CC) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_OPTIMIZATION) $(CC_ASSEMBLER_FLAGS) $(CC_EXTRA_FLAGS) $(CC_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES)-c $< -o $@
	@echo '**********************************************'

# Create build directory
$(BUILD):
	@mkdir -p $(BUILD)
	@echo '##############################################'
	@echo 'Bulding sources...'
	@echo '##############################################'



clean:
	@rm -rf $(BUILD)
	@echo '##############################################'
	@echo ' '
	@echo 'Clean completed!'
	@echo ' '
	@echo '##############################################'

##############################################################


##############################################################
.PHONY: print-interface print-target

print-interface:
	@echo $(OPENOCD_INTERFACE)

print-target:
	@echo $(OPENOCD_TARGET)

##############################################################