<div align="center">

# FreeRTOS-OS {#freertos-os}

**A Linux-inspired OS layer for ARM Cortex-M, built on the FreeRTOS kernel.**

[![CI Pipeline](https://github.com/SS-Electronics/FreeRTOS-OS/actions/workflows/static_analysis.yml/badge.svg)](https://github.com/SS-Electronics/FreeRTOS-OS/actions/workflows/static_analysis.yml)
[![License: GPL-3.0](https://img.shields.io/github/license/SS-Electronics/FreeRTOS-OS?color=blue)](LICENSE)
[![Latest Release](https://img.shields.io/github/v/release/SS-Electronics/FreeRTOS-OS?display_name=tag&include_prereleases)](https://github.com/SS-Electronics/FreeRTOS-OS/releases)
[![Issues](https://img.shields.io/github/issues/SS-Electronics/FreeRTOS-OS)](https://github.com/SS-Electronics/FreeRTOS-OS/issues)
[![Last commit](https://img.shields.io/github/last-commit/SS-Electronics/FreeRTOS-OS)](https://github.com/SS-Electronics/FreeRTOS-OS/commits)

[![Language](https://img.shields.io/badge/language-C99-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/platform-ARM%20Cortex--M-orange?logo=arm&logoColor=white)](https://developer.arm.com/Architectures/Cortex-M)
[![MCU](https://img.shields.io/badge/MCU-STM32F4%20%7C%20STM32H7%20%7C%20STM32U5-03234B?logo=stmicroelectronics&logoColor=white)](docs/ARCHITECTURE.md)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-V11.1%2B-2E9F4D)](https://www.freertos.org/)
[![CPPcheck](https://img.shields.io/badge/CPPcheck-clean-success)](.github/workflows/static_analysis.yml)
[![MISRA](https://img.shields.io/badge/MISRA-C%3A2012-7E57C2)](docs/CHECK.md)
[![IEC&nbsp;62304](https://img.shields.io/badge/IEC%2062304-safety%20layer-blueviolet)](docs/ARCHITECTURE.md#medical-grade-safety-layer)
[![Doxygen](https://img.shields.io/badge/docs-Doxygen-2E9F4D?logo=doxygen&logoColor=white)](docs/ARCHITECTURE.md)

</div>

---

FreeRTOS-OS adds the things on top of FreeRTOS kernel API,
a Linux-style driver model, service threads, an interactive
shell, code generators (Kconfig + board XML → BSP + IRQ tables), and a
medical-grade safety stack.

> **Documentation:**  [Architecture deep-dive](docs/ARCHITECTURE.md) ·
> [Per-vendor `/arch` contract](arch/README.md) ·
> [Per-devboard examples](examples/README.md)

<hr/>

## ✨ Highlights {#highlights}

| | |
|---|---|
| 🐧 **Linux-inspired API**            | `os_thread_create()`, `request_irq()`, intrusive `list_head`, `kmalloc/kfree` — familiar mental model, embedded footprint. |
| 🔌 **Vendor-agnostic driver model**  | Generic `drv_*` vtables, vendor HALs under `drivers/hal/<vendor>/`.  Drop in a new MCU without touching the application. |
| 🧵 **Service-thread architecture**   | UART, I²C, SPI, GPIO, ADC peripherals are each owned by a FreeRTOS task — no ad-hoc concurrency in app code. |
| 🐚 **Interactive shell**             | FreeRTOS-Plus-CLI on UART, ready to register custom commands.  Built-in `ps`, `mem`, `log dump`, `reset`. |
| 🛡️ **Medical-grade safety stack**   | IWDG + per-task software watchdog + `.noinit` fault record + safe-state shutdown — designed against IEC 62304 / 60601-1. |
| 🏭 **Code-gen pipeline**             | One XML per board → BSP, IRQ tables, NVIC init.  One XML per DSP module set → CMSIS-DSP build slice. |
| 🧪 **Per-target CI**                 | 5-stage pipeline (gen → cppcheck → MISRA → build → Doxygen) runs for every supported target on every push. |
| 📦 **Tiny footprint**                | ~48 KB flash + ~19 KB OS static RAM regardless of MCU. `-Os`, `--gc-sections`, no LTO. |

---

## 🎯 Supported Targets & Build Footprint {#supported-targets--build-footprint}

Footprint figures below are for the standalone devboard examples under
[`examples/`](examples/) (no application code beyond `app_main.c`).
*OS static RAM* = `.data + .bss + heap_stack + .noinit` minus the
FreeRTOS heap pool — i.e. what the OS itself plus enabled service
threads occupy before your application allocates anything.

| Target | Core | Clock | Flash | RAM | OS Flash | OS Static RAM | FreeRTOS Heap | `TARGET_NAME` | Status |
|---|---|---|---|---|---|---|---|---|---|
| **stm32f411** | Cortex-M4F  (`fpv4-sp-d16`, hard) | 100 MHz | 512 KB | 128 KB | **47.8 KB** *(9.3 %)* | **18.9 KB** *(14.8 %)* | 64.0 KB | `stm32f411` | ✅ Supported |
| **stm32h723** | Cortex-M7   (`fpv5-d16`,    hard) | 64 MHz HSI | 1 MB | 128 KB DTCM | **48.1 KB** *(4.7 %)* | **19.1 KB** *(15.0 %)* | 78.1 KB | `stm32h723` | ✅ Running, validated |
| **stm32u575** | Cortex-M33 + TrustZone (`fpv5-sp-d16`, hard) | 4 MHz MSI (PLL hooked, not yet enabled) | 2 MB | 192 KB SRAM1 + 64K SRAM2 + 512K SRAM3 | **50.4 KB** *(2.5 %)* | **20.5 KB** *(10.7 % of SRAM1)* | 80.0 KB | `stm32u575` | 🧪 Builds, trustcore + heartbeat |

> **Build modes:** all figures above are `-Os` release size.  Pass `DEBUG=1` (the default)
> to embed `-g3` DWARF info — it adds debug sections to the ELF but does **not** change the
> flash footprint.  Pass `DEBUG=0` to strip debug sections and define `NDEBUG`.
> LTO is intentionally omitted — on arm-none-eabi-gcc 13 it adds ~7 KB due to aggressive
> variable promotion; dead-code elimination relies on `-ffunction-sections` + `--gc-sections`.

---

## 🚀 Quick Start (5 minutes, no application code needed) {#quick-start-5-minutes-no-application-code-needed}

```bash
git clone --recurse-submodules https://github.com/SS-Electronics/FreeRTOS-OS.git
cd FreeRTOS-OS

# 1. Install toolchain + OpenOCD + kconfig-frontends (once per machine)
make install-prerequisites

# 2. Build + flash a devboard example
make dev-stm32h723          # or  make dev-stm32f411 / make dev-stm32u575
make dev-stm32h723-flash    #      make dev-stm32f411-flash / make dev-stm32u575-flash

# 3. Open the shell
stty -F /dev/ttyACM0 115200 raw -echo && cat /dev/ttyACM0   # H723 / U575
# screen /dev/ttyUSB0 115200                                   # F411
```

**Starting your own application project** (creates `../app/` alongside this repo):

```bash
# Scaffold from a board example — copies board XML, kconfig, and a working app_main
make setup-project EXAMPLE=stm32h723

# Or start with a blank project and pick a board later
make setup-project

# Build from the scaffolded project
make dev-stm32h723 APP_DIR=../app
make dev-stm32h723-flash
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

## 🛠 Use Cases {#use-cases}

| Mode | Builds from | Entry point | Best for |
|---|---|---|---|
| **Devboard example** | `examples/<target>/` | `make dev-<target>` | Port validation, CI, evaluating the OS without writing app code. |
| **OS + Application** | sibling `../app/` | `make setup-project` then `make dev-<target> APP_DIR=../app` | Real product firmware.  FreeRTOS-OS is a git submodule of your application repo. |

### Mode 1 — Devboard Example (Standalone) {#mode-1--devboard-example-standalone}

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
   make dev-stm32u575        # NUCLEO-U575ZI-Q  (Cortex-M33 + TrustZone)
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

### Mode 2 — OS + Application (Your Product) {#mode-2--os--application-your-product}

FreeRTOS-OS is consumed as a **git submodule** of your application
repository.  Use `make setup-project` to scaffold the `app/` directory
in one command.

1. **Add the submodule**
   ```bash
   git submodule add https://github.com/SS-Electronics/FreeRTOS-OS.git FreeRTOS-OS
   git submodule update --init --recursive
   ```

2. **Scaffold `../app/`** with `make setup-project`
   ```bash
   cd FreeRTOS-OS

   # From a known board (recommended) — copies board XML, kconfig, and a working app_main:
   make setup-project EXAMPLE=stm32h723   # NUCLEO-H723ZG  (Cortex-M7)
   make setup-project EXAMPLE=stm32f411   # STM32F411 devboard (Cortex-M4F)
   make setup-project EXAMPLE=stm32u575   # NUCLEO-U575ZI-Q (Cortex-M33 + TZ)

   # Or start blank — creates an empty app_main.c with inline API guidelines:
   make setup-project
   ```

   The resulting layout:
   ```
   my-firmware/
   ├── FreeRTOS-OS/              this repo (git submodule)
   ├── app/
   │   ├── app_main.c            your entry point
   │   ├── Makefile              app build fragment (app-obj-y, APP_INCLUDES)
   │   ├── kconfig.conf          Kconfig preset for your target MCU
   │   ├── board/
   │   │   ├── <board>.xml       board peripheral descriptor
   │   │   ├── irq_table.xml     NVIC priorities
   │   │   └── (generated)       board_config.{c,h}, irq_*_generated.*
   │   └── os_conf_include/
   │       ├── conf_board.h      COMM_PRINTK_HW_ID selection
   │       └── def_compiler.h
   └── README.md                 generated by setup-project
   ```

3. **Build and flash**
   ```bash
   cd FreeRTOS-OS
   make dev-stm32h723 APP_DIR=../app        # gen + config + compile
   make dev-stm32h723-flash                 # flash via OpenOCD / ST-Link

   # Release build (strip debug sections, define NDEBUG):
   make dev-stm32h723 APP_DIR=../app DEBUG=0
   ```

4. **Add your own source files** — edit `app/Makefile`:
   ```makefile
   app-obj-y += app_main.o
   app-obj-y += my_module.o
   app-obj-y += subdir/another.o
   ```

5. **Iterate**

   | Change | Re-run |
   |---|---|
   | `kconfig.conf` (MCU, heap size, services) | `cp ../app/kconfig.conf .config && make config-outputs` |
   | Board XML (pins, UARTs, peripherals) | `make dev-<target>-gen APP_DIR=../app` |
   | IRQ table XML (priorities, new IRQs) | `make irq_gen APP_DIR=../app` |
   | `app_main.c` / any `.c` | `make dev-<target> APP_DIR=../app` |

---

## 🧭 Architecture {#architecture}

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

## ⚙️ Make Target Reference {#make-target-reference}

Run all targets from `FreeRTOS-OS/`.  `<t>` ∈ {`stm32f411`, `stm32h723`, `stm32u575`}.

**Build mode flags** (apply to any `dev-*` or `all` target):

| Flag | Default | Effect |
|---|---|---|
| `DEBUG=1` | ✅ yes | `-g3` DWARF info embedded in ELF; flash footprint unchanged |
| `DEBUG=0` | — | Strips debug sections, defines `NDEBUG` (`-Os -DNDEBUG`) |

```bash
make dev-stm32h723             # debug build (default)
make dev-stm32h723 DEBUG=0     # release build
```

<details>
<summary><b>Devboard examples</b> (standalone, no sibling <code>app/</code> required)</summary>

All targets accept an optional `APP_DIR=../app` to build from your project instead of the
built-in example, and `DEBUG=0` for a release build.

| Command | Output |
|---|---|
| `make dev-stm32f411`            | `build/stm32f411.elf` — full gen + build |
| `make dev-stm32f411-gen`        | Regenerate F411 board + IRQ outputs only |
| `make dev-stm32f411-clean`      | Remove F411 generated outputs + `build/` |
| `make dev-stm32f411-flash`      | Flash via OpenOCD |
| `make dev-stm32h723`            | `build/stm32h723.elf` |
| `make dev-stm32h723-gen`        | Regenerate H723 board + IRQ outputs only |
| `make dev-stm32h723-clean`      | Remove H723 generated outputs + `build/` |
| `make dev-stm32h723-flash`      | Flash via OpenOCD |
| `make dev-stm32u575`            | `build/stm32u575.elf` (Cortex-M33 + TrustZone, trustcore demo) |
| `make dev-stm32u575-gen`        | Regenerate U575 board + IRQ outputs only |
| `make dev-stm32u575-clean`      | Remove U575 generated outputs + `build/` |
| `make dev-stm32u575-flash`      | Flash via OpenOCD |
</details>

<details>
<summary><b>High-level aliases</b> (dispatch to <code>dev-*</code> by TARGET)</summary>

| Command | Equivalent to |
|---|---|
| `make os TARGET=<t>`       | `make dev-<t>` |
| `make os-flash TARGET=<t>` | `make dev-<t>-flash` |
</details>

<details>
<summary><b>Project scaffolding</b> — create <code>../app/</code></summary>

| Command | Effect |
|---|---|
| `make setup-project` | Blank project: empty `app_main.c` with API guidelines |
| `make setup-project EXAMPLE=stm32h723` | Scaffold from H723 example (board XML + kconfig + app_main) |
| `make setup-project EXAMPLE=stm32f411` | Scaffold from F411 example |
| `make setup-project EXAMPLE=stm32u575` | Scaffold from U575 example (includes trustcore/) |
| `make setup-project EXAMPLE=<…> FORCE=1` | Overwrite existing `../app/` without prompt (CI / scripts) |
</details>

<details>
<summary><b>OS + Application</b> (sibling <code>../app/</code>)</summary>

| Command | Effect |
|---|---|
| `make dev-<t> APP_DIR=../app`       | Full build: gen → config → compile with `../app/` as app root |
| `make dev-<t> APP_DIR=../app DEBUG=0` | Release build |
| `make dev-<t>-flash`                | Flash via OpenOCD |
| `make dev-<t>-gen APP_DIR=../app`   | Regenerate `../app/board/*` from XML only |
| `make board-gen APP_DIR=../app`     | Regenerate BSP (`board_config.*`, `board_device_ids.h`, …) from board XML |
| `make irq_gen APP_DIR=../app`       | Regenerate IRQ tables from `../app/board/irq_table.xml` |
| `make app TARGET=<t>`               | Legacy alias — builds from `../app/` using hardcoded kconfig paths |
| `make app-flash TARGET=<t>`         | Flash the legacy app ELF |
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

## 🔐 STM32U575 / Cortex-M33 Trust Core {#stm32u575--cortex-m33-trust-core}

The `stm32u575` example wires a small **Trust Core** module into
`app_main()` to exercise the TrustZone-capable side of the M33:

  - **Secure-world hook** (`trustcore_init_secure_world`) — strong
    override of the weak symbol in `startup_stm32u575zitxq.c`; runs from
    `Reset_Handler` **before** `.data` copy / `.bss` clear, so it owns
    the address map before the C runtime touches it.  Today it caches
    the 96-bit STM32 device UID; the SAU programming hook is left in
    place commented for the `CONFIG_U5_TRUSTZONE_ENABLED=y` path.
  - **Boot attestation** (`trustcore_attest_runtime`) — runs from a
    dedicated priority-3 RTOS task once `printk` is online.  CRC32 of
    the vector table + `.text` + `.trustcore_text` is captured into a
    `.noinit` baseline on first boot and re-verified on every soft
    reset.
  - **Operator-visible verdict** — `LED_BOARD` blinks on integrity
    success; `LED_RED` latches on integrity failure; identity failure
    halts the application and fast-blinks `LED_RED`.
  - **Banner over LPUART1 (STLink VCP)** showing device UID + code CRC +
    boot count + status.

A green-light boot looks like this:

```
[trustcore] ───────────────────────────────────────────────
[trustcore]  STM32U575ZI — Cortex-M33 + TrustZone (U5)
[trustcore]  Device UID : 00120036-32365119-32383230
[trustcore]  World      : non-secure (single-image)
[trustcore]  Code CRC32 : 0x9f3b1a04
[trustcore]  Boot count : 4
[trustcore]  Status     : OK — integrity verified
[trustcore] ───────────────────────────────────────────────
```

The trustcore module is in `examples/stm32u575/trustcore/` and is the
recommended starting point for porting TF-M, MCUboot, or any
roll-your-own secure-boot anchor onto this OS.

---

## 🔬 Static Analysis {#static-analysis}

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

## 🤖 CI Pipeline {#ci-pipeline}

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

## 📚 Documentation {#documentation}

| Document | Topic |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Full architecture reference — layer model, drivers, safety, boot FSM, invariants |
| [`docs/DIAGRAMS.md`](docs/DIAGRAMS.md) | **All architectural diagrams** — layer stack, boot FSM, thread map, IRQ flow (Doxygen-rendered SVG) |
| [`docs/FLOW.md`](docs/FLOW.md) | Boot flow & function-call structure — `Reset_Handler → main → app_main → vTaskStartScheduler` |
| [`arch/README.md`](arch/README.md) | `/arch` contract — adding a new MCU / vendor |
| [`examples/README.md`](examples/README.md) | Devboard example layout + how to add a new devboard |
| [`docs/OS_THREAD.md`](docs/OS_THREAD.md) | Thread API, lifecycle, intrusive thread list |
| [`docs/QUEUE.md`](docs/QUEUE.md) | IPC message queues + ring buffer |
| [`docs/DRIVERS.md`](docs/DRIVERS.md) | Driver vtables and HAL backends |
| [`docs/BOARD.md`](docs/BOARD.md) | Board XML schema, BSP generator |
| [`docs/DEV_MGMT.md`](docs/DEV_MGMT.md) | Service-thread internals — every OS task explained |
| [`docs/SHELL_CLI.md`](docs/SHELL_CLI.md) | Shell, CLI, command registration |
| [`docs/IRQ.md`](docs/IRQ.md) | Linux-style IRQ subsystem |
| [`docs/DMA.md`](docs/DMA.md) | DMA engine + HAL backend |
| [`docs/DEBUG.md`](docs/DEBUG.md) | VS Code debug, OpenOCD, GDB, SVD viewer |
| [`docs/OS_INSIDE.md`](docs/OS_INSIDE.md) | Post-mortems and debug recipes |
| [`docs/CHECK.md`](docs/CHECK.md) | CPPcheck + MISRA C:2012 |
| [`docs/DSP.md`](docs/DSP.md) | CMSIS-DSP integration |

---

## 🤝 Contributing {#contributing}

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

## 📄 License {#license}

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
