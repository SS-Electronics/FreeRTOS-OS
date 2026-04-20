# Debug Framework — FreeRTOS-OS

This document describes the complete VSCode debug implementation for FreeRTOS-OS running on the STM32F411VET6 target. The setup mirrors the STM32CubeIDE debug experience — single F5 keypress to build, flash, and halt at `main()` — while adding FreeRTOS task-level awareness, peripheral register inspection, and ITM/SWO output.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Prerequisites](#prerequisites)
- [File Layout](#file-layout)
- [First-Time Setup](#first-time-setup)
- [Debug Configurations](#debug-configurations)
  - [Build, Flash and Debug](#1-build-flash-and-debug)
  - [Attach](#2-attach)
  - [Flash Only](#3-flash-only)
- [VSCode Task Pipeline](#vscode-task-pipeline)
- [OpenOCD Configuration](#openocd-configuration)
  - [Probe and Transport](#probe-and-transport)
  - [Flash Bank Declaration](#flash-bank-declaration)
  - [Reset Strategy](#reset-strategy)
  - [GDB Event Hooks](#gdb-event-hooks)
  - [Ports](#ports)
- [GDB Initialisation Sequence](#gdb-initialisation-sequence)
- [FreeRTOS Task Awareness](#freertos-task-awareness)
- [Peripheral Register Viewer (SVD)](#peripheral-register-viewer-svd)
- [SWO / ITM Output](#swo--itm-output)
- [Debug Panels Reference](#debug-panels-reference)
- [Command-Line Debug (without VSCode)](#command-line-debug-without-vscode)
- [Makefile Flash Target](#makefile-flash-target)
- [Troubleshooting](#troubleshooting)
- [Porting to a Different Board](#porting-to-a-different-board)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         VSCode                                     │
│                                                                     │
│  F5 keypress                                                        │
│    │                                                                │
│    ├─ tasks.json "build"  →  make all  →  build/kernel.elf         │
│    │                                                                │
│    └─ launch.json (Cortex-Debug extension)                         │
│           │                                                         │
│           ├─ spawns: arm-none-eabi-gdb                              │
│           │     │   (GDB client)                                    │
│           │     │                                                   │
│           │     │  GDB Remote Serial Protocol (port 3333)          │
│           │     ▼                                                   │
│           └─ spawns: openocd                                        │
│                 │   (GDB server + flash programmer)                 │
│                 │                                                   │
│                 │  SWD / HLA via USB                                │
│                 ▼                                                   │
│           ST-Link v2 probe                                          │
│                 │                                                   │
│                 │  SWDIO / SWDCLK / NRST                            │
│                 ▼                                                   │
│           STM32F411VET6                                             │
│           (ARM Cortex-M4F target)                                   │
└─────────────────────────────────────────────────────────────────────┘

Panels rendered by Cortex-Debug:
  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────┐
  │ RTOS Threads │  │ Peripherals  │  │   Memory     │  │   SWO    │
  │ (FreeRTOS)   │  │ (SVD regs)   │  │  Inspector   │  │  Output  │
  └──────────────┘  └──────────────┘  └──────────────┘  └──────────┘
```

**Key components:**

| Component | Role |
|-----------|------|
| `marus25.cortex-debug` | VSCode extension — orchestrates GDB + OpenOCD, renders debug panels |
| `arm-none-eabi-gdb` | GDB client — sends debug commands to OpenOCD over port 3333 |
| `openocd` | GDB server — translates GDB RSP to SWD transactions on the probe |
| ST-Link v2 | USB debug probe — physical SWD/NRST connection to the MCU |
| `STM32F411.svd` | XML register description — drives the Peripherals panel |
| `arch/debug_cfg/stm32_f411xx_debug.cfg` | OpenOCD target script — flash bank, reset strategy, event hooks |

---

## Prerequisites

### Hardware
- STM32F411VET6 target board
- ST-Link v2 probe (built into Nucleo/Discovery boards, or external)
- USB cable connecting the probe to the host PC

### Software — install with the provided scripts

```bash
# ARM GCC toolchain (provides arm-none-eabi-gdb)
sudo bash scripts/install_arm_gcc.sh

# OpenOCD (GDB server + flash programmer)
sudo bash scripts/install_openocd.sh

# Cortex-Debug VSCode extension + STM32F411 SVD file
bash scripts/install_debug_tools.sh
```

`install_debug_tools.sh` does the following automatically:
1. Verifies `arm-none-eabi-gdb` and `openocd` are in `PATH`
2. Downloads `STM32F411.svd` from the `posborne/cmsis-svd` repository into `arch/debug_cfg/`
3. Installs `marus25.cortex-debug` and all other recommended extensions via `code --install-extension`

### Verify
```bash
arm-none-eabi-gdb --version   # expect 12.x or newer
openocd --version             # expect 0.12 or newer
```

---

## File Layout

```
FreeRTOS-OS/
│
├── .vscode/
│   ├── launch.json          ← Debug configurations (3 modes)
│   ├── tasks.json           ← Build, flash, OpenOCD server tasks
│   ├── settings.json        ← Cortex-Debug paths, editor prefs
│   ├── extensions.json      ← Recommended extensions list
│   └── c_cpp_properties.json← IntelliSense include paths + defines
│
├── arch/debug_cfg/
│   ├── stm32_f411xx_debug.cfg  ← OpenOCD target configuration
│   ├── stm32_f401xx_debug.cfg  ← (alternative for F401 targets)
│   └── STM32F411.svd           ← Downloaded by install_debug_tools.sh
│
├── scripts/
│   ├── gen_active_debug.py     ← Reads autoconf.h → writes build/active_debug.cfg
│   ├── install_debug_tools.sh  ← One-shot setup (SVD + extensions)
│   ├── run_openocd_server.sh   ← Manual OpenOCD server launcher
│   └── run_gdb.sh              ← Manual GDB launcher
│
├── build/
│   └── active_debug.cfg        ← Generated by gen_active_debug.py (do not edit)
│
└── Makefile                    ← Includes `flash` target
```

---

## First-Time Setup

```bash
# 1. Install tools
bash scripts/install_debug_tools.sh

# 2. Open workspace
code .
# VSCode will prompt to install recommended extensions — click "Install All"

# 3. Build firmware once to verify toolchain
make all
# → build/kernel.elf

# 4. Connect ST-Link to the board

# 5. Press F5
# Select "STM32F411 — Build, Flash & Debug" if prompted
```

From this point the workflow is identical to STM32CubeIDE:

| Action | Keyboard |
|--------|----------|
| Build, flash, and debug | `F5` |
| Step over | `F10` |
| Step into | `F11` |
| Step out | `Shift+F11` |
| Continue | `F5` |
| Pause | `F6` |
| Stop | `Shift+F5` |
| Toggle breakpoint | `F9` |
| Build only | `Ctrl+Shift+B` |

---

## Debug Configurations

All three configurations are defined in `.vscode/launch.json`. Select them from the Run and Debug sidebar (`Ctrl+Shift+D`) drop-down, or use the keyboard shortcut shown.

### 1. Build, Flash and Debug

**When to use:** Normal development cycle — the equivalent of pressing F5 in STM32CubeIDE.

**What happens (in order):**

```
F5 pressed
  │
  ├─ [preLaunchTask] tasks.json "build" runs
  │     └─ make all
  │           ├─ make board-gen (if XML newer than generated files)
  │           └─ arm-none-eabi-gcc ... → build/kernel.elf
  │
  ├─ Cortex-Debug spawns OpenOCD:
  │     openocd -f arch/debug_cfg/stm32_f411xx_debug.cfg
  │     OpenOCD connects to ST-Link via USB (HLA/SWD)
  │     OpenOCD resets and halts the target (connect_assert_srst)
  │
  ├─ Cortex-Debug spawns GDB:
  │     arm-none-eabi-gdb build/kernel.elf
  │
  ├─ GDB connects to OpenOCD:
  │     target extended-remote localhost:3333
  │
  ├─ [preLaunchCommands] GDB initialisation:
  │     set mem inaccessible-by-default off   ← allow peripheral reads
  │     set print pretty on                   ← formatted struct display
  │     set print object on                   ← C++ polymorphic types
  │     set pagination off                    ← no "press enter" prompts
  │
  ├─ Cortex-Debug issues:
  │     load                                  ← programs ELF into flash
  │     compare-sections                      ← verifies flash contents
  │
  ├─ [postLaunchCommands]:
  │     monitor reset halt                    ← clean reset after flash
  │
  ├─ runToEntryPoint: "main"
  │     GDB sets a temporary breakpoint on main()
  │     GDB issues: continue
  │     Target runs from reset vector through startup code
  │     Execution halts at main()
  │
  └─ Debug session live — all panels populated
```

**`launch.json` key fields:**

```jsonc
{
    "name": "STM32F411 — Build, Flash & Debug",
    "type": "cortex-debug",
    "request": "launch",
    "servertype": "openocd",
    "configFiles": ["${workspaceFolder}/build/active_debug.cfg"],  // generated by detect-target
    "executable": "${workspaceFolder}/build/kernel.elf",
    "gdbPath": "arm-none-eabi-gdb",
    "device": "STM32F411VE",
    "interface": "swd",
    "runToEntryPoint": "main",
    "svdFile": "${workspaceFolder}/arch/debug_cfg/STM32F411.svd",
    "rtos": "FreeRTOS",
    "preLaunchTask": "detect-target"   // runs gen_active_debug.py then build
}
```

> `build/active_debug.cfg` is auto-generated by the `detect-target` task before every launch. It reads `CONFIG_TARGET_MCU` from `config/autoconf.h` and emits an OpenOCD `source {}` line pointing to the correct `arch/debug_cfg/*.cfg` file. This means switching targets requires no manual edits to `launch.json`.


---

### 2. Attach

**When to use:** The target is already running (flashed previously). You want to inspect its live state without resetting or reflashing — equivalent to STM32CubeIDE's "Debug → Attach to Running Target".

**What happens:**

```
"Attach" selected → F5
  │
  ├─ No preLaunchTask — build is NOT run
  │
  ├─ Cortex-Debug spawns OpenOCD (same cfg file)
  │
  ├─ GDB connects and issues:
  │     target extended-remote localhost:3333
  │
  ├─ [postLaunchCommands]:
  │     monitor halt             ← halts the currently running target
  │     set mem inaccessible-by-default off
  │     set print pretty on
  │     set pagination off
  │
  └─ Debug session live — target is halted at whatever PC it was at
     Call stack, variables, and peripherals are all readable
```

**Important:** The ELF file must match what is in flash. If the firmware was recompiled without flashing, the symbols will be mismatched and variable values will be wrong.

---

### 3. Flash Only

**When to use:** You want to program the board and let it run without opening a debug session — equivalent to the STM32CubeIDE "Run" button or `make flash`.

**What happens:**

```
"Flash Only" selected → F5
  │
  ├─ [preLaunchTask] "build" runs (make all)
  │
  ├─ Cortex-Debug starts OpenOCD + GDB
  │
  ├─ GDB: load  → programs flash
  │
  ├─ [postLaunchCommands]:
  │     monitor reset run   ← reset and execute (no halting)
  │     disconnect          ← GDB disconnects from OpenOCD
  │     quit                ← GDB exits
  │
  └─ OpenOCD exits (no remaining clients)
     Target is running freely
```

This configuration is useful for production-style programming or when debugging is not needed but a fresh flash is required.

---

## VSCode Task Pipeline

Tasks are defined in `.vscode/tasks.json` and invoked via `Ctrl+Shift+P → Tasks: Run Task` or bound to the build shortcut `Ctrl+Shift+B`.

```
board-gen ──────────────────────────────────────────────────────┐
  make board-gen                                                 │
  scripts/gen_board_config.py boards/<board>.xml                 │
  → app/board/board_config.c                                    ▼
  → app/board/board_device_ids.h                           build (default: Ctrl+Shift+B)
                                                               make all
config-outputs ─────────────────────────────────────────────────┤  (auto-runs board-gen if XML newer)
  make config-outputs                                            │  → build/kernel.elf
  → config/autoconf.h                                           │  → build/kernel.elf.hex
  → autoconf.mk                                                 │  → build/kernel.elf.asm
                                                                │
detect-target (preLaunchTask in launch.json) ───────────────────┤
  python3 scripts/gen_active_debug.py                           │
  reads: config/autoconf.h → CONFIG_TARGET_MCU                  │
  writes: build/active_debug.cfg  (OpenOCD wrapper)             │
          build/debug_env.json    (device, svd, cpu_freq …)     │
                                                                ▼
                                                             flash
                                                               make flash
                                                               openocd -c "program ... verify reset exit"
```

### Task reference

| Label | Command | Purpose |
|-------|---------|---------|
| `build` | `make all` | Default build — BSP gen + compile + link |
| `board-gen` | `make board-gen` | Regenerate BSP from board XML |
| `build (custom board)` | `make all CONFIG_BOARD=<input>` | Build for a different board |
| `flash` | `make flash` | Flash via OpenOCD (depends on build) |
| `clean` | `make clean` | Remove build artifacts and generated BSP |
| `menuconfig` | `make menuconfig` | Kconfig ncurses menu |
| `config-outputs` | `make config-outputs` | Regenerate `autoconf.h` from `.config` |
| `detect-target` | `python3 scripts/gen_active_debug.py` | Reads `config/autoconf.h`, writes `build/active_debug.cfg` and `build/debug_env.json` — runs as `preLaunchTask` before every debug/flash launch |
| `openocd-server` | `openocd -f ...` | Start OpenOCD server (for Attach mode) |
| `install-debug-tools` | `bash scripts/install_debug_tools.sh` | One-shot setup |

### GCC problem matcher

The `build` task uses a custom `problemMatcher` regex that parses GCC diagnostics:

```
^(.+):(\d+):(\d+):\s+(warning|error):\s+(.+)$
```

This populates the VSCode **Problems** panel (`Ctrl+Shift+M`) with clickable links to every compiler warning and error — identical to the Problems view in STM32CubeIDE.

### `openocd-server` task (background)

The `openocd-server` task starts OpenOCD as a background process (`isBackground: true`). VSCode tracks its readiness via:

```jsonc
"background": {
    "activeOnStart": true,
    "beginsPattern": "Open On-Chip Debugger",
    "endsPattern": "Listening on port 3333"
}
```

VSCode considers the task "done" (i.e., ready for dependent tasks) when it sees `Listening on port 3333` in the terminal output. The "Build, Flash and Debug" config does **not** use this task — Cortex-Debug starts its own OpenOCD instance internally.

---

## OpenOCD Configuration

**File:** [`arch/debug_cfg/stm32_f411xx_debug.cfg`](arch/debug_cfg/stm32_f411xx_debug.cfg)

This file is the single OpenOCD script for the STM32F411VET6. It is used by:
- `launch.json` (`"configFiles"` field)
- `make flash` (via `$(OPENOCD_TARGET)` in the Makefile)
- `scripts/run_openocd_server.sh` (`-f` argument)

### Probe and Transport

```tcl
source [find interface/stlink.cfg]
transport select hla_swd
```

`interface/stlink.cfg` is a built-in OpenOCD script that configures the ST-Link USB driver (libusb HLA backend). `hla_swd` selects the Serial Wire Debug transport (2-wire SWDIO + SWDCLK), which is faster and more reliable than JTAG for STM32F4 targets.

### Flash Bank Declaration

```tcl
set CHIPNAME stm32f411xe
source [find target/stm32f4x.cfg]

flash bank stm32f4x.flash stm32f4x 0x08000000 0x00080000 0 0 $_TARGETNAME
```

`target/stm32f4x.cfg` is the generic STM32F4xx target script. It sets up the Cortex-M4 core, RAM regions, and Flash Patch/Breakpoint (FPB) unit.

The explicit `flash bank` declaration:
- **Address:** `0x08000000` — start of internal flash
- **Size:** `0x00080000` — 512 KB (STM32F411VET6 full flash)
- **Driver:** `stm32f4x` — handles sector-based erase and program sequences

Without this declaration, OpenOCD may autodetect the wrong flash geometry or fail to erase correctly.

### Reset Strategy

```tcl
reset_config srst_only srst_nogate connect_assert_srst
```

| Option | Effect |
|--------|--------|
| `srst_only` | Uses the NRST pin for system reset (not JTAG TRST) |
| `srst_nogate` | Allows debug access during the reset pulse |
| `connect_assert_srst` | Holds NRST low while OpenOCD connects — prevents any MCU code from running before the halt |

`connect_assert_srst` is essential for FreeRTOS targets: without it the MCU starts executing from reset, may enable interrupts, and reach a state that conflicts with the halt request.

### GDB Event Hooks

```tcl
$_TARGETNAME configure -event gdb-attach {
    echo "GDB attached — halting target"
    halt
}

$_TARGETNAME configure -event gdb-flash-erase-start {
    reset halt
}
```

- **`gdb-attach`** — fires when GDB first connects. Ensures the target is halted regardless of whether it was already running, preventing GDB from reading a moving target.
- **`gdb-flash-erase-start`** — fires just before `load` erases flash. Issues a `reset halt` so the PC is at the reset vector, not mid-execution in some task.

### Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 3333 | GDB Remote Serial Protocol | GDB client connection (arm-none-eabi-gdb) |
| 4444 | Telnet | Interactive OpenOCD console (manual commands) |
| 6666 | TCL RPC | Scripted OpenOCD control |

Connect to the Telnet console for live commands while debugging:
```bash
telnet localhost 4444
> halt
> reg
> mdw 0x40011004    # read GPIOA ODR
> resume
```

### SWD Clock Speed

```tcl
adapter speed 4000
```

4 MHz is a conservative default that works reliably on all board lengths. If you see `libusb_bulk_transfer` errors or connection timeouts, reduce to `1000`. For very short wiring (PCB-mounted probe), increase to `8000` for faster flash programming.

---

## GDB Initialisation Sequence

Cortex-Debug issues GDB commands in this order when a "launch" session starts:

```
1. target extended-remote localhost:3333      ← connect to OpenOCD

2. [preLaunchCommands] (from launch.json):
     set mem inaccessible-by-default off      ← read peripheral regs without fault
     set print pretty on                      ← struct display like STM32CubeIDE
     set print object on                      ← show derived C++ types
     set pagination off                       ← suppress "(More)" paging

3. monitor reset halt                         ← halt target via OpenOCD

4. load                                       ← program ELF sections into flash
   compare-sections                           ← verify flash == ELF

5. [postLaunchCommands]:
     monitor reset halt                       ← clean reset after programming

6. tbreak main                                ← temporary breakpoint at main()
   continue                                   ← run from reset vector

7. [Target stops at main()]
   ← All panels populate: Variables, RTOS Threads, Peripherals, Call Stack
```

For the **Attach** configuration the sequence is shorter — steps 3–6 are replaced by:
```
monitor halt                                  ← halt wherever the target is
```

---

## FreeRTOS Task Awareness

Setting `"rtos": "FreeRTOS"` in `launch.json` enables the Cortex-Debug FreeRTOS RTOS plugin.

### What it does

At each halt, the plugin reads FreeRTOS internal kernel structures from the target RAM:
- `pxCurrentTCB` — pointer to the currently running task control block
- `pxReadyTasksLists[]` — ready queue (all tasks by priority)
- `xSuspendedTaskList` — suspended tasks
- `xDelayedTaskList1/2` — sleeping tasks

From these structures it reconstructs the full task list and **unwinds each task's stack** to produce a separate call stack per task.

### RTOS Threads panel

The panel shows the same information as STM32CubeIDE's **FreeRTOS Task List**:

```
RTOS Threads
├── uart_mgmt     [Running]   Stack: 487/512 words
├── iic_mgmt      [Blocked]   Stack: 901/1024 words
├── spi_mgmt      [Blocked]   Stack: 498/512 words
├── gpio_mgmt     [Blocked]   Stack: 241/256 words
└── IDLE          [Ready]     Stack: 115/128 words
```

Clicking any task switches the Call Stack panel to that task's stack frame — you can inspect local variables of a suspended task without it being the current thread.

### Source-level FreeRTOS stepping

`settings.json` sets:
```json
"cortex-debug.rtos.freertos.kernelPath": "${workspaceFolder}/kernel/FreeRTOS-Kernel"
```

This tells Cortex-Debug where the FreeRTOS source lives so that stepping into `vTaskDelay()`, `xQueueSend()`, etc. shows the actual C source rather than disassembly.

### Stack high-water mark visibility

With symbols loaded, you can inspect any task's stack usage in the Watch panel:
```
(gdb) p uxTaskGetStackHighWaterMark(xTaskGetHandle("uart_mgmt"))
$1 = 25
```

This reports the minimum free words ever on that stack — identical to the **Stack Usage** column in STM32CubeIDE's task inspector.

---

## Peripheral Register Viewer (SVD)

The SVD (System View Description) file is an XML document published by ST that maps every peripheral register to a name, address, bit-field layout, and description.

**File:** `arch/debug_cfg/STM32F411.svd` (downloaded by `install_debug_tools.sh`)

**Source:** `posborne/cmsis-svd` on GitHub — an open-source aggregator of all vendor SVD files.

### How it works

Cortex-Debug parses the SVD file and creates the **Peripherals** panel. At each halt it reads the actual register values from the target and displays them with field names.

```
Peripherals
├── GPIOA
│   ├── MODER   = 0xA8000000   [MODER15=10 MODER14=10 ...]
│   ├── ODR     = 0x00000200   [ODR9=1 (TX pin high)]
│   └── IDR     = 0x00000400   [IDR10=1 (RX pin)]
├── USART1
│   ├── SR      = 0x000000C0   [TC=1 TXE=1]
│   ├── DR      = 0x00000000
│   ├── BRR     = 0x0000008A   (→ 115200 baud @ 100 MHz)
│   └── CR1     = 0x0000200C   [UE=1 TE=1 RE=1]
├── RCC
│   ├── AHB1ENR = 0x00000007   [GPIOAEN GPIOBEN GPIOCEN]
│   └── APB2ENR = 0x00000010   [USART1EN]
└── ...
```

This is equivalent to STM32CubeIDE's **Peripheral Registers** view.

### Enabling it

The SVD path is set in `launch.json`:
```jsonc
"svdFile": "${workspaceFolder}/arch/debug_cfg/STM32F411.svd"
```

If the file is missing, all other debug features still work but the Peripherals panel will not appear.

---

## SWO / ITM Output

SWO (Single Wire Output) lets the MCU transmit `printf`-style debug data to the host through the debug probe without using a UART. This is what `ITM_DEBUG_EN` in `os_config.h` enables.

### Hardware requirement

SWO requires the ST-Link probe to be connected to the MCU's **SWO pin** (PB3 / TRACESWO on STM32F411) in addition to the standard SWDIO/SWDCLK lines. Nucleo boards have this connected by default.

### Enabling in launch.json

SWO is configured per launch configuration. Change `"enabled": false` to `"enabled": true`:

```jsonc
"swoConfig": {
    "enabled": true,
    "cpuFrequency": 100000000,   // HCLK in Hz — must match actual PLL output
    "swoFrequency": 2000000,     // SWO baud — 2 MHz works for most probes
    "source": "probe",           // ST-Link reads SWO from the probe USB
    "decoders": [
        {
            "type": "console",
            "label": "ITM port 0",
            "port": 0,           // ITM stimulus port 0 → this console
            "encoding": "ascii"
        }
    ]
}
```

### Enabling in firmware

In `config/conf_os.h` (or via `make menuconfig` → **OS Services → ITM Debug Enable**):
```c
#define ITM_DEBUG_EN   (1)
```

The `ITM_PRINT_BUFF_LENGTH` macro controls the buffer used by `printk()` for ITM output.

### Output location

SWO output appears in the **SWO: ITM port 0** terminal tab inside VSCode's panel — identical to the **SWV ITM Data Console** in STM32CubeIDE.

---

## Debug Panels Reference

| VSCode Panel | How to open | STM32CubeIDE equivalent |
|---|---|---|
| **Variables** | Automatic in Run & Debug sidebar | Variables view |
| **Watch** | Add expressions in Run & Debug sidebar | Expressions view |
| **Call Stack** | Automatic in Run & Debug sidebar | Call Stack view |
| **RTOS Threads** | Automatic when `"rtos": "FreeRTOS"` | FreeRTOS Task List |
| **Peripherals** | Cortex-Debug sidebar (requires SVD) | Peripheral Registers |
| **Memory** | `Ctrl+Shift+P → Open Memory Inspector` | Memory Browser |
| **Disassembly** | Right-click source line → Open Disassembly | Disassembly view |
| **Debug Console** | Bottom panel → Debug Console tab | Debug Console |
| **SWO Output** | Bottom panel → SWO tab (when enabled) | SWV ITM Console |
| **Problems** | `Ctrl+Shift+M` | Problems view |

### Memory inspector

The memory inspector can view any address range in real time. Useful for inspecting:
- FreeRTOS heap: `0x20000000` (start of RAM)
- Stack of a specific task: get address from `RTOS Threads` panel
- Peripheral registers: e.g. `0x40011000` (GPIOA)
- Flash contents: `0x08000000`

### Live variable watch

Variables in the Watch panel update at every halt. For live updates **without halting**, use a FreeRTOS-aware expression like:

```
*(uint32_t*)0x20001234      // raw address watch
g_board_config.uart_count   // global struct field
```

---

## Command-Line Debug (without VSCode)

### Start OpenOCD server manually

```bash
bash scripts/run_openocd_server.sh
# Prints ports and waits — keep this terminal open
```

Or directly:
```bash
openocd -f arch/debug_cfg/stm32_f411xx_debug.cfg \
        -c "gdb_port 3333" \
        -c "telnet_port 4444" \
        -c "tcl_port 6666"
```

### Connect GDB manually

```bash
bash scripts/run_gdb.sh
# Equivalent to:
arm-none-eabi-gdb \
    -ex "set mem inaccessible-by-default off" \
    -ex "set print pretty on" \
    -ex "target remote localhost:3333" \
    -ex "monitor reset halt" \
    -ex "load" \
    -ex "monitor reset halt" \
    -ex "break main" \
    -ex "continue" \
    build/kernel.elf
```

### Useful GDB commands

```
# Execution control
(gdb) continue                  # run
(gdb) interrupt                 # halt (Ctrl+C)
(gdb) step                      # step into (source line)
(gdb) next                      # step over
(gdb) finish                    # step out
(gdb) until <line>              # run to line

# Breakpoints
(gdb) break main                # break at function
(gdb) break uart_mgmt.c:87     # break at file:line
(gdb) watch g_var               # data watchpoint (halts on write)
(gdb) rwatch g_var              # read watchpoint
(gdb) info breakpoints          # list all

# Registers and memory
(gdb) info registers            # show core registers
(gdb) x/16xw 0x20000000        # hex dump 16 words from address
(gdb) p/x *((uint32_t*)0x40011014) # read GPIOA ODR register

# OpenOCD commands (via 'monitor' prefix)
(gdb) monitor halt
(gdb) monitor reset halt
(gdb) monitor reset run
(gdb) monitor flash info 0      # show flash bank info
(gdb) monitor reg               # show all core registers

# FreeRTOS
(gdb) p pxCurrentTCB->pcTaskName    # current task name
(gdb) p uxCurrentNumberOfTasks      # number of live tasks
(gdb) p uxTaskGetStackHighWaterMark(xTaskGetHandle("uart_mgmt"))
```

---

## Makefile Flash Target

The `flash` Makefile target programs the ELF without opening a debug session:

```makefile
flash: $(BUILD)/kernel.elf
    openocd -f $(OPENOCD_TARGET) \
            -c "program $(BUILD)/kernel.elf verify reset exit"
```

| OpenOCD option | Effect |
|---|---|
| `program <elf>` | Erase + write ELF sections to flash |
| `verify` | Read-back and compare flash with ELF (catches write errors) |
| `reset` | Issue system reset after programming |
| `exit` | Quit OpenOCD after completion |

```bash
# Default board
make flash

# Custom board
make flash CONFIG_BOARD=my_board

# Flash without rebuilding (use existing ELF)
openocd -f arch/debug_cfg/stm32_f411xx_debug.cfg \
        -c "program build/kernel.elf verify reset exit"
```

`$(OPENOCD_TARGET)` is set in the Makefile based on `CONFIG_TARGET_MCU`:
```makefile
ifeq ($(CONFIG_TARGET_MCU),"STM32F411xE")
    OPENOCD_TARGET += arch/debug_cfg/stm32_f411xx_debug.cfg
endif
```

---

## Troubleshooting

### ST-Link not found / USB error

```
Error: open failed
```
- Check USB cable and connection
- On Linux: `sudo usermod -aG plugdev $USER && sudo udevadm trigger`
- Add udev rule:
  ```bash
  echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="666"' \
    | sudo tee /etc/udev/rules.d/70-stlink.rules
  sudo udevadm control --reload-rules
  ```
- Verify probe is detected: `lsusb | grep STMicroelectronics`

### Target not halting / `connect_assert_srst` fails

```
Error: timed out while waiting for target halted
```
- Check NRST pin is connected between probe and board
- Try removing `connect_assert_srst` from the cfg and retry
- Reduce `adapter speed` to `1000`

### Flash verification fails

```
Error: checksum mismatch
```
- Power-cycle the board (not just reset)
- Check for a running watchdog that resets before verify completes — disable IWDG in `os_config.h` during development

### GDB reports wrong variable values after rebuild

The ELF in `build/kernel.elf` does not match what is in flash. Either:
- Use **Build, Flash and Debug** (not Attach) to always reflash
- Or run `make flash` before attaching

### `libusb_bulk_transfer` errors / CRC errors

```
Error: STLINK_XFER_ERR_NO_MEM
```
- Reduce `adapter speed 4000` to `adapter speed 1000` in the cfg file
- Check for long or poor-quality SWD wiring

### Peripherals panel empty (no SVD)

```
arch/debug_cfg/STM32F411.svd: No such file or directory
```
- Run `bash scripts/install_debug_tools.sh`
- Or manually download and place the file

### FreeRTOS threads not showing

- Verify `"rtos": "FreeRTOS"` is in `launch.json`
- Cortex-Debug needs the ELF to contain FreeRTOS symbols — ensure `CONFIG_RTOS_FREERTOS` is enabled and compiled with `-g3`
- The FreeRTOS version must be supported by Cortex-Debug's RTOS plugin (any recent version works)

### SWO output is garbled

- `cpuFrequency` must exactly match the actual HCLK. The PLL output for STM32F411 is 100 MHz (HSI 16 MHz → PLLM=16 → PLLN=200 → PLLP=2) — configured in `drivers/hal/stm32/hal_rcc_stm32.c`
- Verify the SWO pin (PB3) is not used by another peripheral
- Reduce `swoFrequency` from `2000000` to `1000000`

---

## Porting to a Different Board

### Different STM32F4 variant

1. Copy `arch/debug_cfg/stm32_f411xx_debug.cfg` to a new file
2. Update `set CHIPNAME` and `flash bank` size:

   | MCU | CHIPNAME | Flash size |
   |-----|----------|-----------|
   | STM32F401xE | `stm32f401xe` | `0x00080000` (512 KB) |
   | STM32F405xx | `stm32f405xg` | `0x00100000` (1 MB) |
   | STM32F407xx | `stm32f407xg` | `0x00100000` (1 MB) |
   | STM32F429xx | `stm32f429xi` | `0x00200000` (2 MB) |

3. Update `"configFiles"` in `launch.json` to point to the new cfg
4. Update `"svdFile"` — download the matching SVD from `posborne/cmsis-svd`
5. Update `"device"` field in `launch.json`

### Different debug probe (J-Link)

Change `servertype` in `launch.json` and replace the cfg:

```jsonc
"servertype": "jlink",
"device": "STM32F411VE",
"interface": "swd",
"serialNumber": ""   // optional — J-Link serial for multi-probe setups
```

Remove the `"configFiles"` field — Cortex-Debug configures J-Link directly.

### Different vendor (non-STM32)

1. Source the appropriate interface and target cfgs in a new `.cfg` file
2. Download the vendor SVD from `posborne/cmsis-svd`
3. Update `launch.json` fields accordingly
4. Add a new vendor HAL backend — see [BOARD.md](BOARD.md) for the board config system

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
