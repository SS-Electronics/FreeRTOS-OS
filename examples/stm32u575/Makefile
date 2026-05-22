# examples/stm32u575/Makefile — App build fragment for the U575 example.
#
# Included by FreeRTOS-OS/Makefile when APP_DIR=examples/stm32u575.
# Lists application object files and extra include paths.
#
# Build the example firmware (full: gen + config + compile):
#   make dev-stm32u575              (from FreeRTOS-OS/)
#
# Or, equivalently, the higher-level alias:
#   make os TARGET=stm32u575

# ── Application sources ───────────────────────────────────────────────────

app-obj-y += app_main.o

# Trustcore — device identity / flash integrity / SAU stub. Strong override
# of the weak trustcore_init_secure_world() from startup_stm32u575zitxq.c.
app-obj-y += trustcore/trustcore.o

# Generated board files (run 'make dev-stm32u575-gen' to create these)
app-obj-y += board/board_config.o
app-obj-y += board/irq_table_generated.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

# ── Include paths ─────────────────────────────────────────────────────────

APP_INCLUDES += -I$(APP_DIR)
APP_INCLUDES += -I$(APP_DIR)/os_conf_include
APP_INCLUDES += -I$(APP_DIR)/trustcore
