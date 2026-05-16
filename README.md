# FreeRTOS-OS

A structured, Linux-inspired OS layer built on top of the FreeRTOS kernel for ARM Cortex-M embedded targets. Provides generic thread management, an intrusive linked-list, an IPC message-queue subsystem, a hardware abstraction layer driven by Kconfig, and a service-thread architecture — all sitting above FreeRTOS without replacing it.

| Target | Core | Clock | Flash | RAM | Status |
|---|---|---|---|---|---|
| STM32F411VET6 | Cortex-M4F | 100 MHz | 512 KB | 128 KB | Supported |
| **STM32H723ZGTx** | **Cortex-M7** | **64 MHz (HSI)** | **1 MB** | **128 KB DTCM** | **Confirmed running** |

---

## Documentation

| Document | Description |
|---|---|
| [docs/OS_THREAD.md](docs/OS_THREAD.md) | Thread API, linked list, `os_thread_create()` usage |
| [docs/QUEUE.md](docs/QUEUE.md) | IPC message queues, ring buffer, management service queues |
| [docs/DRIVERS.md](docs/DRIVERS.md) | 3-layer driver architecture, HAL ops vtables, vendor backends |
| [docs/BOARD.md](docs/BOARD.md) | Board XML schema, BSP code generator, adding boards/vendors |
| [docs/DEV_MGMT.md](docs/DEV_MGMT.md) | UART / I2C / SPI / GPIO management service threads |
| [docs/SHELL_CLI.md](docs/SHELL_CLI.md) | Interactive shell over UART, PuTTY setup, registering commands |
| [docs/IRQ.md](docs/IRQ.md) | Linux-style IRQ dispatch, irq_desc chain, request_irq, irq_register |
| [docs/DMA.md](docs/DMA.md) | Linux-style DMA engine, STM32F4 HAL backend, slave/memcpy/cyclic transfers |
| [docs/DEBUG.md](docs/DEBUG.md) | VSCode debug setup, OpenOCD, GDB, ITM/SWO |
| [docs/OS_INSIDE.md](docs/OS_INSIDE.md) | Internals deep-dive: ISR priority rules, post-mortems, debug recipes |
| [docs/CHECK.md](docs/CHECK.md) | CPPcheck + MISRA C:2012 static analysis: setup, options, deviation table |
| [docs/DSP.md](docs/DSP.md) | CMSIS-DSP integration: XML module selection, generator, PID controller API |

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                       User Application                          │
│   app_main() · os_shell_mgmt_start() · gpio_mgmt_post()        │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                      Service Layer                               │
│     UART Mgmt · I2C Mgmt · SPI Mgmt · GPIO Mgmt                │
│     OS Shell (FreeRTOS+CLI, ring-buffer I/O over UART_APP)      │
│     (FreeRTOS tasks; self-register from board config at boot)   │
└───────────┬──────────────────────────────┬───────────────────────┘
            │                              │
┌───────────▼──────────┐     ┌─────────────▼──────────────────────┐
│    OS Kernel API     │     │          IPC Subsystem             │
│  os_thread_create()  │     │   ipc_mqueue_register()            │
│  os_thread_delete()  │     │   ipc_mqueue_send_item()           │
│  os_suspend_thread() │     │   ipc_mqueue_receive_item()        │
│  os_resume_thread()  │     │   Ring-buffer (HW byte streams)    │
│  os_thread_delay()   │     │   xQueue (inter-task messages)     │
└───────────┬──────────┘     └─────────────┬──────────────────────┘
            │                              │
┌───────────▼──────────────────────────────▼───────────────────────┐
│                  Memory Management (mm)                          │
│   Intrusive doubly-circular linked list (Linux list_head style)  │
│   kmalloc() / kfree() — wrappers over pvPortMalloc              │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                      FreeRTOS Kernel                             │
│   Tasks · Queues · Mutexes · Semaphores · Timers · heap_4       │
│   Portable layer: GCC/ARM_CM4F                                   │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                 Board Configuration (BSP)                        │
│   app/board/*.xml → gen_board_config.py / gen_irq_table.py →   │
│   board_config.c · board_device_ids.h · irq_hw_init_generated.c │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│              Hardware Abstraction Layer (HAL)                    │
│   STM32F4xx HAL  or  STM32H7xx HAL — driven by Kconfig          │
│   CMSIS-Core · Startup (F4/H7) · Linker script (F4/H7)          │
│   H7: SCB_EnableICache/DCache in startup · TIM6 timebase         │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│            ARM CMSIS-DSP (optional, per-module)                  │
│   app/board/dsp_dev.xml → arm_dsp_gen.py →                      │
│   dsp_config.h · dsp_config.mk                                   │
│   PID · FFT · FIR/IIR · Statistics · Matrix · …                 │
└──────────────────────────────────────────────────────────────────┘
```

---

## Code Generation

Three independent generators produce C source from configuration files. Run them from the **project root** (`FreeRTOS-OS-App/`).

### 1 — Kconfig (`make menuconfig` + `make config-outputs`)

Controls which peripherals are compiled in, heap size, tick rate, task limits, etc.

```bash
make menuconfig       # interactive TUI — saves FreeRTOS-OS/.config
make config-outputs   # reads .config → writes the three files below
```

**Outputs written to `$(APP_DIR)/board/` (demo build) or `FreeRTOS-OS/config/` (standalone):**

| File | Contents |
|---|---|
| `autoconf.h` | `#define CONFIG_*` symbols for every enabled option |
| `autoconf.mk` | `CONFIG_*=1/0/value` for Makefile conditionals |
| `stm32f4xx_hal_conf.h` / `stm32h7xx_hal_conf.h` | STM32 HAL module enable/disable flags (family-specific) |

Re-run both commands whenever you change a Kconfig option (target MCU, heap size, UART count, tick rate, enabled services, etc.).

---

### 2 — Board BSP (`make board-gen`)

Reads the board XML descriptor and generates the peripheral table, device-ID constants, and MCU config header.

```bash
make board-gen
# Reads:   app/board/stm32f411_devboard.xml   (F4 target)
#       or demo/board/stm32h723_devboard.xml  (H723 demo)
# Writes:  <APP_DIR>/board/board_config.c
#          <APP_DIR>/board/board_device_ids.h
#          <APP_DIR>/board/board_handles.h
#          <APP_DIR>/board/mcu_config.h
```

Re-run whenever you edit the board XML (adding a UART, changing GPIO pin assignments, renaming a device ID, etc.).

To use a different board XML:
```bash
make board-gen CONFIG_BOARD=my_custom_board
```

---

### 3 — IRQ Table (`make irq_gen`)

Reads the IRQ XML and generates the software-IRQ-to-NVIC binding table and all peripheral vector stubs.

```bash
make irq_gen
# Reads:   app/board/irq_table.xml
# Writes:  FreeRTOS-OS/irq/irq_table.c            — software IRQ name table
#          app/board/irq_hw_init_generated.c        — NVIC priority / binding init
#          app/board/irq_hw_init_generated.h        — irq_hw_init_all() declaration
#          app/board/irq_periph_handlers_generated.h — ISR function declarations
#          app/board/irq_periph_vectors_generated.inc — vector table entries
```

Re-run whenever you edit `app/board/irq_table.xml` (adding a peripheral interrupt, changing an IRQ priority, adding an EXTI line, etc.).

> **Priority rule:** All peripheral IRQ priorities in `irq_table.xml` must be `>= configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY` (default `5`). Priorities 1–4 are above the FreeRTOS BASEPRI mask and must not call any FreeRTOS API. See [docs/OS_INSIDE.md](docs/OS_INSIDE.md).

---

### 4 — DSP Configuration (`arm_dsp_gen.py`)

Reads the DSP module selection XML and generates a C header and Makefile fragment that control which CMSIS-DSP modules are compiled.

```bash
python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml
# Reads:   app/board/dsp_dev.xml
# Writes:  app/board/dsp_config.h     — CONFIG_ARM_DSP_* #defines for firmware
#          app/board/dsp_config.mk    — CONFIG_ARM_DSP_* variables for lib/Makefile
```

Edit `app/board/dsp_dev.xml` to enable or disable individual DSP function groups (`basic_math`, `controller`, `filtering`, `transform`, etc.), then re-run the generator and rebuild. Only enabled modules are compiled; disabled modules produce zero code and zero flash cost.

See [docs/DSP.md](docs/DSP.md) for the full reference: XML schema, module dependency rules, flash budget, and PID controller usage.

---

### When to Re-run Each Generator

| Change made | Command(s) to re-run |
|---|---|
| Kconfig option (MCU, heap size, UART count…) | `make menuconfig` → `make config-outputs` |
| Board XML (pin, UART, peripheral assignment) | `make board-gen` |
| IRQ table XML (priority, new IRQ, new EXTI) | `make irq_gen` |
| DSP XML (enable/disable a DSP module) | `python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml` |
| Any of the above | `make menuconfig` → `make config-outputs` → `make board-gen` → `make irq_gen` → `arm_dsp_gen.py` |

After any generator run, rebuild with `make app`.

---

## Setting Up a New App Project

Use FreeRTOS-OS as a git submodule inside your own application repository. The OS build system is self-contained inside `FreeRTOS-OS/`; your project only needs a thin root `Makefile` and an `app/` directory.

---

### Repository Layout

```
my-app/
├── Makefile                               ← top-level entry point  (see example below)
├── FreeRTOS-OS/                           ← this repo, added as a git submodule
└── app/
    ├── Makefile                           ← app build fragment     (see example below)
    ├── app_main.c                         ← application entry point
    └── board/
        ├── stm32f411_devboard.xml         ← board descriptor       (BSP generator input)
        ├── irq_table.xml                  ← IRQ assignments        (IRQ generator input)
        │
        │   ── generated by make board-gen ──────────────────────────────────────────
        ├── board_config.c
        ├── board_device_ids.h
        ├── board_handles.h
        ├── mcu_config.h
        │
        │   ── generated by make irq_gen ────────────────────────────────────────────
        ├── irq_hw_init_generated.c
        ├── irq_hw_init_generated.h
        ├── irq_periph_dispatch_generated.c
        ├── irq_periph_handlers_generated.h
        └── irq_periph_vectors_generated.inc
```

All generated files are output of `make board-gen` or `make irq_gen`. Do not hand-edit them; re-run the generator instead. Commit the XML sources; optionally gitignore the generated files.

---

### Step 1 — Add FreeRTOS-OS as a Submodule

```bash
mkdir my-app && cd my-app
git init

# Add the OS as a submodule
git submodule add <freertos-os-repo-url> FreeRTOS-OS
git submodule update --init --recursive
```

When cloning an existing project that already has the submodule:

```bash
git clone --recurse-submodules <my-app-repo-url>
cd my-app
```

---

### Step 2 — Root Makefile

Create `Makefile` at the project root. This is the only build entry point — all `make` commands are run here.

```makefile
# Makefile — top-level build entry point
#
# Usage:
#   make menuconfig      Configure kernel (generates FreeRTOS-OS/.config)
#   make config-outputs  Generate autoconf headers from .config
#   make board-gen       Generate BSP from app/board/<board>.xml
#   make irq_gen         Generate NVIC init + vector stubs from irq_table.xml
#   make app             Build OS + app/ → FreeRTOS-OS/build/app.elf
#   make kernel          Build OS only   → FreeRTOS-OS/build/kernel.elf
#   make flash-app       Flash app.elf via OpenOCD
#   make flash-kernel    Flash kernel.elf via OpenOCD
#   make clean           Delete build artefacts

OS_DIR  := FreeRTOS-OS
APP_SRC := app

.PHONY: all kernel app flash-kernel flash-app clean
.PHONY: menuconfig config-outputs board-gen irq_gen

all: app

# ── Build ──────────────────────────────────────────────────────────────────────
kernel:
	$(MAKE) -C $(OS_DIR) all TARGET_NAME=kernel

app:
	$(MAKE) -C $(OS_DIR) all APP_DIR=../$(APP_SRC) TARGET_NAME=app

# ── Flash ──────────────────────────────────────────────────────────────────────
flash-kernel:
	$(MAKE) -C $(OS_DIR) flash TARGET_NAME=kernel

flash-app:
	$(MAKE) -C $(OS_DIR) flash APP_DIR=../$(APP_SRC) TARGET_NAME=app

# ── Code generation ────────────────────────────────────────────────────────────
menuconfig:
	$(MAKE) -C $(OS_DIR) menuconfig

config-outputs:
	$(MAKE) -C $(OS_DIR) config-outputs

board-gen:
	$(MAKE) -C $(OS_DIR) board-gen

irq_gen:
	python3 $(OS_DIR)/scripts/gen_irq_table.py $(APP_SRC)/board/irq_table.xml

# ── Housekeeping ───────────────────────────────────────────────────────────────
clean:
	$(MAKE) -C $(OS_DIR) clean
```

> **Never run `make` from inside `FreeRTOS-OS/` directly for the application build** — it lacks the `app/` objects. Always invoke from the project root.

---

### Step 3 — App Build Fragment (`app/Makefile`)

This file is included by the OS build system when `APP_DIR` is set. It declares which `.c` files to compile and which include paths to expose. All paths are relative to `APP_DIR`.

```makefile
# app/Makefile — application build fragment
#
# Included by FreeRTOS-OS/Makefile when APP_DIR=../app is passed.
# Add one .o entry per .c source file in your application.

# ── Application source files ───────────────────────────────────────────────────
app-obj-y += app_main.o

# ── Generated BSP / IRQ board files ───────────────────────────────────────────
app-obj-y += board/board_config.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

# ── Expose app headers to the rest of the build ────────────────────────────────
APP_INCLUDES += -I$(APP_DIR)
```

Add a new `app-obj-y` line for each `.c` file you add under `app/`. The OS Makefile links all `app-obj-y` objects together with the kernel objects.

---

### Step 4 — First-Time Workflow

Run these once when setting up the project, or whenever the corresponding input files change:

```bash
# ── 1. Install toolchain and tools (once per machine) ─────────────────────────
make -C FreeRTOS-OS install-prerequisites
# Installs: arm-none-eabi-gcc, OpenOCD, kconfig-frontends, debug tools

# ── 2. Configure kernel ────────────────────────────────────────────────────────
make menuconfig       # TUI: select target MCU, tick rate, heap size, services
make config-outputs   # writes config/autoconf.h + config/autoconf.mk

# ── 3. Generate BSP from board XML ────────────────────────────────────────────
make board-gen        # reads app/board/<board>.xml → board_config.c, device IDs

# ── 4. Generate IRQ table ─────────────────────────────────────────────────────
make irq_gen          # reads app/board/irq_table.xml → NVIC init + vector stubs

# ── 5. Build ──────────────────────────────────────────────────────────────────
make app              # OS + app/  →  FreeRTOS-OS/build/app.elf
make kernel           # OS only    →  FreeRTOS-OS/build/kernel.elf

# ── 6. Flash ──────────────────────────────────────────────────────────────────
make flash-app        # programs app.elf via OpenOCD, verifies, resets target

# ── 7. Connect to the interactive shell ───────────────────────────────────────
# H723 NUCLEO — STLink VCP (shell + printk share one port):
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0
# F411 — separate shell UART on USB-serial adapter:
# screen /dev/ttyUSB0 115200
# Banner appears ~1 s after reset on H723, ~5 s on F411

# ── 8. Clean ──────────────────────────────────────────────────────────────────
make clean
```

> After first-time setup, day-to-day development only needs `make app` and `make flash-app`.

---

## Quick Start — H723 Demo (NUCLEO-H723ZG)

All commands run from inside `FreeRTOS-OS/`:

```bash
# ── Build ─────────────────────────────────────────────────────────────────────
make all APP_DIR=demo TARGET_NAME=demo
# → build/demo.elf  (237 KB text)

# ── Flash ─────────────────────────────────────────────────────────────────────
make flash TARGET_NAME=demo
# → ** Verified OK ** → ** Resetting Target **

# ── Connect — open terminal BEFORE reset so the banner is not missed ──────────
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0
# After reset you should see (within 1 second):
#
#  [500] [heartbeat] tick 1
#
#   +--------------------------------------------+
#   |   ____              ___ _____ ___  ____    |
#   |  |  __| ___ ___ ___|  _|_   _/   \/ ___|  |
#   ...
#   | CPU Clock: 64  MHz
#   | Kernel   : FreeRTOS V11.1.0+
#   +--------------------------------------------+
#   Type 'help' for available commands.
#  OS >

# ── Clean ─────────────────────────────────────────────────────────────────────
make clean
```

## Quick Start — F411 App (existing repo)

```bash
# ── Clone ─────────────────────────────────────────────────────────────────────
git clone --recurse-submodules <repo-url>
cd FreeRTOS-OS-App

# ── Install prerequisites (first time only) ───────────────────────────────────
make -C FreeRTOS-OS install-prerequisites

# ── Configure kernel ──────────────────────────────────────────────────────────
make menuconfig       # choose MCU, tick rate, heap, enabled services
make config-outputs   # generates autoconf.h + autoconf.mk + stm32f4xx_hal_conf.h

# ── Generate board BSP ────────────────────────────────────────────────────────
make board-gen        # reads app/board/stm32f411_devboard.xml → BSP sources

# ── Generate IRQ table ────────────────────────────────────────────────────────
make irq_gen          # reads app/board/irq_table.xml → NVIC init + vector stubs

# ── Build ─────────────────────────────────────────────────────────────────────
make app              # OS + app/ → FreeRTOS-OS/build/app.elf
make kernel           # OS only   → FreeRTOS-OS/build/kernel.elf

# ── Flash ─────────────────────────────────────────────────────────────────────
make flash-app        # programs app.elf via OpenOCD, verifies, resets target

# ── Connect to shell ──────────────────────────────────────────────────────────
screen /dev/ttyUSB0 115200    # UART_APP (USART2, USB-to-serial)

# ── Clean ─────────────────────────────────────────────────────────────────────
make clean
```

> Steps 2–5 are one-time setup. Day-to-day development only needs `make app` and `make flash-app`.

---

## Make Target Reference

### Demo Build (H723 — from `FreeRTOS-OS/`)

| Command | Output | Description |
|---|---|---|
| `make all APP_DIR=demo TARGET_NAME=demo` | `build/demo.elf` | H723 demo firmware |
| `make flash TARGET_NAME=demo` | — | Flash `build/demo.elf` via OpenOCD |
| `make clean` | — | Delete `build/` |

### ECG H723 App Build (device-full-stack — from `FreeRTOS-OS/`)

> **`CONFIG_BOARD` is required.** The Makefile default is `stm32f411_devboard`. `autoconf.mk` does NOT override it. Omitting `CONFIG_BOARD=stm32h723_devboard` causes a build failure trying to open the F411 XML.

```bash
cd FreeRTOS-OS
cp ../app/kconfig_ecg_h723.conf .config && make config-outputs
make all  APP_DIR=../app TARGET_NAME=ecg_h723 CONFIG_BOARD=stm32h723_devboard
make flash            TARGET_NAME=ecg_h723 CONFIG_BOARD=stm32h723_devboard
```

| Command | Output | Description |
|---|---|---|
| `make all APP_DIR=../app TARGET_NAME=ecg_h723 CONFIG_BOARD=stm32h723_devboard` | `build/ecg_h723.elf` | ECG inference firmware (220 KB text, 126 KB BSS+data) |
| `make flash TARGET_NAME=ecg_h723 CONFIG_BOARD=stm32h723_devboard` | — | Flash via OpenOCD (verify + reset) |

Connect: `stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0`

To test all features on-board: invoke `/flash-and-test` in the Claude Code session (project-local skill at `.claude/commands/flash-and-test.md`).

### App Build (F411 — from project root `FreeRTOS-OS-App/`)

| Target | Output | Description |
|---|---|---|
| `make app` | `FreeRTOS-OS/build/app.elf` | Full firmware: OS + `app/` |
| `make kernel` | `FreeRTOS-OS/build/kernel.elf` | Standalone OS, no application |
| `make all` | same as `make app` | Default target |
| `make clean` | — | Delete `FreeRTOS-OS/build/` and generated BSP files |

### Flash

| Target | Description |
|---|---|
| `make flash-app` | Program `app.elf` into MCU flash via OpenOCD (verify + reset) |
| `make flash-kernel` | Program `kernel.elf` via OpenOCD |

### Code Generation

| Target | Reads | Writes |
|---|---|---|
| `make menuconfig` | `FreeRTOS-OS/Kconfig` | `FreeRTOS-OS/.config` |
| `make config-outputs` | `FreeRTOS-OS/.config` | `$(APP_DIR)/board/autoconf.h`, `autoconf.mk`, `stm32{f4,h7}xx_hal_conf.h` |
| `make board-gen` | `$(APP_DIR)/board/<board>.xml` | `board_config.c`, `board_device_ids.h`, `board_handles.h`, `mcu_config.h` |
| `make irq_gen` | `$(APP_DIR)/board/irq_table.xml` | `irq/irq_table.c`, `irq_hw_init_generated.c/.h`, `irq_periph_handlers_generated.h`, `irq_periph_vectors_generated.inc` |
| `python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml` | `app/board/dsp_dev.xml` | `app/board/dsp_config.h`, `app/board/dsp_config.mk` |

### Prerequisites

| Target | Installs |
|---|---|
| `make install-prerequisites` | All tools below in one shot |
| `make install-toolchain` | `arm-none-eabi-gcc` + `arm-none-eabi-gdb` |
| `make install-openocd` | OpenOCD flash/debug server |
| `make install-kconfig` | `kconfig-frontends` (provides `kconfig-mconf` for menuconfig) |
| `make install-debug-tools` | STM32F411 SVD file + Cortex-Debug VS Code extension |
| `make install-doxygen` | Doxygen + Graphviz (optional, for `make docs`) |

> All install targets are in `FreeRTOS-OS/Makefile` and must be run with `-C FreeRTOS-OS` or from inside that directory.

---

## Writing an Application

Implement `app_main()` in `app/app_main.c`. The OS calls it after the scheduler starts. Create tasks and return — do not block.

```c
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <services/gpio_mgmt.h>
#include <board/board_device_ids.h>

static void heartbeat(void *param)
{
    (void)param;
    uint32_t tick = 0;
    for (;;) {
        gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        printk("[heartbeat] tick %lu\n", (unsigned long)tick++);
        os_thread_delay(500);
    }
}

int app_main(void)
{
    os_thread_create(heartbeat, "heartbeat", 256, 1, NULL);
    return 0;
}
```

`printk()` accepts `printf`-style arguments. `printk_init()` and `printk_enable()` are called automatically by `uart_mgmt_thread` at boot (~20 ms after scheduler start) — no manual call needed in `app_main()`.

The shell CLI and `printk` UART assignment depends on the board:

**STM32H723ZGTx (NUCLEO-H723ZG) — single shell UART:**
```
UART_DEBUG (dev_id=1, USART3, PD8/PD9) ── STLink VCP ── /dev/ttyACM0
  └─ shell interactive I/O  +  printk() log output  (shared)
```

**STM32F411VET6 — separate UARTs:**
```
UART_DEBUG (dev_id=0, USART1, PA9/PA10)  ── ST-Link VCP  ── printk() log output
UART_APP   (dev_id=1, USART2, PA2/PA3)   ── USB-Serial   ── interactive shell
```

See [docs/OS_THREAD.md](docs/OS_THREAD.md) for the thread API, [docs/SHELL_CLI.md](docs/SHELL_CLI.md) for shell usage and connection setup.

---

## Prerequisites

### Automated install

```bash
# From FreeRTOS-OS/ directory:
make install-prerequisites
```

### Manual install

```bash
sudo bash FreeRTOS-OS/scripts/install_arm_gcc.sh     # ARM GCC + GDB
sudo bash FreeRTOS-OS/scripts/install_openocd.sh     # OpenOCD
sudo bash FreeRTOS-OS/scripts/install_kconfig.sh     # kconfig-frontends
bash FreeRTOS-OS/scripts/install_debug_tools.sh      # SVD + VS Code ext (no sudo)
sudo bash FreeRTOS-OS/scripts/install_doxygen.sh     # Doxygen (optional)
```

### ST-Link udev rules (Linux)

Required for non-root USB access to the ST-Link debugger:

```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# Reconnect ST-Link USB after this step.
```

### USB-to-Serial udev (Linux)

Required if `/dev/ttyUSB0` gives Permission denied:

```bash
sudo usermod -aG dialout $USER
# Log out and back in (or: newgrp dialout for the current shell only).
```

### VS Code debug setup

1. Open `FreeRTOS-OS-App/` in VS Code.
2. Run task **install-debug-tools** to download the SVD file.
3. Connect ST-Link (SWDIO, SWDCLK, GND — NRST optional).
4. Press **F5** → **Auto — Build, Flash & Debug**.

See [docs/DEBUG.md](docs/DEBUG.md) for full GDB and peripheral register viewer setup.

---

## ISR Priority Rules

FreeRTOS on Cortex-M uses `BASEPRI` to implement critical sections. Any peripheral ISR that calls FreeRTOS API (including `FROM_ISR` variants) **must** have NVIC priority **≥ `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY`** (= `5` on this project). Priorities 1–4 run above the BASEPRI mask and must not call any FreeRTOS API.

| NVIC priority | FreeRTOS API allowed? | Used for |
|---|---|---|
| 1–4 | **No** — above BASEPRI mask | HAL timebase timer (TIM6 on H7 / TIM1 on F4) |
| 5–14 | **Yes** — `FROM_ISR` variants only | UART, SPI, I2C, EXTI |
| 15 | Lowest — kernel reserved | SysTick, PendSV |

Set priorities in `app/board/irq_table.xml`, then regenerate:
```bash
make irq_gen && make app
```

See [docs/OS_INSIDE.md](docs/OS_INSIDE.md) for the full post-mortem and prevention guidelines.

---

## Supported Devices

Change with `make menuconfig` → *Target MCU part number*.

**Cortex-M7 (H7 family)**

| Config ID | Core | Clock | Flash | RAM | HAL Timebase |
|---|---|---|---|---|---|
| `STM32H723xx` ⭐ | Cortex-M7 | 64 MHz HSI (no PLL) | 1 MB | 128 KB DTCM | TIM6 (`TIM6_DAC_IRQn`) |

**Cortex-M4 (F4 family)**

| Config ID | Core | Clock | Flash | RAM | HAL Timebase |
|---|---|---|---|---|---|
| `STM32F411xE` | Cortex-M4F | 100 MHz | 512 KB | 128 KB | TIM1 (`TIM1_UP_TIM10_IRQn`) |
| `STM32F405xx` | Cortex-M4F | 168 MHz | 1 MB | 192 KB | TIM1 |
| `STM32F407xx` | Cortex-M4F | 168 MHz | 1 MB | 192 KB | TIM1 |
| `STM32F446xx` | Cortex-M4F | 180 MHz | 512 KB | 128 KB | TIM1 |
| `STM32F429xx` | Cortex-M4F | 180 MHz | 2 MB | 256 KB | TIM1 |

> **H7 note:** On H7 the HAL timebase uses TIM6 (dispatched via `TIM6_DAC_IRQHandler`), not TIM1. `TIM1_UP_IRQHandler` is an empty stub in the generated dispatch table. See [docs/OS_INSIDE.md](docs/OS_INSIDE.md) for the post-mortem.

---

## Medical-Grade Safety Layer (IEC 62304 / IEC 60601-1)

The ECG device-full-stack variant ships a complete safety subsystem above the FreeRTOS kernel. It is fully optional and Kconfig-gated; standard demo and F411 builds are unaffected.

### Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                       Safety Subsystem                              │
│                                                                     │
│   log/slog.c         — structured severity logger (ring buffer,     │
│                         severity levels, tick timestamp, IRQ-safe)  │
│                                                                     │
│   safety/wdog.c      — IWDG hw init + per-task sw watchdog bitmask │
│   safety/safe_state.c — coordinated shutdown → direct UART →       │
│                         AIRCR reset; .noinit record survives reset   │
│   drivers/hal/stm32/stm32_exceptions.c — naked fault trampolines,  │
│                         register dump, .noinit fault_record_t,      │
│                         reset instead of infinite loop              │
│                                                                     │
│   init/main.c        — boot_phase_t FSM (10 phases), checks prior  │
│                         fault + safe-state records before scheduler │
└─────────────────────────────────────────────────────────────────────┘
```

### Key files

| File | Purpose |
|---|---|
| `include/log/slog.h` + `log/slog.c` | 32-entry ring buffer logger; `LOG_E/W/I/D()` macros; printk echo for ERROR+WARN |
| `include/safety/wdog.h` + `safety/wdog.c` | IWDG init + SW task-bitmask watchdog service thread |
| `include/safety/safe_state.h` + `safety/safe_state.c` | `safety_enter_safe_state(reason)` — noreturn coordinated shutdown |
| `include/safety/fault_handler.h` | Boot-time API to query/clear `.noinit` fault records |
| `include/def_err.h` | 15 error codes (`OS_ERR_NONE` … `OS_ERR_NOT_INIT`) for IEC 62304 traceability |
| `include/def_attributes.h` | `__SECTION_NOINIT` macro — places data outside `.bss` zero-clear |

### Kconfig keys (ECG H723 app)

| Key | Default | Meaning |
|---|---|---|
| `CONFIG_INC_SERVICE_WDOG` | `n` | Enable IWDG + sw-watchdog service thread |
| `CONFIG_HAL_IWDG_MODULE_ENABLED` | `n` | Pull `stm32h7xx_hal_iwdg.c` into the build |
| `PROC_SERVICE_WDOG_STACK_SIZE` | `512` | Watchdog service task stack words |
| `PROC_SERVICE_WDOG_PRIORITY` | `4` | Watchdog service priority (above app tasks) |
| `SLOG_PRINTK_MIN_LEVEL` | `1` | Minimum slog level echoed to printk (0=ERROR only, 1=+WARN, 2=+INFO) |

### Watchdog design

```
wdog_hw_init()   — called from main() BEFORE scheduler; IWDG cannot stop
                   H7: IWDG1, LSI/64, ~4 s; F4: IWDG, same prescaler
wdog_sw_init()   — called from app_main() to register per-task bitmask slots
wdog_task_kick() — called inside every critical task each iteration

Slots (ECG app):
  WDOG_SLOT_HEARTBEAT (0) — heartbeat_task, every 500 ms
  WDOG_SLOT_ECG_ACQ   (1) — ecg_acquire_task, per completed window + idle retry
  WDOG_SLOT_ECG_INF   (2) — ecg_infer_task, every ulTaskNotifyTake (2 s timeout)

wdog_service_thread checks every 1000 ms:
  • all slots set → kick IWDG + clear bitmask
  • any slot missing → increment missed_checks counter
  • missed_checks >= 3 → safety_enter_safe_state(SAFE_REASON_WATCHDOG) → AIRCR reset
```

### Boot state machine (`init/main.c`)

```c
BOOT_HAL_INIT      (0) — HAL_Init()
BOOT_CLK_INIT      (1) — _stm32_clock_init()
BOOT_PERIPH_CLK    (2) — peripheral RCC enables
BOOT_IRQ_INIT      (3) — IRQ table init
BOOT_IPC_INIT      (4) — IPC queues
BOOT_SLOG_INIT     (5) — slog_init()        ← first LOG_ call after this
BOOT_PREV_FAULT_CHK(6) — check .noinit fault + safe-state records
BOOT_SERVICES_INIT (7) — OS services (wdog_service_start() first)
BOOT_WDOG_HW_INIT  (8) — wdog_hw_init()    ← IWDG starts, cannot stop
BOOT_APP_INIT      (9) — app_main()
BOOT_SCHEDULER    (10) — vTaskStartScheduler()
```

Any phase failure calls `safety_enter_safe_state(SAFE_REASON_BOOT_FAIL)`.

### `.noinit` RAM section

Placed after `._user_heap_stack` in `stm32_h723_FLASH.ld` (DTCM), outside `[__bss_start__, __bss_end__]`. The startup zero-loop does not touch it, so `fault_record_t` (magic `0xDEADC0DE`) and the safe-state record (magic `0xC0DEBEEF`) survive NVIC soft reset and are inspected at `BOOT_PREV_FAULT_CHK`.

---

## Static Analysis

FreeRTOS-OS ships a complete CPPcheck + MISRA C:2012 analysis workflow.

```bash
# Install CPPcheck (once per machine)
./scripts/install_cppcheck.sh

# Run CPPcheck (warnings and above)
./scripts/run_cppcheck.sh

# Run CPPcheck + MISRA C:2012 checks
./scripts/run_cppcheck.sh --misra

# Generate HTML reports
./scripts/run_cppcheck.sh --misra --severity=style --html
```

Reports are written to `reports/cppcheck/` (git-ignored). A GitHub Actions pipeline runs the checks automatically on every push and pull request.

See **[docs/CHECK.md](docs/CHECK.md)** for the full reference: options, report formats, MISRA deviation table, and CI/CD integration.

---

## Known Bugs / Fixes

| Date | Component | Root Cause | Fix |
|---|---|---|---|
| 2026-05-16 | Build system | `CONFIG_BOARD` Make variable defaults to `stm32f411_devboard`; not written to `autoconf.mk`, so H723 ECG builds fail looking for the F411 XML. | Pass `CONFIG_BOARD=stm32h723_devboard` on every `make all` and `make flash` for the ECG app. |
| 2026-05-16 | `app/board/irq_periph_handlers_generated.h` | Only 5 active ISR handlers declared; `irq_periph_vectors_generated.inc` references ~75 additional names (DMA, TIM1–TIM8, I2C, SPI, WWDG…) that were undeclared in the startup TU → linker error. | Added `__attribute__((weak, alias("Default_Handler")))` declarations for all 75 unused peripheral vectors. Active handlers remain plain `void Foo_IRQHandler(void);` declarations. |
| 2026-05-16 | `drivers/hal/stm32/hal_iic_stm32.c` | File-level guard was `#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)` only — missing `&& defined(HAL_I2C_MODULE_ENABLED)`. With I2C disabled in kconfig, HAL I2C types/functions are absent → compile error. `hal_spi_stm32.c` already had the correct pattern. | Changed opening `#if` to include `&& defined(HAL_I2C_MODULE_ENABLED)`; updated closing `#endif` comment. |

---

## License

FreeRTOS-OS is distributed under the **GNU General Public License v3.0**.
See `COPYING` or <https://www.gnu.org/licenses/gpl-3.0.html>.

Submodule licenses: FreeRTOS-Kernel — MIT · STM32F4xx HAL — BSD 3-Clause · CMSIS-6 — Apache 2.0 · CMSIS-DSP — Apache 2.0 · CANopen stack — Apache 2.0 · ringbuffer — GPL v2.

---

*Author: Subhajit Roy — subhajitroy005@gmail.com*
