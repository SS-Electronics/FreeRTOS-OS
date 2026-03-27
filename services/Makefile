# File:        services/Makefile
# Author:      Subhajit Roy
#              subhajitroy005@gmail.com
#
# Module:      services
# Info:        OS service threads (management daemons for each driver class)
#
# This file is part of FreeRTOS-OS Project.

include autoconf.mk

# ── Include paths ──────────────────────────────────────────────────────────
INCLUDES += -I$(CURDIR)/include/services

# ── Kernel / core services ─────────────────────────────────────────────────
obj-y += services/kernel_service_core.o
obj-y += services/kernel_service_task_manager.o
obj-y += services/kernel_syscall.o

# ── Driver management service threads ──────────────────────────────────────

ifeq ($(CONFIG_INC_SERVICE_UART_MGMT),1)
obj-y += services/uart_mgmt.o
endif

ifeq ($(CONFIG_INC_SERVICE_IIC_MGMT),1)
obj-y += services/iic_mgmt.o
endif

ifneq ($(CONFIG_MCU_NO_OF_SPI_PERIPHERAL),0)
obj-y += services/spi_mgmt.o
endif

obj-y += services/gpio_mgmt.o
