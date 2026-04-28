# demo/Makefile — App build fragment for the FreeRTOS-OS demo build.
#
# Included by FreeRTOS-OS/Makefile when APP_DIR=demo.
# Lists application object files and extra include paths.
#
# Generate board files first:
#   make demo-gen              (from FreeRTOS-OS/)
#
# Build demo firmware:
#   make demo                  (full: gen + config + compile)
#   make all APP_DIR=demo TARGET_NAME=demo   (manual, after demo-gen)

# ── Application sources ───────────────────────────────────────────────────

app-obj-y += app_main.o

# Generated board files (run 'make demo-gen' to create these)
app-obj-y += board/board_config.o
app-obj-y += board/irq_table_generated.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

# ── Include paths ─────────────────────────────────────────────────────────

# Expose demo root so <board/...> and <os_conf_include/...> resolve
APP_INCLUDES += -I$(APP_DIR)
# OS configuration headers (conf_board.h, def_compiler.h)
APP_INCLUDES += -I$(APP_DIR)/os_conf_include
