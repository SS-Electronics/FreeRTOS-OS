# File:        drivers/Makefile
# Author:      Subhajit Roy
#              subhajitroy005@gmail.com
#
# Module:      drivers
# Info:        Generic driver layer + vendor HAL backends
#              Management service threads live in services/Makefile
#
# This file is part of FreeRTOS-OS Project.

include autoconf.mk

# ── Include paths ──────────────────────────────────────────────────────────
# Project root allows <drivers/com/hal/stm32/hal_uart_stm32.h> to resolve
INCLUDES += -I$(CURDIR)

# ── Generic driver layer ───────────────────────────────────────────────────
obj-y += drivers/drv_uart.o
obj-y += drivers/drv_iic.o
obj-y += drivers/drv_spi.o
obj-y += drivers/drv_gpio.o
obj-y += drivers/drv_time.o
obj-y += drivers/drv_cpu.o

# ── Vendor HAL backend — STM32 ─────────────────────────────────────────────
ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")
obj-y += drivers/hal/stm32/hal_uart_stm32.o
obj-y += drivers/hal/stm32/hal_iic_stm32.o
obj-y += drivers/hal/stm32/hal_spi_stm32.o
obj-y += drivers/hal/stm32/hal_gpio_stm32.o
endif

ifeq ($(CONFIG_TARGET_MCU),"STM32F401xE")
obj-y += drivers/hal/stm32/hal_uart_stm32.o
obj-y += drivers/hal/stm32/hal_iic_stm32.o
obj-y += drivers/hal/stm32/hal_spi_stm32.o
obj-y += drivers/hal/stm32/hal_gpio_stm32.o
endif

# ── Vendor HAL backend — Infineon ──────────────────────────────────────────
# Uncomment when porting to an Infineon XMC target:
# obj-y += drivers/hal/infineon/hal_uart_infineon.o
# obj-y += drivers/hal/infineon/hal_iic_infineon.o
