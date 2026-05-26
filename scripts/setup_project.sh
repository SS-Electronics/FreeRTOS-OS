#!/usr/bin/env bash
# scripts/setup_project.sh — scaffold a FreeRTOS-OS application project at ../app
#
# Usage (called via Makefile):
#   make setup-project                     # blank starter project
#   make setup-project EXAMPLE=stm32h723  # scaffold from examples/stm32h723
#   make setup-project EXAMPLE=stm32f411
#   make setup-project EXAMPLE=stm32u575
#
# What gets created:
#   ../app/                     — application root
#     app_main.c                — entry point (blank with guidelines, or from example)
#     Makefile                  — app build fragment (app-obj-y / APP_INCLUDES)
#     kconfig.conf              — Kconfig preset (blank stubs, or copied from example)
#     board/                    — board descriptor + generated BSP files
#     os_conf_include/          — conf_board.h, def_compiler.h
#   ../README.md                — project README

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OS_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PARENT_DIR="$(dirname "${OS_ROOT}")"
APP_DIR="${PARENT_DIR}/app"
README_PATH="${PARENT_DIR}/README.md"
EXAMPLES_DIR="${OS_ROOT}/examples"
EXAMPLE="${1:-}"

# ── Colour helpers ─────────────────────────────────────────────────────────────
RED='\033[0;31]'; GREEN='\033[0;32]'; YELLOW='\033[1;33]'; CYAN='\033[0;36]'; NC='\033[0m'
info()    { echo -e "${CYAN}###${NC} $*"; }
success() { echo -e "${GREEN}>>>${NC} $*"; }
warn()    { echo -e "${YELLOW}!!!${NC} $*"; }
die()     { echo -e "${RED}ERR${NC} $*" >&2; exit 1; }

# ── Validate EXAMPLE argument ─────────────────────────────────────────────────
VALID_EXAMPLES=()
while IFS= read -r -d '' dir; do
    VALID_EXAMPLES+=("$(basename "${dir}")")
done < <(find "${EXAMPLES_DIR}" -mindepth 1 -maxdepth 1 -type d -print0 | sort -z)

if [[ -n "${EXAMPLE}" ]]; then
    EXAMPLE_PATH="${EXAMPLES_DIR}/${EXAMPLE}"
    if [[ ! -d "${EXAMPLE_PATH}" ]]; then
        die "Example '${EXAMPLE}' not found.  Available: ${VALID_EXAMPLES[*]}"
    fi
fi

# ── Guard: don't silently overwrite an existing project ───────────────────────
# Non-interactive override: make setup-project FORCE=1
FORCE="${FORCE:-0}"

if [[ -d "${APP_DIR}" ]]; then
    warn "App directory already exists: ${APP_DIR}"
    if [[ "${FORCE}" == "1" ]]; then
        warn "FORCE=1 — overwriting."
    elif [[ -w /dev/tty ]]; then
        printf "    Overwrite? [y/N] " > /dev/tty
        read -r answer < /dev/tty
        [[ "${answer}" =~ ^[Yy]$ ]] || { info "Aborted."; exit 0; }
    else
        die "App directory exists and terminal not available.\n    Remove it manually or re-run with FORCE=1:\n      make setup-project FORCE=1"
    fi
    rm -rf "${APP_DIR}"
fi

# ── Create base directory skeleton ────────────────────────────────────────────
info "Creating project skeleton at ${APP_DIR} ..."
mkdir -p "${APP_DIR}/board"
mkdir -p "${APP_DIR}/os_conf_include"

# ──────────────────────────────────────────────────────────────────────────────
# EXAMPLE mode — copy source files from examples/<name>/
# Generated board files are included so the project builds immediately;
# re-run 'make board-gen APP_DIR=../app' after modifying the board XML.
# ──────────────────────────────────────────────────────────────────────────────
if [[ -n "${EXAMPLE}" ]]; then
    info "Scaffolding from example '${EXAMPLE}' ..."

    # Copy the whole example tree
    cp -r "${EXAMPLE_PATH}/." "${APP_DIR}/"

    # Derive metadata for README
    BOARD_XML=$(find "${APP_DIR}/board" -name "*_devboard.xml" -maxdepth 1 2>/dev/null | head -1)
    BOARD_NAME=$(basename "${BOARD_XML:-_devboard.xml}" _devboard.xml)
    MCU=$(grep 'CONFIG_TARGET_MCU' "${APP_DIR}/kconfig.conf" 2>/dev/null | head -1 \
          | sed 's/.*="\(.*\)"/\1/' || echo "unknown")
    CPU=$(grep 'CONFIG_CPU_ARCH' "${APP_DIR}/kconfig.conf" 2>/dev/null | head -1 \
          | sed 's/.*="\(.*\)"/\1/' || echo "ARM-CM?")
    TARGET="${EXAMPLE}"

    success "Example '${EXAMPLE}' copied to ${APP_DIR}"

# ──────────────────────────────────────────────────────────────────────────────
# BLANK mode — create a minimal starter project skeleton
# ──────────────────────────────────────────────────────────────────────────────
else
    BOARD_NAME="<your_board>"
    MCU="<TARGET_MCU>"
    CPU="<CPU_ARCH>"
    TARGET="app"

    # ── kconfig.conf (blank stubs) ────────────────────────────────────────────
    cat > "${APP_DIR}/kconfig.conf" << 'KCONF'
# kconfig.conf — Kconfig preset for your FreeRTOS-OS application.
#
# Copy a complete preset from FreeRTOS-OS/examples/<board>/kconfig.conf
# and adapt it, or fill in the values below manually.
#
# Activate (from FreeRTOS-OS/):
#   cp ../app/kconfig.conf .config && make config-outputs

# ── Target MCU (REQUIRED — choose one) ───────────────────────────────────────
# CONFIG_TARGET_MCU="STM32F411xE"    CONFIG_CPU_ARCH="ARM-CM4"
# CONFIG_TARGET_MCU="STM32H723xx"    CONFIG_CPU_ARCH="ARM-CM7"
# CONFIG_TARGET_MCU="STM32U575xx"    CONFIG_CPU_ARCH="ARM-CM33"

# ── Clock values ──────────────────────────────────────────────────────────────
# CONFIG_HSE_VALUE=25000000
# CONFIG_HSI_VALUE=16000000
# CONFIG_LSI_VALUE=32000
# CONFIG_LSE_VALUE=32768
# CONFIG_HSE_STARTUP_TIMEOUT=100
# CONFIG_LSE_STARTUP_TIMEOUT=5000

# ── Core HAL modules (always required) ───────────────────────────────────────
# CONFIG_HAL_CORTEX_MODULE_ENABLED=y
# CONFIG_HAL_PWR_MODULE_ENABLED=y
# CONFIG_HAL_RCC_MODULE_ENABLED=y
# CONFIG_HAL_GPIO_MODULE_ENABLED=y
# CONFIG_HAL_FLASH_MODULE_ENABLED=y
# CONFIG_HAL_DMA_MODULE_ENABLED=y
# CONFIG_HAL_TIM_MODULE_ENABLED=y

# ── Communication peripherals ─────────────────────────────────────────────────
# CONFIG_HAL_UART_MODULE_ENABLED=y
# CONFIG_HAL_I2C_MODULE_ENABLED=y
# CONFIG_HAL_SPI_MODULE_ENABLED=y

# ── Debug ─────────────────────────────────────────────────────────────────────
# CONFIG_DEFAULT_DEBUG_EN=y
# CONFIG_DRV_DEBUG_EN=y

# ── OS services ───────────────────────────────────────────────────────────────
# CONFIG_INC_SERVICE_UART_MGMT=y
# CONFIG_INC_SERVICE_OS_SHELL_MGMT=y

# ── FreeRTOS kernel ───────────────────────────────────────────────────────────
# CONFIG_RTOS_TICK_RATE_HZ=1000
# CONFIG_RTOS_TOTAL_HEAP_SIZE=65536
# CONFIG_RTOS_MAX_PRIORITIES=56
# CONFIG_RTOS_MINIMAL_STACK_SIZE=512
# CONFIG_RTOS_USE_PREEMPTION=y
# CONFIG_RTOS_USE_MUTEXES=y
# CONFIG_RTOS_USE_TIMERS=y
KCONF

    # ── app Makefile fragment (blank) ─────────────────────────────────────────
    cat > "${APP_DIR}/Makefile" << 'MK'
# app/Makefile — application build fragment.
#
# Included by FreeRTOS-OS/Makefile when APP_DIR=../app.
#
# Build (from FreeRTOS-OS/):
#   make all APP_DIR=../app TARGET_NAME=app CONFIG_BOARD=<board_name>
#
# Flash:
#   make flash TARGET_NAME=app

# ── Application sources ───────────────────────────────────────────────────────
app-obj-y += app_main.o

# Generated board support files — created by:
#   make board-gen APP_DIR=../app   (inside FreeRTOS-OS/)
# Run this once after adding your board XML to board/.
app-obj-y += board/board_config.o
app-obj-y += board/irq_table_generated.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

# ── Add your own source files here ────────────────────────────────────────────
# app-obj-y += my_module.o
# app-obj-y += subdir/another_module.o

# ── Include paths ─────────────────────────────────────────────────────────────
APP_INCLUDES += -I$(APP_DIR)
APP_INCLUDES += -I$(APP_DIR)/os_conf_include
MK

    # ── os_conf_include headers ───────────────────────────────────────────────
    cat > "${APP_DIR}/os_conf_include/conf_board.h" << 'HDR'
#ifndef APP_CONF_BOARD_H_
#define APP_CONF_BOARD_H_

#include <def_hw.h>
#include <def_std.h>

/*
 * COMM_PRINTK_HW_ID selects which hardware UART printk() writes to.
 * Match this to the UART connected to your host (ST-Link VCP / USB-UART).
 *
 * Common choices:
 *   HW_ID_UART_1  — USART1
 *   HW_ID_UART_2  — USART2  (ST-Link VCP on most Nucleo boards)
 *   HW_ID_UART_3  — USART3
 */
#define COMM_PRINTK_HW_ID    HW_ID_UART_2

#endif /* APP_CONF_BOARD_H_ */
HDR

    cat > "${APP_DIR}/os_conf_include/def_compiler.h" << 'HDR'
#ifndef APP_DEF_COMPILER_H_
#define APP_DEF_COMPILER_H_

#include <stdint.h>

#endif /* APP_DEF_COMPILER_H_ */
HDR

    # ── app_main.c with developer guidelines ─────────────────────────────────
    cat > "${APP_DIR}/app_main.c" << 'BLANK_MAIN'
/*
 * app_main.c — FreeRTOS-OS application entry point.
 *
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  GETTING STARTED                                                         ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║                                                                          ║
 * ║  1. Scaffold from a known board (recommended):                           ║
 * ║       cd FreeRTOS-OS                                                     ║
 * ║       make setup-project EXAMPLE=stm32h723                               ║
 * ║     — copies board XML, kconfig, and a working app_main from the example ║
 * ║                                                                          ║
 * ║     Available examples: stm32f411 | stm32h723 | stm32u575               ║
 * ║                                                                          ║
 * ║  2. If you prefer to configure manually:                                 ║
 * ║       a. Copy a board XML from FreeRTOS-OS/examples/<board>/board/       ║
 * ║          into this project's board/ directory.                           ║
 * ║       b. Fill in kconfig.conf (see examples/<board>/kconfig.conf).       ║
 * ║       c. cd FreeRTOS-OS                                                  ║
 * ║          cp ../app/kconfig.conf .config && make config-outputs           ║
 * ║          make board-gen APP_DIR=../app                                   ║
 * ║          make all APP_DIR=../app TARGET_NAME=app CONFIG_BOARD=<board>    ║
 * ║          make flash TARGET_NAME=app                                      ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  CREATING TASKS                                                          │
 * │                                                                          │
 * │  app_main() is called by the kernel after OS init.                       │
 * │  Do NOT call vTaskStartScheduler() — the kernel does that.               │
 * │                                                                          │
 * │    int app_main(void)                                                    │
 * │    {                                                                     │
 * │        os_thread_create(my_task, "my_task", 256, 1, NULL);              │
 * │        return 0;                                                         │
 * │    }                                                                     │
 * │                                                                          │
 * │  Task function signature:                                                │
 * │    static void my_task(void *param) { for (;;) { ... } }                │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  PRINTING                                                                │
 * │                                                                          │
 * │  printk() is thread-safe and routes to COMM_PRINTK_HW_ID                │
 * │  (set in os_conf_include/conf_board.h).                                  │
 * │                                                                          │
 * │    printk("[app] hello from task %s\n", pcTaskGetName(NULL));            │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  DELAYS                                                                  │
 * │                                                                          │
 * │    os_thread_delay(500);   // 500 ms (uses FreeRTOS tick)                │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  GPIO                                                                    │
 * │                                                                          │
 * │    #include <services/gpio_mgmt.h>                                       │
 * │    #include <board/board_device_ids.h>   // LED_BOARD etc. (generated)  │
 * │                                                                          │
 * │    gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);               │
 * │    gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_SET,    0, 0);  // high      │
 * │    gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_CLEAR,  0, 0);  // low       │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  IRQ CALLBACKS                                                           │
 * │                                                                          │
 * │    #include <irq/irq_desc.h>                                             │
 * │                                                                          │
 * │    // Callback signature:                                                │
 * │    static void my_rx_cb(irq_id_t id, void *data,                        │
 * │                         void *arg, BaseType_t *pxHPT) { ... }           │
 * │                                                                          │
 * │    irq_register(IRQ_ID_UART_RX(UART_DEBUG), my_rx_cb, NULL);            │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  UART TX                                                                 │
 * │                                                                          │
 * │    #include <services/uart_mgmt.h>                                       │
 * │    uint8_t buf[] = "hello\r\n";                                          │
 * │    uart_mgmt_post(UART_DEBUG, UART_MGMT_CMD_TX, buf, sizeof(buf));      │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  BUILD TARGET NAMES                                                      │
 * │                                                                          │
 * │  The CONFIG_BOARD value must match the XML filename (without .xml):      │
 * │    board/stm32h723_devboard.xml  →  CONFIG_BOARD=stm32h723_devboard     │
 * └──────────────────────────────────────────────────────────────────────────┘
 */

#include <os/kernel.h>
#include <os/kernel_syscall.h>

/* ── Implement your application tasks below ──────────────────────────────── */

static void app_task(void *param)
{
    (void)param;
    printk("[app] FreeRTOS-OS running\n");
    for (;;)
    {
        os_thread_delay(1000);
    }
}

/* ── Entry point — called by init/main.c after OS init ───────────────────── */

int app_main(void)
{
    os_thread_create(app_task, "app_task", 256, 1, NULL);
    return 0;
}
BLANK_MAIN

    success "Blank project skeleton created."
fi

# ──────────────────────────────────────────────────────────────────────────────
# Write ../README.md
# ──────────────────────────────────────────────────────────────────────────────
info "Writing ${README_PATH} ..."

if [[ -n "${EXAMPLE}" ]]; then
    BOARD_XML_REAL=$(find "${APP_DIR}/board" -name "*_devboard.xml" -maxdepth 1 2>/dev/null | head -1)
    BOARD_NAME_REAL=$(basename "${BOARD_XML_REAL:-x_devboard.xml}" _devboard.xml)
    DEV_TARGET="dev-${EXAMPLE}"

    cat > "${README_PATH}" << README
# App — FreeRTOS-OS Application (${EXAMPLE})

Scaffolded from the **${EXAMPLE}** example.
MCU: \`${MCU}\`  |  CPU: \`${CPU}\`  |  Board config: \`app/board/${BOARD_NAME_REAL}_devboard.xml\`

---

## Quick start

\`\`\`bash
cd FreeRTOS-OS

# Generate board BSP + activate Kconfig + compile
make ${DEV_TARGET} APP_DIR=../app

# Flash to target
make ${DEV_TARGET}-flash
\`\`\`

> **Tip:** pass \`DEBUG=0\` for a release build (-Os -DNDEBUG):
> \`\`\`bash
> make ${DEV_TARGET} APP_DIR=../app DEBUG=0
> \`\`\`

---

## Directory layout

\`\`\`
.
├── app/                               Application project (this scaffold)
│   ├── app_main.c                     Entry point — implement your tasks here
│   ├── Makefile                       Build fragment (app-obj-y, APP_INCLUDES)
│   ├── kconfig.conf                   Kconfig preset
│   ├── board/
│   │   ├── ${BOARD_NAME_REAL}_devboard.xml   Board peripheral descriptor (edit this)
│   │   ├── irq_table.xml              IRQ routing table
│   │   ├── mcu_config.h               MCU peripheral counts / UART_x_EN flags
│   │   └── <generated files>          board_config.{c,h}, board_device_ids.h, …
│   └── os_conf_include/
│       ├── conf_board.h               COMM_PRINTK_HW_ID selection
│       └── def_compiler.h             Compiler / type includes
└── FreeRTOS-OS/                       RTOS kernel (this repo)
\`\`\`

---

## Build reference

| Command | Description |
|---------|-------------|
| \`make ${DEV_TARGET} APP_DIR=../app\` | Full build: gen → config → compile |
| \`make ${DEV_TARGET} APP_DIR=../app DEBUG=0\` | Release build (-Os -DNDEBUG) |
| \`make ${DEV_TARGET}-flash\` | Flash via OpenOCD / ST-Link |
| \`make ${DEV_TARGET}-gen\` | Regenerate board BSP files only |
| \`make ${DEV_TARGET}-clean\` | Remove generated files + build/ |
| \`make board-gen APP_DIR=../app\` | Regenerate BSP from board XML |

---

## Customising

### Add application source files

Edit \`app/Makefile\`:
\`\`\`makefile
app-obj-y += my_module.o
app-obj-y += subdir/another.o
\`\`\`
Add the corresponding \`.c\` files under \`app/\`.

### Change board peripherals

1. Edit \`app/board/${BOARD_NAME_REAL}_devboard.xml\`.
2. Edit \`app/board/irq_table.xml\` if IRQ routing changes.
3. Regenerate:
   \`\`\`bash
   cd FreeRTOS-OS
   make ${DEV_TARGET}-gen APP_DIR=../app
   \`\`\`

### Change Kconfig (HAL modules, heap size, …)

1. Edit \`app/kconfig.conf\`.
2. Activate:
   \`\`\`bash
   cd FreeRTOS-OS
   cp ../app/kconfig.conf .config && make config-outputs
   \`\`\`
3. Rebuild: \`make ${DEV_TARGET} APP_DIR=../app\`

---

## Available examples

Re-scaffold to switch target board:

\`\`\`bash
cd FreeRTOS-OS
make setup-project EXAMPLE=stm32h723   # NUCLEO-H723ZG  (Cortex-M7, 1 MB flash)
make setup-project EXAMPLE=stm32f411   # STM32F411 devboard (Cortex-M4F, 512 KB)
make setup-project EXAMPLE=stm32u575   # NUCLEO-U575ZI-Q (Cortex-M33 + TrustZone)
\`\`\`
README

else
    # Blank README
    cat > "${README_PATH}" << README
# App — FreeRTOS-OS Application

Blank project scaffold.  Follow the steps below to configure and build.

---

## Setup

### Option A — Scaffold from a known board (easiest)

\`\`\`bash
cd FreeRTOS-OS

# Pick a target board:
make setup-project EXAMPLE=stm32h723   # NUCLEO-H723ZG  (Cortex-M7)
make setup-project EXAMPLE=stm32f411   # STM32F411 devboard (Cortex-M4F)
make setup-project EXAMPLE=stm32u575   # NUCLEO-U575ZI-Q (Cortex-M33 + TZ)
\`\`\`

### Option B — Configure manually

\`\`\`bash
# 1. Copy a board descriptor and IRQ table from an example
cp FreeRTOS-OS/examples/stm32h723/board/stm32h723_devboard.xml  app/board/
cp FreeRTOS-OS/examples/stm32h723/board/irq_table.xml           app/board/
cp FreeRTOS-OS/examples/stm32h723/board/mcu_config.h            app/board/

# 2. Copy a Kconfig preset and adapt it
cp FreeRTOS-OS/examples/stm32h723/kconfig.conf  app/kconfig.conf

# 3. Activate Kconfig
cd FreeRTOS-OS
cp ../app/kconfig.conf .config && make config-outputs

# 4. Generate board BSP (board_config.c/h, IRQ tables, …)
make board-gen APP_DIR=../app

# 5. Build
make all APP_DIR=../app TARGET_NAME=app CONFIG_BOARD=stm32h723_devboard

# 6. Flash
make flash TARGET_NAME=app
\`\`\`

---

## Directory layout

\`\`\`
.
├── app/                          Application project
│   ├── app_main.c                Entry point — implement your tasks here
│   ├── Makefile                  Build fragment (app-obj-y, APP_INCLUDES)
│   ├── kconfig.conf              Kconfig preset — fill in CONFIG_TARGET_MCU etc.
│   ├── board/                    Add board XML here; generated BSP lands here too
│   └── os_conf_include/
│       ├── conf_board.h          COMM_PRINTK_HW_ID selection
│       └── def_compiler.h        Compiler / type includes
└── FreeRTOS-OS/                  RTOS kernel
\`\`\`

---

## app_main.c guide

See the inline comments in \`app/app_main.c\` for:

- Creating tasks with \`os_thread_create()\`
- \`printk()\` — thread-safe UART output
- \`os_thread_delay(ms)\` — FreeRTOS tick-based delay
- GPIO control via \`gpio_mgmt_post()\`
- IRQ callbacks with \`irq_register()\`
- UART TX with \`uart_mgmt_post()\`

---

## Build reference

| Command | Description |
|---------|-------------|
| \`make all APP_DIR=../app TARGET_NAME=app CONFIG_BOARD=<board>\` | Compile |
| \`make all … DEBUG=0\` | Release build (-Os -DNDEBUG) |
| \`make flash TARGET_NAME=app\` | Flash via OpenOCD / ST-Link |
| \`make board-gen APP_DIR=../app\` | Regenerate BSP from board XML |
| \`make clean\` | Remove build/ |
README
fi

# ── Final summary ─────────────────────────────────────────────────────────────
echo ""
success "Project created:  ${APP_DIR}"
success "README written:   ${README_PATH}"
echo ""

if [[ -n "${EXAMPLE}" ]]; then
    BOARD_XML_FINAL=$(find "${APP_DIR}/board" -name "*_devboard.xml" -maxdepth 1 2>/dev/null | head -1)
    BOARD_NAME_FINAL=$(basename "${BOARD_XML_FINAL:-x_devboard.xml}" _devboard.xml)
    info "Next steps:"
    echo "  cd FreeRTOS-OS"
    echo "  make dev-${EXAMPLE} APP_DIR=../app        # build"
    echo "  make dev-${EXAMPLE}-flash                 # flash"
    echo ""
    info "To modify board peripherals:"
    echo "  Edit app/board/${BOARD_NAME_FINAL}_devboard.xml"
    echo "  Then: make dev-${EXAMPLE}-gen APP_DIR=../app"
else
    info "Next steps:"
    echo ""
    echo "  Option A (recommended) — scaffold from a board example:"
    echo "    cd FreeRTOS-OS"
    echo "    make setup-project EXAMPLE=stm32h723"
    echo ""
    echo "  Option B — configure manually:"
    echo "    1. Copy board XML to app/board/"
    echo "    2. Fill in app/kconfig.conf"
    echo "    3. cd FreeRTOS-OS"
    echo "    4. cp ../app/kconfig.conf .config && make config-outputs"
    echo "    5. make board-gen APP_DIR=../app"
    echo "    6. make all APP_DIR=../app TARGET_NAME=app CONFIG_BOARD=<board>"
fi
echo ""
