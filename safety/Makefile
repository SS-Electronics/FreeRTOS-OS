# File:        safety/Makefile
# Author:      Subhajit Roy
#
# Module:      safety
# Info:        Medical-device safety services — IWDG watchdog, safe-state manager
#
# This file is part of FreeRTOS-OS Project.

include autoconf.mk

# ── Safe-state manager (always built — used by fault handler and hooks) ────
obj-y += safety/safe_state.o

# ── Hardware + software watchdog service (Kconfig-gated) ──────────────────
ifeq ($(CONFIG_INC_SERVICE_WDOG),1)
obj-y += safety/wdog.o
endif
