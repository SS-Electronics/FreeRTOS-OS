<div align="center">

# FreeRTOS-OS

**A Linux-inspired OS layer for ARM Cortex-M, built on the FreeRTOS kernel.**

[![CI Pipeline](https://github.com/SS-Electronics/FreeRTOS-OS/actions/workflows/static_analysis.yml/badge.svg)](https://github.com/SS-Electronics/FreeRTOS-OS/actions/workflows/static_analysis.yml)
[![License: GPL-3.0](https://img.shields.io/github/license/SS-Electronics/FreeRTOS-OS?color=blue)](LICENSE)
[![Latest Release](https://img.shields.io/github/v/release/SS-Electronics/FreeRTOS-OS?display_name=tag&include_prereleases)](https://github.com/SS-Electronics/FreeRTOS-OS/releases)
[![Issues](https://img.shields.io/github/issues/SS-Electronics/FreeRTOS-OS)](https://github.com/SS-Electronics/FreeRTOS-OS/issues)
[![Last commit](https://img.shields.io/github/last-commit/SS-Electronics/FreeRTOS-OS)](https://github.com/SS-Electronics/FreeRTOS-OS/commits)

[![Language](https://img.shields.io/badge/language-C99-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/platform-ARM%20Cortex--M-orange?logo=arm&logoColor=white)](https://developer.arm.com/Architectures/Cortex-M)
[![MCU](https://img.shields.io/badge/MCU-STM32F4%20%7C%20STM32H7-03234B?logo=stmicroelectronics&logoColor=white)](docs/ARCHITECTURE.md)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-V11.1%2B-2E9F4D)](https://www.freertos.org/)
[![CPPcheck](https://img.shields.io/badge/CPPcheck-clean-success)](.github/workflows/static_analysis.yml)
[![MISRA](https://img.shields.io/badge/MISRA-C%3A2012-7E57C2)](docs/CHECK.md)
[![IEC&nbsp;62304](https://img.shields.io/badge/IEC%2062304-safety%20layer-blueviolet)](docs/ARCHITECTURE.md#medical-grade-safety-layer)
[![Doxygen](https://img.shields.io/badge/docs-Doxygen-2E9F4D?logo=doxygen&logoColor=white)](docs/ARCHITECTURE.md)

</div>

---

FreeRTOS-OS adds the things the bare FreeRTOS kernel deliberately leaves
out — a Linux-style driver model, service threads, an interactive
shell, code generators (Kconfig + board XML → BSP + IRQ tables), and a
medical-grade safety stack — **without replacing the FreeRTOS
scheduler**.  It targets ARM Cortex-M MCUs, ships production-ready
support for STM32F4 and STM32H7, and is structured so a new vendor or
family slots in cleanly under `arch/devices/<VENDOR>/<FAMILY>/<PART>/`.

> **Documentation:**  [Architecture deep-dive](docs/ARCHITECTURE.md) ·
> [Per-vendor `/arch` contract](arch/README.md) ·
> [Per-devboard examples](examples/README.md)

---

## ✨ Highlights

| | |
|---|---|
| 🐧 **Linux-inspired API**            | `os_thread_create()`, `request_irq()`, intrusive `list_head`, `kmalloc/kfree` — familiar mental model, embedded footprint. |
| 🔌 **Vendor-agnostic driver model**  | Generic `drv_*` vtables, vendor HALs under `drivers/hal/<vendor>/`.  Drop in a new MCU without touching the application. |
| 🧵 **Service-thread architecture**   | UART, I²C, SPI, GPIO, ADC peripherals are each owned by a FreeRTOS task — no ad-hoc concurrency in app code. |
| 🐚 **Interactive shell**             | FreeRTOS-Plus-CLI on UART, ready to register custom commands.  Built-in `ps`, `mem`, `log dump`, `reset`. |
| 🛡️ **Medical-grade safety stack**   | IWDG + per-task software watchdog + `.noinit` fault record + safe-state shutdown — designed against IEC 62304 / 60601-1. |
| 🏭 **Code-gen pipeline**             | One XML per board → BSP, IRQ tables, NVIC init.  One XML per DSP module set → CMSIS-DSP build slice. |
| 🧪 **Per-target CI**                 | 5-stage pipeline (gen → cppcheck → MISRA → build → Doxygen) runs for every supported target on every push. |
| 📦 **Tiny footprint**                | ~48 KB flash + ~19 KB OS static RAM regardless of MCU. |

---

## 🎯 Supported Targets & Build Footprint

Footprint figures below are for the standalone devboard examples under
[`examples/`](examples/) (no application code beyond `app_main.c`).
*OS static RAM* = `.data + .bss + heap_stack + .noinit` minus the
FreeRTOS heap pool — i.e. what the OS itself plus enabled service
threads occupy before your application allocates anything.

| Target | Core | Clock | Flash | RAM | OS Flash | OS Static RAM | FreeRTOS Heap | `TARGET_NAME` | Status |
|---|---|---|---|---|---|---|---|---|---|
| **stm32f411** | Cortex-M4F (`fpv4-sp-d16`, hard) | 100 MHz | 512 KB | 128 KB | **47.8 KB** *(9.3 %)* | **18.9 KB** *(14.8 %)* | 64.0 KB | `stm32f411` | ✅ Supported |
| **stm32h723** | Cortex-M7  (`fpv5-d16`,  hard) | 64 MHz HSI | 1 MB | 128 KB DTCM | **48.4 KB** *(4.7 %)* | **19.1 KB** *(15.0 %)* | 78.1 KB | `stm32h723` | ✅ Running, validated |

Reproduce: `make dev-stm32f411 && arm-none-eabi-size build/stm32f411.elf`

<details>
<summary>Section-by-section breakdown (latest build)</summary>

| Section | F411 (bytes) | H723 (bytes) | Region |
|---|---|---|---|
| `.isr_vector`        |    408 |    672 | flash |
| `.text` + `.rodata` + ARM/init | 48,388 | 48,688 | flash |
| `.data` (init RAM)   |    180 |    184 | RAM (loaded from flash) |
| `.bss` (zero RAM)    | 83,136 | 97,804 | RAM — *includes the FreeRTOS heap pool (`ucHeap[]`)* |
| `._user_heap_stack`  |  1,540 |  1,540 | RAM (newlib heap + MSP) |
| `.noinit`            |     80 |     80 | RAM (`.noinit` fault + safe-state records) |
| **Total flash**      | **48,976 (47.8 KB)** | **49,544 (48.4 KB)** | |
| **Total RAM**        | **84,936 (82.9 KB)** | **99,608 (97.3 KB)** | |
| ↳ minus FreeRTOS heap | −65,536 | −80,000 | `CONFIG_RTOS_TOTAL_HEAP_SIZE` |
| **OS static RAM**    | **19,400 (18.9 KB)** | **19,608 (19.1 KB)** | OS + service-thread structures only |
</details>

---

## 🚀 Quick Start (5 minutes, no application code needed)

```bash
git clone --recurse-submodules https://github.com/SS-Electronics/FreeRTOS-OS.git
cd FreeRTOS-OS

# 1. Install toolchain + OpenOCD + kconfig-frontends (once per machine)
make install-prerequisites

# 2. Build + flash a devboard example
make dev-stm32f411          # or  make dev-stm32h723
make dev-stm32f411-flash    #      make dev-stm32h723-flash

# 3. Open the shell
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0   # H723
# screen /dev/ttyUSB0 115200                                   # F411
```

Within ~1 s of reset you should see the banner:

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

---

## 🛠 Use Cases

| Mode | Builds from | Entry point | Best for |
|---|---|---|---|
| **Devboard example** | `examples/<target>/` | `make dev-<target>` | Port validation, CI, evaluating the OS without writing app code. |
| **OS + Application** | sibling `../app/` | `make app TARGET=<target>` | Real product firmware.  FreeRTOS-OS is a git submodule of your application repo. |

### Mode 1 — Devboard Example (Standalone)

1. **Install prerequisites** *(once)*
   ```bash
   make install-prerequisites
   # Linux only — non-root USB access to ST-Link and /dev/ttyUSB*:
   sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules && sudo udevadm trigger
   sudo usermod -aG dialout $USER       # log out / back in
   ```
2. **Build a devboard** — one command per target
   ```bash
   make dev-stm32f411        # STM32F411 Devboard
   make dev-stm32h723        # NUCLEO-H723ZG
   ```
   Each invocation chains 5 internal sub-steps: IRQ gen → BSP gen →
   Kconfig activation → clean → compile + link.  Outputs land in
   `build/<target>.elf` (+ `.hex`, `.asm`, `.sym`).
3. **Flash:** `make dev-stm32f411-flash` / `make dev-stm32h723-flash`
4. **Connect:** `stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0`
5. **Iterate** — only re-running what changed:

   | Change | Re-run |
   |---|---|
   | `board/<board>.xml` (pin / UART assignment) | `make dev-<target>-gen` |
   | `board/irq_table.xml` (priority / new IRQ) | `make dev-<target>-gen` |
   | `kconfig.conf` (heap, services, MCU flags) | `make dev-<target>` |
   | `app_main.c` only | `make dev-<target>` |

   `make dev-<target>-clean` only removes that devboard's generated
   outputs plus `build/`; the *other* devboard's `board/` is untouched.

### Mode 2 — OS + Application (Your Product)

FreeRTOS-OS is consumed as a **git submodule** of your application
repository.  Your project supplies a thin root `Makefile`, an `app/`
directory, and per-target configuration.

1. **Layout**
   ```
   my-firmware/
   ├── Makefile                  (Step 3)
   ├── FreeRTOS-OS/              (this repo, added as a submodule)
   └── app/
       ├── app_main.c            your entry point
       ├── Makefile              (Step 4)
       ├── kconfig_<target>.conf Kconfig preset per supported MCU
       └── board/
           ├── <board>.xml       hardware description
           ├── irq_table.xml     NVIC priorities
           └── (generated)       board_config.{c,h}, irq_*_generated.*
   ```
2. **Add the submodule**
   ```bash
   git submodule add https://github.com/SS-Electronics/FreeRTOS-OS.git FreeRTOS-OS
   git submodule update --init --recursive
   ```
3. **Top-level `Makefile`**
   ```makefile
   OS_DIR := FreeRTOS-OS

   .PHONY: app flash menuconfig clean
   app:        ; $(MAKE) -C $(OS_DIR) app       TARGET=stm32h723
   flash:      ; $(MAKE) -C $(OS_DIR) app-flash TARGET=stm32h723
   menuconfig: ; $(MAKE) -C $(OS_DIR) menuconfig
   clean:      ; $(MAKE) -C $(OS_DIR) clean
   ```
4. **`app/Makefile` fragment**
   ```makefile
   # One entry per .c file in your application
   app-obj-y += app_main.o

   # Generated BSP / IRQ outputs (recreated by `make app-gen`)
   app-obj-y += board/board_config.o
   app-obj-y += board/irq_hw_init_generated.o
   app-obj-y += board/irq_periph_dispatch_generated.o

   APP_INCLUDES += -I$(APP_DIR)
   ```
5. **`app/app_main.c`**
   ```c
   #include <os/kernel.h>
   #include <services/gpio_mgmt.h>
   #include <board/board_device_ids.h>

   static void heartbeat(void *p)
   {
       (void)p;
       for (uint32_t t = 0;; t++) {
           gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
           printk("[heartbeat] tick %lu\n", (unsigned long)t);
           os_thread_delay(500);
       }
   }

   int app_main(void)
   {
       os_thread_create(heartbeat, "heartbeat", 256, 1, NULL);
       return 0;
   }
   ```
6. **First-time build:** `make app` (chains `app-gen` → `config-outputs` → clean → compile).
7. **Iterate**:

   | Change | Re-run |
   |---|---|
   | Kconfig (MCU, heap size, …) | `make menuconfig` → `make config-outputs` |
   | Board XML | `make app-gen TARGET=<mcu>` |
   | IRQ table XML | `make irq_gen APP_DIR=../app` |
   | DSP XML | `python3 FreeRTOS-OS/scripts/arm_dsp_gen.py app/board/dsp_dev.xml` |
   | `app_main.c` / any `.c` | `make app` |

---

## 🧭 Architecture

```
┌──────────────────────────────────────────────────────────┐
│         Your Application  (app_main, app/*.c)            │
├──────────────────────────────────────────────────────────┤
│  Service Threads   ·   Interactive Shell  ·   IPC API    │
├──────────────────────────────────────────────────────────┤
│  Driver Layer  (drv_* vtables)  +  HAL Backend           │
├──────────────────────────────────────────────────────────┤
│      Memory  ·  IRQ  ·  Safety  ·  Logging  ·  Boot FSM  │
├──────────────────────────────────────────────────────────┤
│                 FreeRTOS Kernel                          │
├──────────────────────────────────────────────────────────┤
│  arch/devices/<VENDOR>/<FAMILY>/<PART>/  +  CMSIS-Core   │
└──────────────────────────────────────────────────────────┘
```

Read the full layer model, boot FSM, safety design, and critical
invariants in [**docs/ARCHITECTURE.md**](docs/ARCHITECTURE.md).  Add a
new MCU vendor by following the contract in
[**arch/README.md**](arch/README.md).

---

## ⚙️ Make Target Reference

Run all targets from `FreeRTOS-OS/`.  `<t>` = `stm32f411` or `stm32h723`.

<details>
<summary><b>Devboard examples</b> (no sibling <code>app/</code> required)</summary>

| Command | Output |
|---|---|
| `make dev-stm32f411`        | `build/stm32f411.elf` (full gen + build) |
| `make dev-stm32f411-gen`    | Regenerate F411 board + IRQ outputs only |
| `make dev-stm32f411-clean`  | Remove F411 generated outputs + `build/` |
| `make dev-stm32f411-flash`  | Flash via OpenOCD |
| `make dev-stm32h723`        | `build/stm32h723.elf` |
| `make dev-stm32h723-gen`    | Regenerate H723 board + IRQ outputs only |
| `make dev-stm32h723-clean`  | Remove H723 generated outputs + `build/` |
| `make dev-stm32h723-flash`  | Flash via OpenOCD |
</details>

<details>
<summary><b>High-level aliases</b> (dispatch to <code>dev-*</code> by TARGET)</summary>

| Command | Equivalent to |
|---|---|
| `make os TARGET=<t>`       | `make dev-<t>` |
| `make os-flash TARGET=<t>` | `make dev-<t>-flash` |
</details>

<details>
<summary><b>OS + Application</b> (sibling <code>../app/</code>)</summary>

| Command | Effect |
|---|---|
| `make app TARGET=<t>`      | Build OS + `../app/` |
| `make app-gen TARGET=<t>`  | Regenerate `../app/board/*` from XML |
| `make app-flash TARGET=<t>`| Flash the application ELF |
</details>

<details>
<summary><b>Code-gen primitives</b></summary>

| Command | Output |
|---|---|
| `make menuconfig`            | Interactive Kconfig TUI |
| `make config-outputs`        | `autoconf.{h,mk}`, `stm32{f4,h7}xx_hal_conf.h` |
| `make irq_gen APP_DIR=…`     | `irq/irq_table.c` + `<dir>/board/irq_*_generated.*` |
| `make cppcheck-board-gen`    | Prep F411 board headers for `run_cppcheck.sh` |
</details>

<details>
<summary><b>Toolchain install &amp; housekeeping</b></summary>

| Command | Installs / Effect |
|---|---|
| `make install-prerequisites` | All tools below in one shot |
| `make install-toolchain`     | `arm-none-eabi-gcc` + `gdb` |
| `make install-openocd`       | OpenOCD |
| `make install-kconfig`       | `kconfig-frontends` |
| `make install-debug-tools`   | SVD + Cortex-Debug VS Code extension |
| `make install-doxygen`       | Doxygen + Graphviz |
| `make docs`                  | Generate Doxygen HTML → `docs/doxygen/html/` |
| `make clean`                 | Remove `build/` |
| `make print-target`          | Print OpenOCD target script for active MCU |
</details>

---

## 🔬 Static Analysis

```bash
./scripts/install_cppcheck.sh

# Per-target — F411
make dev-stm32f411-gen                                           # board headers
bash scripts/run_cppcheck.sh --target=stm32f411 --severity=warning
bash scripts/run_cppcheck.sh --target=stm32f411 --misra

# Per-target — H723
make dev-stm32h723-gen
bash scripts/run_cppcheck.sh --target=stm32h723 --severity=warning
bash scripts/run_cppcheck.sh --target=stm32h723 --misra
```

Reports land in `reports/cppcheck/<target>/` (git-ignored).
See [`docs/CHECK.md`](docs/CHECK.md) for option flags and the MISRA
deviation table.

---

## 🤖 CI Pipeline

Every push and pull request runs a 5-stage matrix pipeline against
every supported target:

| Stage | Job ID | What it does | Per-target |
|---|---|---|---|
| 1 | `1. generation` | `make dev-<target>-gen` + verifies all 11 generator outputs | ✓ |
| 2 | `2. cppcheck`   | `run_cppcheck.sh --target=<target> --severity=warning` | ✓ |
| 3 | `3. misra`      | `run_cppcheck.sh --target=<target> --misra` | ✓ |
| 4 | `4. build`      | `make dev-<target>` + footprint summary + ELF upload | ✓ |
| 5 | `5. doxygen`    | `doxygen Doxyfile` + HTML artifact upload | (single) |

Stages run sequentially (`needs:` previous); both targets in a stage
run in parallel.  Reports, ELFs, and Doxygen HTML are uploaded as
artifacts.  See [`.github/workflows/static_analysis.yml`](.github/workflows/static_analysis.yml).

---

## 📚 Documentation

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

## 🤝 Contributing

Bug reports, ports to new MCUs, and pull requests are welcome.

  - **Ports to a new MCU vendor** → follow the worked LPC55S69 example
    in [`arch/README.md`](arch/README.md).
  - **New devboard examples** → copy an existing tree under
    [`examples/`](examples/) and add a `make dev-<target>` rule; see
    [`examples/README.md`](examples/README.md).
  - **Code style** → Allman braces, 4-space indent, Doxygen file
    headers (see [`scripts/apply_doxygen_header.py`](scripts/apply_doxygen_header.py)).
  - **CI must stay green** — the workflow runs cppcheck (`warning`
    severity = 0 errors expected) and MISRA C:2012 on every push.

Open an issue at <https://github.com/SS-Electronics/FreeRTOS-OS/issues>
before starting a large change so we can scope it together.

---

## 📄 License

FreeRTOS-OS is licensed under **GPL v3.0**.  See [`LICENSE`](LICENSE) or
<https://www.gnu.org/licenses/gpl-3.0.html>.

Bundled submodule licenses (each kept intact under its own directory):

| Component | License |
|---|---|
| FreeRTOS-Kernel               | MIT          |
| STMicroelectronics STM32 HAL  | BSD 3-Clause |
| ARM CMSIS-6                   | Apache 2.0   |
| ARM CMSIS-DSP                 | Apache 2.0   |
| ARM CMSIS-NN                  | Apache 2.0   |
| CANopen stack                 | Apache 2.0   |
| ringbuffer                    | GPL v2       |

---

<div align="center">

**Maintainer:** [Subhajit Roy](mailto:subhajitroy005@gmail.com) · `subhajitroy005@gmail.com`

⭐ If FreeRTOS-OS is useful in your project, please star the repo — it
helps prioritise upstream work.

</div>
