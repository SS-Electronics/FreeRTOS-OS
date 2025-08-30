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
SUBDIRS := init arch

INCLUDES :=

LINKER_SCRIPT :=

##############################################################




##############################################################
# Object list (collected from subdir Makefiles)
include $(patsubst %, %/Makefile, $(SUBDIRS))

# Prepend build/ to all objects
OBJS := $(addprefix $(BUILD)/, $(obj-y))

export INCLUDES

export LINKER_SCRIPT
##############################################################




##############################################################
# build stages 
all: $(BUILD)/kernel.elf

# Link final kernel
$(BUILD)/kernel.elf: $(OBJS) | $(BUILD)
	@echo '##############################################'
	@echo 'Linking together...'
	@echo '##############################################'

	$(CPP) $(CC_LINKER_FLAGS) -T"$(LINKER_SCRIPT)"  -o $@ $(OBJS)

	@echo '##############################################'
	@echo ' '
	@echo 'Build completed!'
	@echo ' '
	@echo '##############################################'

# Rule for compiling into build dir
$(BUILD)/%.o: %.c | $(BUILD)
	@echo '**********************************************'
	@echo 'Building C Source $< ...'
	@echo '***********************'
	@mkdir -p $(dir $@)
	$(CC) $(CC_OPTIMIZATION) $(CC_EXTRA_FLAGS) $(CC_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES) -c $< -o $@
	@echo '**********************************************'

$(BUILD)/%.o: %.s | $(BUILD)
	@echo '**********************************************'
	@echo 'Building Assembly source: $< ...'
	@echo '**********************************************'
	@mkdir -p $(dir $@)
	$(CC) $(CC_OPTIMIZATION) $(CC_ASSEMBLER_FLAGS) $(CC_EXTRA_FLAGS) $(CC_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES)-c $< -o $@




# Create build directory
$(BUILD):
	mkdir -p $(BUILD)
	@echo '##############################################'
	@echo 'Bulding sources...'
	@echo '##############################################'

##############################################################





clean:
	rm -rf $(BUILD)
	@echo '##############################################'
	@echo ' '
	@echo 'Clean completed!'
	@echo ' '
	@echo '##############################################'