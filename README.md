# FreeRTOS-OS

A Linux-inspired OS layer on top of the FreeRTOS kernel for ARM
Cortex-M MCUs.  Adds a driver model, service threads, an interactive
shell, code generators (Kconfig + board XML), and a medical-grade
safety stack — all without replacing the FreeRTOS scheduler.

Architecture and rationale: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).
Per-vendor / per-MCU contract: [`arch/README.md`](arch/README.md).

---

## Supported Targets & Build Footprint

Footprint figures are for the standalone devboard examples under
[`examples/`](examples/) (no application code beyond `app_main.c`).
"OS static RAM" = `.data + .bss + ._user_heap_stack + .noinit` *minus*
the FreeRTOS heap pool — i.e. what FreeRTOS-OS itself plus enabled
service threads occupy before the application allocates anything.

| Target | Core | Clock | Flash | RAM | OS Flash | OS Static RAM | FreeRTOS Heap | TARGET_NAME | Status |
|---|---|---|---|---|---|---|---|---|---|
| **stm32f411** | Cortex-M4F (fpv4-sp-d16, hard) | 100 MHz | 512 KB | 128 KB | **47.8 KB**  (9.3 %) | **18.9 KB**  (14.8 %) | 64.0 KB | `stm32f411` | Supported |
| **stm32h723** | Cortex-M7  (fpv5-d16,  hard)   | 64 MHz HSI | 1 MB | 128 KB DTCM | **48.4 KB**  (4.7 %) | **19.1 KB**  (15.0 %) | 78.1 KB | `stm32h723` | Running, validated |

Footprint reproduction: `make dev-stm32f411 && arm-none-eabi-size build/stm32f411.elf`
(swap target name for the H7 row).

### Section breakdown (latest build)

| Section | F411 (bytes) | H723 (bytes) | Region |
|---|---|---|---|
| `.isr_vector`        |    408 |    672 | flash |
| `.text` + `.rodata` + ARM/init | 48,388 | 48,688 | flash |
| `.data` (init RAM)   |    180 |    184 | RAM (load from flash) |
| `.bss` (zero RAM)    | 83,136 | 97,804 | RAM — *includes the FreeRTOS heap pool (`ucHeap[]`)* |
| `._user_heap_stack`  |  1,540 |  1,540 | RAM (newlib heap + MSP) |
| `.noinit`            |     80 |     80 | RAM (`.noinit` fault + safe-state records) |
| **Total flash**      | **48,976 (47.8 KB)** | **49,544 (48.4 KB)** | |
| **Total RAM**        | **84,936 (82.9 KB)** | **99,608 (97.3 KB)** | |
| ↳ minus FreeRTOS heap | −65,536 | −80,000 | `CONFIG_RTOS_TOTAL_HEAP_SIZE` |
| **OS static RAM**    | **19,400 (18.9 KB)** | **19,608 (19.1 KB)** | OS + service-thread structures only |

---

## Use Cases

FreeRTOS-OS supports two build modes; both share the same OS tree.

| Use case | Builds from | Entry command | When to use |
|---|---|---|---|
| **Devboard example** (this repo only) | `examples/<target>/` | `make dev-<target>` | Smoke-test on a devboard, validate a port, run CI without an app. |
| **OS + Application** (sibling `app/`) | `../app/` | `make app TARGET=<target>` | Real product firmware.  FreeRTOS-OS is a git submodule of your application repo. |

---

## Use Case 1 — Devboard Example (Standalone)

The fastest path to a working OS + interactive shell on a NUCLEO-H723ZG
or STM32F411VET6 devboard.  No external `app/` is required.

### 1.1  Install prerequisites (once per machine)

```bash
make install-prerequisites
# Installs: arm-none-eabi-gcc, OpenOCD, kconfig-frontends, debug tools
```

Linux only — allow non-root USB access to ST-Link and `/dev/ttyUSB*`:

```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG dialout $USER          # log out / back in
```

### 1.2  Pick a devboard

```bash
make dev-stm32f411          # STM32F411 Devboard
# or
make dev-stm32h723          # NUCLEO-H723ZG
```

Each `make dev-<target>` invocation chains internally:

1. **IRQ table generation** — `scripts/gen_irq_table.py` reads
   `examples/<target>/board/irq_table.xml` and writes the seven
   `irq_*_generated.{c,h,inc}` files into the same directory.
2. **Board BSP generation** — `scripts/gen_board_config.py` reads
   `examples/<target>/board/<board>.xml` and writes
   `board_config.{c,h}`, `board_device_ids.h`, `board_handles.h`.
3. **Kconfig activation** — `examples/<target>/kconfig.conf` is copied
   to `.config` and `make config-outputs` regenerates `autoconf.{h,mk}`
   and `stm32{f4,h7}xx_hal_conf.h`.
4. **Clean** — old objects under `build/` are removed.
5. **Compile + link** — `make all APP_DIR=examples/<target>` produces
   `build/<target>.elf` plus `.hex`, `.asm`, `.sym`.

### 1.3  Flash the board

```bash
make dev-stm32f411-flash
# or
make dev-stm32h723-flash
```

### 1.4  Connect to the shell

```bash
# H723 NUCLEO — STLink VCP carries shell + printk together
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0

# F411 — shell is on USART2 via a USB-serial adapter, printk on USART1
screen /dev/ttyUSB0 115200
```

Within ~1 s of reset:

```
 [500] [heartbeat] tick 1
  +--------------------------------------------+
  |   ____              ___ _____ ___  ____    |
  |  |  __| ___ ___ ___|  _|_   _/   \/ ___|  |
  |  ...                                       |
  | CPU Clock: 64  MHz                         |
  | Kernel   : FreeRTOS V11.1.0+               |
  +--------------------------------------------+
  Type 'help' for available commands.
 OS >
```

Try: `help`, `ps`, `mem`, `log dump`.

### 1.5  Iterate on the example

If you change anything under `examples/<target>/`:

| Change | Re-run |
|---|---|
| `board/<board>.xml` (pin / UART assignment) | `make dev-<target>-gen` |
| `board/irq_table.xml` (priority / new IRQ) | `make dev-<target>-gen` |
| `kconfig.conf` (heap size, services, MCU flags) | `make dev-<target>` (full re-build picks it up) |
| `app_main.c` only | `make dev-<target>` |

`make dev-<target>-clean` removes only that devboard's generated outputs
plus `build/` — the *other* devboard's `board/` directory is untouched.

---

## Use Case 2 — OS + Application (Your Product)

FreeRTOS-OS is consumed as a git submodule inside your application
repository.  The OS tree is self-contained; your project supplies a
thin root `Makefile`, an `app/` directory, and per-target configuration
files.

### 2.1  Repository layout

```
my-firmware/
├── Makefile                  Top-level entry point (Step 2.3)
├── FreeRTOS-OS/              This repo, added as a submodule
└── app/
    ├── app_main.c            Your application entry point
    ├── Makefile              Application build fragment (Step 2.4)
    ├── kconfig_<target>.conf Kconfig preset per supported MCU
    └── board/
        ├── <board>.xml       Hardware description  (BSP generator input)
        ├── irq_table.xml     NVIC priorities       (IRQ generator input)
        └── (generated)       board_config.{c,h}, irq_*_generated.*
```

### 2.2  Add FreeRTOS-OS as a submodule

```bash
mkdir my-firmware && cd my-firmware
git init
git submodule add <freertos-os-url> FreeRTOS-OS
git submodule update --init --recursive
```

Cloning an existing project that already has the submodule:

```bash
git clone --recurse-submodules <my-firmware-url>
```

### 2.3  Top-level `Makefile`

```makefile
OS_DIR  := FreeRTOS-OS
APP_SRC := app

.PHONY: all app kernel flash clean menuconfig

all: app

app:
	$(MAKE) -C $(OS_DIR) app TARGET=stm32h723

kernel:
	$(MAKE) -C $(OS_DIR) os TARGET=stm32h723

flash:
	$(MAKE) -C $(OS_DIR) app-flash TARGET=stm32h723

menuconfig:
	$(MAKE) -C $(OS_DIR) menuconfig

clean:
	$(MAKE) -C $(OS_DIR) clean
```

### 2.4  Application build fragment — `app/Makefile`

```makefile
# Included by FreeRTOS-OS/Makefile when APP_DIR=../app is passed.
# One entry per .c source file in your application.

app-obj-y += app_main.o

# Generated BSP / IRQ outputs (recreated by `make app-gen`)
app-obj-y += board/board_config.o
app-obj-y += board/irq_hw_init_generated.o
app-obj-y += board/irq_periph_dispatch_generated.o

APP_INCLUDES += -I$(APP_DIR)
```

### 2.5  Write `app_main.c`

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

`printk()` is `printf`-style.  `printk_init()` / `printk_enable()` are
called by `uart_mgmt_thread` at boot — no manual init in `app_main`.

### 2.6  First-time build

`make app` chains: `app-gen` (board BSP) → copy `kconfig_<target>.conf`
→ `config-outputs` → `clean` → compile + link.

```bash
make app                # OS + app/ → FreeRTOS-OS/build/<app_name>.elf
make flash              # programs the ELF via OpenOCD
```

After the first build, day-to-day development is just `make app` /
`make flash`.

### 2.7  Iterate on application inputs

| Change | Re-run |
|---|---|
| Kconfig (MCU, heap size, UART count, …) | `make menuconfig` → `make config-outputs` |
| Board XML (pin / UART / peripheral assignment) | `make app-gen TARGET=<mcu>` |
| IRQ table XML (priority, new IRQ, EXTI) | `make irq_gen APP_DIR=../app` |
| DSP module XML (CMSIS-DSP modules) | `python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml` |
| `app_main.c` or any `.c` listed in `app/Makefile` | `make app` |

---

## Make Target Reference

Run from `FreeRTOS-OS/`.  `<t>` = `stm32f411` or `stm32h723`.
`TARGET=<t>` selects the MCU variant for the high-level aliases.

| Command | What it does | Output |
|---|---|---|
| **Devboard examples** | | |
| `make dev-stm32f411` | F411 example: full gen + config + clean + build | `build/stm32f411.elf` |
| `make dev-stm32f411-gen` | Regenerate F411 board + IRQ outputs only | `examples/stm32f411/board/*` |
| `make dev-stm32f411-clean` | Remove F411 generated outputs + `build/` | — |
| `make dev-stm32f411-flash` | Flash `build/stm32f411.elf` via OpenOCD | — |
| `make dev-stm32h723` | H723 example: full gen + config + clean + build | `build/stm32h723.elf` |
| `make dev-stm32h723-gen` | Regenerate H723 board + IRQ outputs only | `examples/stm32h723/board/*` |
| `make dev-stm32h723-clean` | Remove H723 generated outputs + `build/` | — |
| `make dev-stm32h723-flash` | Flash `build/stm32h723.elf` via OpenOCD | — |
| **High-level aliases (dispatch to dev-* by TARGET)** | | |
| `make os TARGET=<t>` | Equivalent to `make dev-<t>` | `build/<t>.elf` |
| `make os-flash TARGET=<t>` | Flash `build/<t>.elf` via OpenOCD | — |
| **OS + Application (sibling `../app/`)** | | |
| `make app TARGET=<t>` | Build OS + `../app/` for `<t>` | `build/<app_name>.elf` |
| `make app-gen TARGET=<t>` | Regenerate `../app/board/*` from XML | `../app/board/*` |
| `make app-flash TARGET=<t>` | Flash the application ELF | — |
| **Code generation primitives** | | |
| `make menuconfig` | Interactive Kconfig TUI | `.config` |
| `make config-outputs` | Generate autoconf + HAL conf from `.config` | `autoconf.{h,mk}`, `stm32{f4,h7}xx_hal_conf.h` |
| `make irq_gen APP_DIR=…` | IRQ table + dispatch + vector outputs | `irq/irq_table.c`, `<dir>/board/irq_*_generated.*` |
| **Static analysis** | | |
| `make cppcheck-board-gen` | Prep board headers (used by `run_cppcheck.sh`) | `examples/stm32f411/board/*` |
| `make clean-cppcheck-board` | Remove `build/cppcheck-board/` | — |
| **Documentation** | | |
| `make docs` | Run Doxygen | `docs/doxygen/html/` |
| `make clean-docs` | Remove generated docs | — |
| **Toolchain install (one-shot)** | | |
| `make install-prerequisites` | All tools below | — |
| `make install-toolchain` | `arm-none-eabi-gcc` + `gdb` | — |
| `make install-openocd` | OpenOCD | — |
| `make install-kconfig` | `kconfig-frontends` | — |
| `make install-debug-tools` | SVD + Cortex-Debug VS Code extension | — |
| `make install-doxygen` | Doxygen + Graphviz | — |
| **Housekeeping** | | |
| `make clean` | Remove `build/` only | — |
| `make print-target` | Print OpenOCD target script for active MCU | stdout |
| `make print-interface` | Print OpenOCD interface script for active MCU | stdout |

`TARGET_NAME=` (lowercase) is the internal Make variable that becomes
the output ELF name — `make dev-stm32f411` sets `TARGET_NAME=stm32f411`
which produces `build/stm32f411.elf`.  You rarely need to set it
manually; the table above shows the resolved value in the *Output*
column.

---

## Static Analysis

```bash
./scripts/install_cppcheck.sh

# F411 analysis
make dev-stm32f411-gen                                                  # board headers
bash scripts/run_cppcheck.sh --target=stm32f411 --severity=warning
bash scripts/run_cppcheck.sh --target=stm32f411 --misra
bash scripts/run_cppcheck.sh --target=stm32f411 --misra --severity=style --html

# H723 analysis (re-runs with the H7 HAL on the include path)
make dev-stm32h723-gen
bash scripts/run_cppcheck.sh --target=stm32h723 --severity=warning
bash scripts/run_cppcheck.sh --target=stm32h723 --misra
```

Reports land in `reports/cppcheck/<target>/` (git-ignored).  See
[`docs/CHECK.md`](docs/CHECK.md) for option flags and the MISRA
deviation table.

### CI Pipeline

GitHub Actions runs a 5-stage matrix pipeline for every supported
target on every push and pull request:

| Stage | Job | What it runs | Per-target |
|---|---|---|---|
| 1 | `1. generation`  | `make dev-<target>-gen` (verifies all 11 generator outputs) | yes |
| 2 | `2. cppcheck`    | `run_cppcheck.sh --target=<target> --severity=warning` | yes |
| 3 | `3. misra`       | `run_cppcheck.sh --target=<target> --misra` | yes |
| 4 | `4. build`       | `make dev-<target>` + footprint summary + ELF upload | yes |
| 5 | `5. doxygen`     | `doxygen Doxyfile` (project-wide) + HTML artifact upload | no  |

Stages run sequentially (each `needs:` the previous); targets within
a stage run in parallel.  All reports / ELFs / HTML are uploaded as
artifacts.  See [`.github/workflows/static_analysis.yml`](.github/workflows/static_analysis.yml).

---

## Documentation

| Document | Topic |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Full architecture reference — layer model, drivers, safety, boot FSM, invariants |
| [`arch/README.md`](arch/README.md) | `/arch` contract — adding a new MCU / vendor |
| [`examples/README.md`](examples/README.md) | Devboard example layout + how to add a new devboard |
| [`docs/OS_THREAD.md`](docs/OS_THREAD.md) | Thread API |
| [`docs/QUEUE.md`](docs/QUEUE.md) | IPC message queues + ring buffer |
| [`docs/DRIVERS.md`](docs/DRIVERS.md) | Driver vtables and HAL backends |
| [`docs/BOARD.md`](docs/BOARD.md) | Board XML schema, BSP generator |
| [`docs/DEV_MGMT.md`](docs/DEV_MGMT.md) | Service-thread internals |
| [`docs/SHELL_CLI.md`](docs/SHELL_CLI.md) | Shell, CLI, command registration |
| [`docs/IRQ.md`](docs/IRQ.md) | Linux-style IRQ subsystem |
| [`docs/DMA.md`](docs/DMA.md) | DMA engine + HAL backend |
| [`docs/DEBUG.md`](docs/DEBUG.md) | VS Code debug, OpenOCD, GDB, SVD viewer |
| [`docs/OS_INSIDE.md`](docs/OS_INSIDE.md) | Post-mortems and debug recipes |
| [`docs/CHECK.md`](docs/CHECK.md) | CPPcheck + MISRA C:2012 |
| [`docs/DSP.md`](docs/DSP.md) | CMSIS-DSP integration |

---

## License

GPL v3.0 — see [`LICENSE`](LICENSE) or
<https://www.gnu.org/licenses/gpl-3.0.html>.

Submodule licenses: FreeRTOS-Kernel (MIT) · STM32 HAL (BSD 3-Clause) ·
CMSIS-6 (Apache 2.0) · CMSIS-DSP (Apache 2.0) · CANopen stack
(Apache 2.0) · ringbuffer (GPL v2).

---

*Author: Subhajit Roy — <subhajitroy005@gmail.com>*
