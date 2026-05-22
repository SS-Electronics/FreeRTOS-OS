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
# autoconf.h — in $(APP_DIR)/board/ when APP_DIR is set; else config/
# Uses deferred assignment (=) so APP_DIR is resolved at rule-expansion time.
AUTOCONF_H      			= $(if $(APP_DIR),$(APP_DIR)/board/autoconf.h,config/autoconf.h)
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
CC_OPTIMIZATION				:= -Os -g -ffunction-sections -fdata-sections -c
CC_EXTRA_FLAGS				:= --specs=nano.specs
CC_INPUT_STD				:= -std=gnu99
CPP_INPUT_STD				:= -std=gnu++14
CC_WARNINGS					:= -Wall
CC_TARGET_PROP				:= 
CC_LINKER_INPUT				:= -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
CC_ASSEMBLER_FLAGS			:= -x assembler-with-cpp
# CPU-specific flags (-mcpu, -mfpu, -mfloat-abi, -mthumb) are set per-target
# in CC_TARGET_PROP below and appended to the link command — not hardcoded here.
CC_LINKER_FLAGS				:= -Wl,--gc-sections -static --specs=nano.specs -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
##############################################################






##############################################################
# Directory setup
BUILD   := build

# Subdirectories
SUBDIRS := include arch irq drivers kernel mm ipc services drv_app drv_ext_chips shell lib init safety log

INCLUDES :=

# When APP_DIR is set, add it first so app/board/ headers shadow include/board/ stubs.
# Also add APP_DIR/board/ directly so <autoconf.h>, <board/board_handles.h> etc. resolve.
ifdef APP_DIR
INCLUDES += -I$(APP_DIR)
INCLUDES += -I$(APP_DIR)/board
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
STM32_HAL_CONF_H   := $(if $(APP_DIR),$(APP_DIR)/board/stm32f4xx_hal_conf.h,config/stm32f4xx_hal_conf.h)
OPENOCD_TARGET     += arch/debug/target/stm32_f411xx_debug.cfg
OPENOCD_INTERFACE  += interface/stlink.cfg
CC_TARGET_PROP     += -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
endif

ifeq ($(CONFIG_TARGET_MCU),"STM32H723xx")
STM32_HAL_CONF_H   := $(if $(APP_DIR),$(APP_DIR)/board/stm32h7xx_hal_conf.h,config/stm32h7xx_hal_conf.h)
OPENOCD_TARGET     += arch/debug/target/stm32_h723xx_debug.cfg
OPENOCD_INTERFACE  += interface/stlink.cfg
CC_TARGET_PROP     += -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
endif

ifeq ($(CONFIG_TARGET_MCU),"STM32U575xx")
STM32_HAL_CONF_H   := $(if $(APP_DIR),$(APP_DIR)/board/stm32u5xx_hal_conf.h,config/stm32u5xx_hal_conf.h)
OPENOCD_TARGET     += arch/debug/target/stm32_u575xx_debug.cfg
OPENOCD_INTERFACE  += interface/stlink.cfg
# Cortex-M33 — single-precision FPU (fpv5-sp-d16), DSP extensions present on U5.
# -mcmse needed only when building secure-world entries; the single-image
# example targets non-secure so we leave -mcmse off here.
CC_TARGET_PROP     += -mthumb -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard
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
# Devboard examples — standalone FreeRTOS-OS without the sibling app/
# repository.
#
# Each supported devboard lives under examples/<target>/ and is a fully
# self-contained tree:
#
#   examples/stm32f411/
#     app_main.c, Makefile, kconfig.conf, os_conf_include/,
#     board/stm32f411_devboard.xml, board/irq_table.xml,
#     board/mcu_config.h    (tracked inputs)
#   examples/stm32f411/board/
#     board_config.{c,h}, board_device_ids.h, board_handles.h,
#     irq_hw_conf.h, irq_hw_init_generated.{c,h},
#     irq_periph_dispatch_generated.c,
#     irq_periph_handlers_generated.h,
#     irq_periph_vectors_generated.inc,
#     irq_table_generated.c   (gitignored outputs)
#
# Top-level targets (per devboard):
#   make dev-stm32f411              Full flow: gen + config + clean + build
#   make dev-stm32f411-gen          Regenerate board + IRQ files only
#   make dev-stm32f411-clean        Remove generated outputs and build/
#   make dev-stm32f411-flash        Flash build/stm32f411.elf via OpenOCD
#
#   make dev-stm32h723              (same set for the H723 devboard)
#
# Higher-level aliases that route to the above:
#   make os TARGET=stm32f411  →  make dev-stm32f411
#   make os TARGET=stm32h723  →  make dev-stm32h723

EXAMPLES_DIR        := examples
F411_DIR            := $(EXAMPLES_DIR)/stm32f411
H723_DIR            := $(EXAMPLES_DIR)/stm32h723
U575_DIR            := $(EXAMPLES_DIR)/stm32u575
CPPCHECK_BOARD_DIR  := build/cppcheck-board

# Per-board generated files — listed here so the corresponding -clean
# target can remove them deterministically and CI can list them.
define GENERATED_BOARD_FILES
$(1)/board/irq_hw_conf.h                       \
$(1)/board/irq_hw_init_generated.c             \
$(1)/board/irq_hw_init_generated.h             \
$(1)/board/irq_periph_dispatch_generated.c     \
$(1)/board/irq_periph_handlers_generated.h     \
$(1)/board/irq_periph_vectors_generated.inc    \
$(1)/board/irq_table_generated.c               \
$(1)/board/board_config.c                      \
$(1)/board/board_config.h                      \
$(1)/board/board_device_ids.h                  \
$(1)/board/board_handles.h
endef


# ── STM32F411 devboard ─────────────────────────────────────────────────────

.PHONY: dev-stm32f411-gen
dev-stm32f411-gen:
	@echo "### [dev-stm32f411] Generating IRQ headers from $(F411_DIR)/board/irq_table.xml ..."
	@python3 scripts/gen_irq_table.py \
		$(F411_DIR)/board/irq_table.xml \
		--outdir $(F411_DIR)/board
	@echo "### [dev-stm32f411] Generating board BSP from $(F411_DIR)/board/stm32f411_devboard.xml ..."
	@python3 scripts/gen_board_config.py \
		$(F411_DIR)/board/stm32f411_devboard.xml \
		--outdir $(F411_DIR)/board
	@echo "### [dev-stm32f411] Board generation done — outputs in $(F411_DIR)/board/"

.PHONY: dev-stm32f411
dev-stm32f411: dev-stm32f411-gen
	@echo "### [dev-stm32f411] Activating Kconfig from $(F411_DIR)/kconfig.conf ..."
	@cp $(F411_DIR)/kconfig.conf .config
	@$(MAKE) config-outputs
	@echo "### [dev-stm32f411] Cleaning stale build artifacts ..."
	@$(MAKE) clean
	@echo "### [dev-stm32f411] Building firmware → build/stm32f411.elf ..."
	@$(MAKE) all APP_DIR=$(F411_DIR) TARGET_NAME=stm32f411 CONFIG_BOARD=stm32f411_devboard
	@echo "### [dev-stm32f411] Build complete: build/stm32f411.elf"

.PHONY: dev-stm32f411-flash
dev-stm32f411-flash:
	@$(MAKE) flash TARGET_NAME=stm32f411

.PHONY: dev-stm32f411-clean
dev-stm32f411-clean:
	@echo "### [dev-stm32f411] Removing generated board files ..."
	@rm -f $(call GENERATED_BOARD_FILES,$(F411_DIR))
	@$(MAKE) clean
	@echo "### [dev-stm32f411] Clean complete"


# ── STM32H723 devboard (NUCLEO-H723ZG) ─────────────────────────────────────

.PHONY: dev-stm32h723-gen
dev-stm32h723-gen:
	@echo "### [dev-stm32h723] Generating IRQ headers from $(H723_DIR)/board/irq_table.xml ..."
	@python3 scripts/gen_irq_table.py \
		$(H723_DIR)/board/irq_table.xml \
		--outdir $(H723_DIR)/board
	@echo "### [dev-stm32h723] Generating board BSP from $(H723_DIR)/board/stm32h723_devboard.xml ..."
	@python3 scripts/gen_board_config.py \
		$(H723_DIR)/board/stm32h723_devboard.xml \
		--outdir $(H723_DIR)/board
	@echo "### [dev-stm32h723] Board generation done — outputs in $(H723_DIR)/board/"

.PHONY: dev-stm32h723
dev-stm32h723: dev-stm32h723-gen
	@echo "### [dev-stm32h723] Activating Kconfig from $(H723_DIR)/kconfig.conf ..."
	@cp $(H723_DIR)/kconfig.conf .config
	@$(MAKE) config-outputs
	@echo "### [dev-stm32h723] Cleaning stale build artifacts ..."
	@$(MAKE) clean
	@echo "### [dev-stm32h723] Building firmware → build/stm32h723.elf ..."
	@$(MAKE) all APP_DIR=$(H723_DIR) TARGET_NAME=stm32h723 CONFIG_BOARD=stm32h723_devboard
	@echo "### [dev-stm32h723] Build complete: build/stm32h723.elf"

.PHONY: dev-stm32h723-flash
dev-stm32h723-flash:
	@$(MAKE) flash TARGET_NAME=stm32h723

.PHONY: dev-stm32h723-clean
dev-stm32h723-clean:
	@echo "### [dev-stm32h723] Removing generated board files ..."
	@rm -f $(call GENERATED_BOARD_FILES,$(H723_DIR))
	@$(MAKE) clean
	@echo "### [dev-stm32h723] Clean complete"


# ── STM32U575 devboard (NUCLEO-U575ZI-Q, Cortex-M33 + TrustZone) ───────────

.PHONY: dev-stm32u575-gen
dev-stm32u575-gen:
	@echo "### [dev-stm32u575] Generating IRQ headers from $(U575_DIR)/board/irq_table.xml ..."
	@python3 scripts/gen_irq_table.py \
		$(U575_DIR)/board/irq_table.xml \
		--outdir $(U575_DIR)/board
	@echo "### [dev-stm32u575] Generating board BSP from $(U575_DIR)/board/stm32u575_devboard.xml ..."
	@python3 scripts/gen_board_config.py \
		$(U575_DIR)/board/stm32u575_devboard.xml \
		--outdir $(U575_DIR)/board
	@echo "### [dev-stm32u575] Board generation done — outputs in $(U575_DIR)/board/"

.PHONY: dev-stm32u575
dev-stm32u575: dev-stm32u575-gen
	@echo "### [dev-stm32u575] Activating Kconfig from $(U575_DIR)/kconfig.conf ..."
	@cp $(U575_DIR)/kconfig.conf .config
	@$(MAKE) config-outputs
	@echo "### [dev-stm32u575] Cleaning stale build artifacts ..."
	@$(MAKE) clean
	@echo "### [dev-stm32u575] Building firmware → build/stm32u575.elf ..."
	@$(MAKE) all APP_DIR=$(U575_DIR) TARGET_NAME=stm32u575 CONFIG_BOARD=stm32u575_devboard
	@echo "### [dev-stm32u575] Build complete: build/stm32u575.elf"

.PHONY: dev-stm32u575-flash
dev-stm32u575-flash:
	@$(MAKE) flash TARGET_NAME=stm32u575

.PHONY: dev-stm32u575-clean
dev-stm32u575-clean:
	@echo "### [dev-stm32u575] Removing generated board files ..."
	@rm -f $(call GENERATED_BOARD_FILES,$(U575_DIR))
	@$(MAKE) clean
	@echo "### [dev-stm32u575] Clean complete"


# ── Static-analysis helper ─────────────────────────────────────────────────
# cppcheck-board-gen ensures the F411 example board headers are present
# (the static-analysis run includes examples/stm32f411/board/ in its
# include path).  Run before `make cppcheck` or `bash scripts/run_cppcheck.sh`.
.PHONY: cppcheck-board-gen
cppcheck-board-gen: dev-stm32f411-gen
	@echo "### [cppcheck] F411 example board headers ready in $(F411_DIR)/board/"

.PHONY: clean-cppcheck-board
clean-cppcheck-board:
	@rm -rf $(CPPCHECK_BOARD_DIR)
	@echo "### $(CPPCHECK_BOARD_DIR) removed"
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

	@$(CPP) $(TARGET_SYSMBOL_DEF) $(SYMBOL_DEF) $(CC_LINKER_FLAGS) $(CC_TARGET_PROP) -T"$(LINKER_SCRIPT)" -o $@ $(OBJS)

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
DOC_DIR  ?= docs/doxygen

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
# Simplified build interface — aliases over the dev-* targets above
# ──────────────────────────────────────────────────────────────────
# TARGET selects the MCU variant.  Valid values:
#   stm32f411   STM32F411xE   (Cortex-M4)  devboard
#   stm32h723   STM32H723ZGTx (Cortex-M7)  devboard
#
# Standalone OS  (devboard example — no sibling app/ required):
#   make os  TARGET=stm32f411   →  make dev-stm32f411
#   make os  TARGET=stm32h723   →  make dev-stm32h723
#
# Application firmware (config from sibling app/ directory):
#   make app TARGET=stm32h723
#
# Flash after a successful build:
#   make os-flash  TARGET=stm32f411
#   make app-flash TARGET=stm32h723

TARGET ?= stm32f411

ifeq ($(TARGET),stm32f411)
  _OS_DEV_TARGET := dev-stm32f411
  _CONFIG_BOARD  := stm32f411_devboard
  _APP_KCONF     := ../app/kconfig_f411.conf
  _APP_ELF       := app-stm32f411
else ifeq ($(TARGET),stm32h723)
  _OS_DEV_TARGET := dev-stm32h723
  _CONFIG_BOARD  := stm32h723_devboard
  _APP_KCONF     := ../app/kconfig_ecg_h723.conf
  _APP_ELF       := ecg_h723
else ifeq ($(TARGET),stm32u575)
  _OS_DEV_TARGET := dev-stm32u575
  _CONFIG_BOARD  := stm32u575_devboard
  _APP_KCONF     := ../app/kconfig_u575.conf
  _APP_ELF       := app-stm32u575
else
  $(error TARGET='$(TARGET)' is not valid. Valid targets: stm32f411  stm32h723  stm32u575)
endif

.PHONY: os os-flash app app-gen app-flash

# os — alias for the per-devboard dev-* target.
os:
	@$(MAKE) $(_OS_DEV_TARGET)

os-flash:
	@$(MAKE) flash TARGET_NAME=$(TARGET)

# app-gen — regenerate board BSP files from the sibling app/ board XML.
app-gen:
	@echo "### [app/$(TARGET)] Regenerating board BSP from ../app/board/$(_CONFIG_BOARD).xml ..."
	@python3 scripts/gen_board_config.py \
		../app/board/$(_CONFIG_BOARD).xml \
		--outdir ../app/board
	@echo "### [app/$(TARGET)] Board BSP done"

# app — FreeRTOS-OS + application, config sourced from sibling app/
app: app-gen
	@if [ ! -f "$(_APP_KCONF)" ]; then \
		echo "### Error: $(_APP_KCONF) not found."; \
		echo "###        Create a Kconfig preset for TARGET=$(TARGET) under app/."; \
		exit 1; \
	fi
	@echo "### [app/$(TARGET)] Kconfig: $(_APP_KCONF)"
	@cp $(_APP_KCONF) .config
	@$(MAKE) config-outputs
	@$(MAKE) clean
	@$(MAKE) all APP_DIR=../app TARGET_NAME=$(_APP_ELF) CONFIG_BOARD=$(_CONFIG_BOARD)
	@echo "### [app/$(TARGET)] Done: build/$(_APP_ELF).elf"

app-flash:
	@$(MAKE) flash TARGET_NAME=$(_APP_ELF)
##############################################################


##############################################################
.PHONY: print-interface print-target flash docs clean-docs \
        dev-stm32f411 dev-stm32f411-gen dev-stm32f411-clean dev-stm32f411-flash \
        dev-stm32h723 dev-stm32h723-gen dev-stm32h723-clean dev-stm32h723-flash \
        dev-stm32u575 dev-stm32u575-gen dev-stm32u575-clean dev-stm32u575-flash \
        cppcheck-board-gen clean-cppcheck-board

print-interface:
	@echo $(OPENOCD_INTERFACE)

print-target:
	@echo $(OPENOCD_TARGET)
##############################################################