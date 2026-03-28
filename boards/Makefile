# File:        boards/Makefile
# Author:      Subhajit Roy
#              subhajitroy005@gmail.com
#
# Module:      boards — Board Support Package
# Info:        Compiles the generated board_config.c for the selected board.
#              The source file is generated from boards/$(CONFIG_BOARD).xml
#              by running:  make board-gen
#
# This file is part of FreeRTOS-OS Project.

include autoconf.mk

# Default board — override on the command line or in .config
CONFIG_BOARD ?= stm32f411_devboard

# board_device_ids.h is generated into include/board/
INCLUDES += -I$(CURDIR)/include/board

# Compile the generated board_config.c
obj-y += boards/$(CONFIG_BOARD)/board_config.o
