# examples/stm32f411/Makefile — App build fragment for the F411 devboard example.
#
# Included by FreeRTOS-OS/Makefile when APP_DIR=examples/stm32f411.
# Lists application object files and extra include paths.
#
# Build the example firmware (full: gen + config + compile):
#   make dev-stm32f411              (from FreeRTOS-OS/)
#
# Or, equivalently, the higher-level alias:
#   make os TARGET=stm32f411

# ── Application sources ───────────────────────────────────────────────────

app-obj-y += app_main.o

# Generated board files (run 'make dev-stm32f411-gen' to create these)
app-obj-y += board/board_config.o
app-obj-y += board/irq_table_generated.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

# ── Include paths ─────────────────────────────────────────────────────────

# Expose example root so <board/...> and <os_conf_include/...> resolve
APP_INCLUDES += -I$(APP_DIR)
# OS configuration headers (conf_board.h, def_compiler.h)
APP_INCLUDES += -I$(APP_DIR)/os_conf_include
