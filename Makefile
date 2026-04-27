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
# autoconf.mk  — included by sub-Makefiles for conditional compilation
AUTOCONF_MK     			:= autoconf.mk
# autoconf.h   — included by C source / headers for CONFIG_* symbols
AUTOCONF_H      			:= config/autoconf.h
##############################################################


##############################################################
# Board selection
# Board files live in the application tree (APP_DIR/board/).
# Override board name:  make CONFIG_BOARD=my_board APP_DIR=../app
CONFIG_BOARD    ?= stm32f411_devboard
export CONFIG_BOARD

ifdef APP_DIR
BOARD_XML          := $(APP_DIR)/board/$(CONFIG_BOARD).xml
BOARD_BSP_C        := $(APP_DIR)/board/board_config.c
BOARD_BSP_H        := $(APP_DIR)/board/board_device_ids.h
BOARD_HANDLES_H    := $(APP_DIR)/board/board_handles.h
BOARD_CONFIG_H     := $(APP_DIR)/board/board_config.h
else
# Standalone kernel build — no board config; stubs in include/board/ are used.
BOARD_XML          :=
BOARD_BSP_C        :=
BOARD_BSP_H        :=
BOARD_HANDLES_H    :=
BOARD_CONFIG_H     :=
endif
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
CPP_INPUT_STD				:= -std=gnu++14
CC_WARNINGS					:= -Wall
CC_TARGET_PROP				:= 
CC_LINKER_INPUT				:= -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
CC_ASSEMBLER_FLAGS			:= -x assembler-with-cpp
CC_LINKER_FLAGS				:= -mcpu=cortex-m4 -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
##############################################################






##############################################################
# Directory setup
BUILD   := build

# Subdirectories
SUBDIRS := include arch irq drivers kernel mm ipc services drv_app drv_ext_chips shell lib init

INCLUDES :=

# When APP_DIR is set, add it first so app/board/ headers shadow include/board/ stubs
ifdef APP_DIR
INCLUDES += -I$(APP_DIR)
endif

LINKER_SCRIPT :=

SYMBOL_DEF :=

TARGET_SYSMBOL_DEF +=

OPENOCD_INTERFACE :=

OPENOCD_TARGET:=

# Application integration
# Set APP_DIR to the path of your application (relative to this Makefile).
# Example: make app APP_DIR=../app
APP_DIR     ?=
APP_INCLUDES :=

# Output binary name: 'kernel' for standalone OS, 'app' when APP_DIR is set.
TARGET_NAME ?= kernel
##############################################################






##############################################################
# Object list (collected from subdir Makefiles)
include $(patsubst %, %/Makefile, $(SUBDIRS))

# Prepend build/ to all objects
OBJS := $(addprefix $(BUILD)/, $(obj-y))

# Include app sources when APP_DIR is provided
ifdef APP_DIR
-include $(APP_DIR)/Makefile
APP_OBJS := $(addprefix $(BUILD)/app/, $(app-obj-y))
OBJS     += $(APP_OBJS)
endif

export INCLUDES

export LINKER_SCRIPT

export SYMBOL_DEF


# Include generated Makefile configs if they exist
-include $(AUTOCONF_MK)

# File generation based on the input targets 
TARGET_SYSMBOL_DEF += -D$(patsubst "%",%,$(CONFIG_TARGET_MCU))

STM32_HAL_CONF_H :=

ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")
	STM32_HAL_CONF_H   := arch/devices/device_conf/stm32f4xx_hal_conf.h
	OPENOCD_TARGET     += arch/debug_cfg/stm32_f411xx_debug.cfg
	OPENOCD_INTERFACE  += interface/stlink.cfg
	CC_TARGET_PROP     += -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
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




config-outputs: $(AUTOCONF_MK) $(AUTOCONF_H) $(STM32_HAL_CONF_H)

# ── autoconf.mk / autoconf.h — only generated when .config exists ────────────
# Without a .config (i.e. before menuconfig has been run) the build still
# proceeds; empty stub files are created so -include doesn't cause an error.
ifneq ($(wildcard $(KCONFIG_CONFIG)),)

$(AUTOCONF_MK): $(KCONFIG_CONFIG)
	@echo "### Generating $@ from $(KCONFIG_CONFIG)"
	@rm -f $@
	@sed -ne 's/^\(CONFIG_[A-Za-z0-9_]*\)=y/\1=1/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]*\)=n/\1=0/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]*\)=\([0-9][0-9]*\)/\1=\2/p' \
	       -e 's/^\(CONFIG_[A-Za-z0-9_]*\)="\(.*\)"/\1="\2"/p' \
	       $(KCONFIG_CONFIG) | sort -u > $@
	@echo "### $@ done"

$(AUTOCONF_H): $(KCONFIG_CONFIG)
	@echo "### Generating $@ from $(KCONFIG_CONFIG)"
	@mkdir -p $(dir $@)
	@rm -f $@
	@printf '/* AUTO-GENERATED - do not edit. Re-run: make config-outputs */\n' > $@
	@printf '#ifndef __AUTOCONF_H__\n#define __AUTOCONF_H__\n\n'              >> $@
	@sed -ne 's/^\(CONFIG_[A-Za-z0-9_]*\)=y/#define \1 1/p' \
	        -e 's/^\(CONFIG_[A-Za-z0-9_]*\)=\([0-9][0-9]*\)/#define \1 \2/p' \
	        -e 's/^\(CONFIG_[A-Za-z0-9_]*\)="\(.*\)"/#define \1 "\2"/p' \
	        $(KCONFIG_CONFIG) | sort -u                                         >> $@
	@printf '\n#endif /* __AUTOCONF_H__ */\n'                                  >> $@
	@echo "### $@ done"

else

# No .config yet — create empty stubs so -include is a no-op.
$(AUTOCONF_MK):
	@touch $@

$(AUTOCONF_H):
	@mkdir -p $(dir $@)
	@touch $@

endif

# ── STM32 HAL conf — generated from a static template (not from .config) ─────
# Content is fixed; all CONFIG_* logic is resolved by the C preprocessor.
# Depends on autoconf.h so it is refreshed whenever Kconfig changes.
ifneq ($(STM32_HAL_CONF_H),)
$(STM32_HAL_CONF_H): $(AUTOCONF_H)
	@echo "### Generating $@ from $(AUTOCONF_H) ..."
	@python3 scripts/gen_stm32_hal_conf.py $(AUTOCONF_H) $@
endif
##############################################################








##############################################################
# IRQ table code generation
# Reads APP_DIR/board/irq_table.xml and writes:
#   irq/irq_table.c                    — software IRQ name table
#   APP_DIR/board/irq_hw_init_generated.c/.h — NVIC priority / binding init
# Run: make irq_gen APP_DIR=../app
.PHONY: irq_gen
ifdef APP_DIR
irq_gen:
	@echo "### Generating IRQ table from $(APP_DIR)/board/irq_table.xml ..."
	@python3 scripts/gen_irq_table.py $(APP_DIR)/board/irq_table.xml
	@echo "### IRQ table generation done"
else
irq_gen:
	@echo "### irq_gen requires APP_DIR — e.g.: make irq_gen APP_DIR=../app"
	@exit 1
endif
##############################################################

##############################################################
# Board BSP generation
# Board files live in APP_DIR/board/. Requires APP_DIR to be set.
# Run: make board-gen APP_DIR=../app
ifdef APP_DIR
$(BOARD_BSP_C) $(BOARD_BSP_H) $(BOARD_HANDLES_H) $(BOARD_CONFIG_H): $(BOARD_XML)
	@echo "### Generating BSP from $< ..."
	@python3 scripts/gen_board_config.py $< \
		--outdir $(APP_DIR)/board
	@echo "### BSP generation done"

.PHONY: board-gen
board-gen: $(BOARD_BSP_C) $(BOARD_BSP_H) $(BOARD_HANDLES_H) $(BOARD_CONFIG_H)
	@echo "### Board: $(CONFIG_BOARD)  XML: $(BOARD_XML)"
	@echo "### Regenerating IntelliSense configuration ..."
	@bash scripts/gen_intellisense.sh $(patsubst "%",%,$(CONFIG_TARGET_MCU))
	@echo "### IntelliSense done"
else
.PHONY: board-gen
board-gen:
	@echo "### board-gen requires APP_DIR — e.g.: make board-gen APP_DIR=../app"
	@exit 1
endif
##############################################################


##############################################################
# build stages
# BSP files are generated before compiling any C source (when APP_DIR is set).
ifdef APP_DIR
BOARD_PREREQS := $(BOARD_BSP_C) $(BOARD_BSP_H) $(BOARD_HANDLES_H) $(BOARD_CONFIG_H)
else
BOARD_PREREQS :=
endif
all: $(BOARD_PREREQS) $(BUILD)/$(TARGET_NAME).elf

# Link final binary
$(BUILD)/$(TARGET_NAME).elf: $(OBJS) | $(BUILD) $(AUTOCONF)
	@echo '**********************************************'
	@echo 'Linking together...'
	@echo '**********************************************'

	@$(CPP) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_LINKER_FLAGS) -T"$(LINKER_SCRIPT)" -o $@ $(OBJS)

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
	@echo ' '
	@echo 'Generating Assembly'
	@arm-none-eabi-objdump -D $@ > $@.asm
	@echo '##############################################'
	@echo ' '
	@echo 'Generating Symbol Map'
	@arm-none-eabi-nm --numeric-sort --print-size $@ > $@.sym
	@echo ' Symbol map: $@.sym'
	@echo '##############################################'

# Rule for compiling OS sources into build dir
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

# Rule for compiling app sources into build/app/
$(BUILD)/app/%.o: $(APP_DIR)/%.c | $(BUILD) $(AUTOCONF)
	@echo '----------------------------------------------'
	@echo 'Building App Source $< ...'
	@echo '----------------------'
	@mkdir -p $(dir $@)
	@$(CC) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_OPTIMIZATION) $(CC_EXTRA_FLAGS) $(CC_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES) $(APP_INCLUDES) -c $< -o $@
	@echo '**********************************************'

# Rule for compiling app C++ sources into build/app/
$(BUILD)/app/%.o: $(APP_DIR)/%.cpp | $(BUILD) $(AUTOCONF)
	@echo '----------------------------------------------'
	@echo 'Building App C++ Source $< ...'
	@echo '----------------------'
	@mkdir -p $(dir $@)
	@$(CPP) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_OPTIMIZATION) $(CC_EXTRA_FLAGS) $(CPP_INPUT_STD) $(CC_WARNINGS) $(CC_TARGET_PROP) $(INCLUDES) $(APP_INCLUDES) -c $< -o $@
	@echo '**********************************************'

# Create build directory
$(BUILD):
	@mkdir -p $(BUILD)
	@echo '##############################################'
	@echo 'Bulding sources...'
	@echo '##############################################'



clean:
	@rm -rf $(BUILD)
ifdef APP_DIR
	@rm -f $(BOARD_BSP_C) $(BOARD_BSP_H) $(BOARD_HANDLES_H) $(BOARD_CONFIG_H)
endif
	@echo '##############################################'
	@echo ' '
	@echo 'Clean completed!'
	@echo ' '
	@echo '##############################################'

##############################################################


##############################################################
# Flash target — programs the ELF into MCU flash via OpenOCD
# Usage:
#   make flash                      (uses default CONFIG_BOARD)
#   make flash CONFIG_BOARD=my_board
#
# OpenOCD 'program' command:
#   verify  — reads back flash and compares with ELF (catches write errors)
#   reset   — issues a system reset after programming
#   exit    — closes OpenOCD immediately after flashing
flash: $(BUILD)/$(TARGET_NAME).elf
	@echo '##############################################'
	@echo 'Flashing $(BUILD)/$(TARGET_NAME).elf via OpenOCD ...'
	@echo '##############################################'
	openocd -f $(OPENOCD_TARGET) \
	        -c "program $(BUILD)/$(TARGET_NAME).elf verify reset exit"
	@echo ' '
	@echo 'Flash complete. Target is running.'
	@echo '##############################################'
##############################################################


##############################################################
# Documentation
DOXYGEN  ?= doxygen
DOXYFILE ?= Doxyfile
DOC_DIR  ?= docs/generated

docs:
	@echo "Generating documentation..."
	$(DOXYGEN) $(DOXYFILE)
	@echo "Documentation generated in $(DOC_DIR)/html"

clean-docs:
	@echo "Cleaning documentation..."
	@rm -rf $(DOC_DIR)/html
	@echo "Documentation cleaned."
##############################################################


##############################################################
# Prerequisite installation
#
# Usage (run once after cloning):
#   make install-prerequisites        →  everything below in order
#   make install-toolchain            →  ARM GCC + GDB  (arm-none-eabi-*)
#   make install-openocd              →  OpenOCD flash/debug server
#   make install-kconfig              →  kconfig-frontends  (make menuconfig)
#   make install-debug-tools          →  SVD file + VS Code extensions
#   make install-doxygen              →  Doxygen + Graphviz  (optional, for docs)

SCRIPTS_DIR := scripts

.PHONY: install-prerequisites install-toolchain install-openocd \
        install-kconfig install-debug-tools install-doxygen

install-toolchain:
	@echo '##############################################'
	@echo ' Installing ARM GCC Toolchain (arm-none-eabi)'
	@echo '##############################################'
	@bash $(SCRIPTS_DIR)/install_arm_gcc.sh

install-openocd:
	@echo '##############################################'
	@echo ' Installing OpenOCD'
	@echo '##############################################'
	@bash $(SCRIPTS_DIR)/install_openocd.sh

install-kconfig:
	@echo '##############################################'
	@echo ' Installing kconfig-frontends (menuconfig)'
	@echo '##############################################'
	@bash $(SCRIPTS_DIR)/install_kconfig.sh

install-debug-tools:
	@echo '##############################################'
	@echo ' Installing debug tools (SVD + VS Code extensions)'
	@echo '##############################################'
	@bash $(SCRIPTS_DIR)/install_debug_tools.sh

install-doxygen:
	@echo '##############################################'
	@echo ' Installing Doxygen + Graphviz'
	@echo '##############################################'
	@bash $(SCRIPTS_DIR)/install_doxygen.sh

install-prerequisites: install-toolchain install-openocd install-kconfig install-debug-tools
	@echo ''
	@echo '============================================='
	@echo ' All prerequisites installed.'
	@echo ''
	@echo ' Next steps:'
	@echo '   make menuconfig                       configure target MCU'
	@echo '   make config-outputs                   generate autoconf.h / autoconf.mk'
	@echo '   make all APP_DIR=../app               build firmware'
	@echo '   make flash                            program the target'
	@echo '============================================='
##############################################################


##############################################################
.PHONY: print-interface print-target flash docs clean-docs

print-interface:
	@echo $(OPENOCD_INTERFACE)

print-target:
	@echo $(OPENOCD_TARGET)
##############################################################