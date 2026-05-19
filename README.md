# FreeRTOS-OS

A Linux-inspired OS layer on top of the FreeRTOS kernel for ARM
Cortex-M MCUs.  Adds a driver model, service threads, an interactive
shell, code generators (Kconfig + board XML), and a medical-grade
safety stack — all without replacing the FreeRTOS scheduler.

| Target | Core | Clock | Flash | RAM | Status |
|---|---|---|---|---|---|
| STM32F411VET6 | Cortex-M4F | 100 MHz | 512 KB | 128 KB | Supported |
| **STM32H723ZGTx** | **Cortex-M7** | **64 MHz HSI** | **1 MB** | **128 KB DTCM** | **Running, validated** |

Architecture and rationale: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).
Architecture / vendor contract: [`arch/README.md`](arch/README.md).

---

## Use cases

FreeRTOS-OS supports two build modes; both share the same OS tree.

| Use case | Command | When to use |
|---|---|---|
| **Standalone OS** (this repo only) | `make os TARGET=stm32f411` / `make os TARGET=stm32h723` | Smoke-test the OS on a devboard, validate a port, run CI without an app. |
| **OS + Application** (sibling `app/`) | `make app TARGET=stm32h723` | Real product firmware.  `FreeRTOS-OS` is a git submodule inside your application repo. |

---

## Use case 1 — Standalone OS on a devboard

Everything you need to flash a working OS + interactive shell onto a
NUCLEO-H723ZG or an STM32F411VET6 dev-board.

### 1.1  Install prerequisites (once per machine)

```bash
make install-prerequisites
# Installs: arm-none-eabi-gcc, OpenOCD, kconfig-frontends, debug tools
```

### 1.2  Allow non-root USB access to the ST-Link probe (Linux)

```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG dialout $USER          # also needed for /dev/ttyUSB0
# Log out / back in after the usermod
```

### 1.3  Build & flash

```bash
# F411
make os TARGET=stm32f411
make os-flash TARGET=stm32f411

# or H723
make os TARGET=stm32h723
make os-flash TARGET=stm32h723
```

Each `make os` invocation chains *board-gen → config-outputs → clean →
compile* internally.  No manual `menuconfig` step is required for the
shipped demo configurations.

### 1.4  Connect to the shell

```bash
# H723 NUCLEO — STLink VCP carries shell + printk together
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0

# F411 — shell is on USART2 via a USB-serial adapter, printk on USART1
screen /dev/ttyUSB0 115200
```

You should see, within ~1 s of reset:

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

Type `help`, `ps`, `mem`, `log dump` to explore.

---

## Use case 2 — OS + Application (your own product)

`FreeRTOS-OS` is consumed as a git submodule inside your application
repository.  The OS build system is self-contained; your project only
needs a thin root `Makefile`, an `app/` directory, and per-target
configuration files.

### 2.1  Repository layout

```
my-firmware/
├── Makefile                  Top-level entry point (see 2.3 below)
├── FreeRTOS-OS/              This repo, added as a submodule
└── app/
    ├── app_main.c            Your application entry point
    ├── Makefile              Application build fragment (see 2.4)
    ├── kconfig_<target>.conf Preset Kconfig for each MCU you support
    └── board/
        ├── <board>.xml       Hardware description (BSP generator input)
        ├── irq_table.xml     NVIC priorities (IRQ generator input)
        └── (generated)       board_config.c, irq_*_generated.*, …
```

### 2.2  Add FreeRTOS-OS as a submodule

```bash
mkdir my-firmware && cd my-firmware
git init
git submodule add <freertos-os-url> FreeRTOS-OS
git submodule update --init --recursive
```

Cloning an existing project with the submodule:

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

### 2.4  Application build fragment `app/Makefile`

```makefile
# Included by FreeRTOS-OS/Makefile when APP_DIR=../app is passed.
# One entry per .c source file in your application.

app-obj-y += app_main.o

# Generated BSP / IRQ outputs
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

`printk()` is `printf`-style.  `printk_init()` and `printk_enable()`
are called automatically by `uart_mgmt_thread` at boot — no manual
init needed in `app_main`.

### 2.6  Build & flash

```bash
make app           # builds OS + app/ → FreeRTOS-OS/build/<target>.elf
make flash         # programs the ELF via OpenOCD
```

After the first build, day-to-day development is just `make app` /
`make flash`.

### 2.7  Regenerate after changing inputs

| Change | Command(s) |
|---|---|
| Kconfig (MCU, heap size, UART count, …) | `make menuconfig` → `make config-outputs` |
| Board XML (pin / UART / peripheral assignment) | `make app-gen TARGET=<mcu>` |
| IRQ table XML (priority, new IRQ, EXTI) | `make irq_gen APP_DIR=../app` |
| DSP module XML (CMSIS-DSP modules) | `python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml` |

---

## Prerequisites

| Tool | Required? | Install |
|---|---|---|
| `arm-none-eabi-gcc` + `gdb` | Yes | `make install-toolchain` |
| OpenOCD | Yes (for flash + debug) | `make install-openocd` |
| `kconfig-frontends` | Only when running `make menuconfig` | `make install-kconfig` |
| SVD + Cortex-Debug VS Code extension | Only for IDE debugging | `make install-debug-tools` |
| Doxygen + Graphviz | Only for `make docs` | `make install-doxygen` |

All install scripts live under `scripts/install_*.sh` and are wrapped
by `make install-*` targets.

---

## Make target reference

All commands run from `FreeRTOS-OS/`.  `TARGET` selects the MCU variant
(`stm32f411` or `stm32h723`).

### Standalone OS

| Command | Output |
|---|---|
| `make os TARGET=<t>` | `build/<t>.elf` |
| `make os-flash TARGET=<t>` | Flashes the standalone build via OpenOCD |

### With-application

| Command | Output |
|---|---|
| `make app TARGET=<t>` | `build/<app_name>.elf` (name from app's Kconfig preset) |
| `make app-flash TARGET=<t>` | Flashes the app build via OpenOCD |
| `make app-gen TARGET=<t>` | Regenerates board BSP from `app/board/*.xml` |

### Code generation

| Command | Output |
|---|---|
| `make menuconfig` | Interactive Kconfig TUI |
| `make config-outputs` | `autoconf.h`, `autoconf.mk`, `stm32{f4,h7}xx_hal_conf.h` |
| `make irq_gen APP_DIR=…` | `irq_table.c`, `irq_*_generated.{c,h,inc}` |
| `make demo-gen` / `demo-gen-h723` | Regenerate demo board headers (for CPPcheck and the standalone build) |

### Utilities

| Command | Effect |
|---|---|
| `make docs` | Generates Doxygen HTML under `docs/doxygen/html/` |
| `make clean` | Removes `build/` and generated board files |
| `make print-target` | Shows the OpenOCD target file for the active MCU |

---

## Static analysis

```bash
./scripts/install_cppcheck.sh
make demo-gen                              # required for include resolution
bash scripts/run_cppcheck.sh --severity=warning
bash scripts/run_cppcheck.sh --misra
bash scripts/run_cppcheck.sh --misra --severity=style --html
```

GitHub Actions runs CPPcheck on every push (job: `cppcheck`).  Reports
land in `reports/cppcheck/` (git-ignored).  See [`docs/CHECK.md`](docs/CHECK.md)
for option flags and the MISRA deviation table.

---

## Documentation

| Document | Topic |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Full architecture reference — layer model, drivers, safety, boot FSM, invariants |
| [`arch/README.md`](arch/README.md) | `/arch` contract — adding a new MCU / vendor |
| [`docs/OS_THREAD.md`](docs/OS_THREAD.md) | Thread API, `os_thread_create()` usage |
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
