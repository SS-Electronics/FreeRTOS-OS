# FreeRTOS-OS

A layered embedded OS framework built on top of the FreeRTOS kernel, targeting ARM Cortex-M4F (STM32F411VET6) with a vendor-abstracted driver stack and compile-time board configuration.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Repository Layout](#repository-layout)
- [Architecture Diagram](#architecture-diagram)
- [Quick Start](#quick-start)
- [Build System](#build-system)
- [Configuration (Kconfig)](#configuration-kconfig)
- [Board Configuration](#board-configuration)
- [Driver Stack](#driver-stack)
- [OS Services](#os-services)
- [Debug Setup](#debug-setup)
- [Documentation Index](#documentation-index)

---

## Project Overview

FreeRTOS-OS provides:

- **Board generation** — XML-driven BSP: one `make board-gen` produces four C/H files with all peripheral descriptors, device IDs, HAL handles, and MCU capability constants.
- **Vendor-abstracted drivers** — UART, I2C, SPI, GPIO, Timer, WDG with an ops-vtable pattern. Swap vendors by changing `CONFIG_DEVICE_VARIANT` — no changes to the generic driver layer or application code.
- **Management service threads** — per-bus FreeRTOS tasks that self-register all peripherals from the board config and serialise hardware access through message queues.
- **RCC driver** — clock initialisation abstracted behind `drv_rcc_clock_init()` with a vendor HAL backend in `drivers/hal/stm32/hal_rcc_stm32.c`.
- **Single-click VSCode debug** — F5 builds, flashes, and halts at `main()` with FreeRTOS task awareness, peripheral register viewer (SVD), and SWO output.

**Currently supported MCU:** STM32F411VET6 (ARM Cortex-M4F @ 100 MHz, 512 KB Flash, 128 KB RAM).

---

## Repository Layout

```
FreeRTOS-OS-App/
│
├── app/
│   ├── board/
│   │   ├── stm32f411_devboard.xml     ← user board description (hand-written)
│   │   ├── board_config.c             ← generated — do not edit
│   │   ├── board_device_ids.h         ← generated — do not edit
│   │   ├── board_handles.h            ← generated — do not edit
│   │   └── mcu_config.h               ← generated — do not edit
│   └── Makefile
│
└── FreeRTOS-OS/
    ├── arch/                          ← Layer 0: silicon support
    │   ├── Makefile
    │   ├── arm/CMSIS_6/               ← ARM CMSIS-Core headers
    │   └── devices/
    │       ├── device.h               ← vendor-detection umbrella include
    │       ├── device_conf/
    │       │   └── stm32f4xx_hal_conf.h
    │       └── STM/
    │           ├── STM32F4xx/         ← CMSIS device header + linker script
    │           └── stm32f4xx-hal-driver/
    │
    ├── drivers/                       ← Layers 2 + 3: driver implementation
    │   ├── drv_rcc.c                  ← RCC clock-init dispatcher
    │   ├── drv_uart.c  drv_iic.c  drv_spi.c  drv_gpio.c
    │   ├── drv_irq.c                  ← vendor-agnostic IRQ dispatch wrappers
    │   └── hal/stm32/
    │       ├── hal_rcc_stm32.c        ← SystemInit + PLL config + CMSIS tables
    │       ├── hal_uart_stm32.c  hal_iic_stm32.c  hal_spi_stm32.c
    │       ├── hal_gpio_stm32.c  hal_msp_stm32.c
    │       ├── hal_timebase_stm32.c   ← TIM1-based HAL tick
    │       ├── hal_it_stm32.c         ← peripheral ISR dispatch (direct register access)
    │       ├── irq_chip_nvic.c        ← ARM NVIC irq_chip implementation
    │       └── stm32_exceptions.c     ← Cortex-M4 core fault handlers
    │
    ├── include/
    │   ├── drivers/
    │   │   ├── drv_handle.h           ← handle structs + ops vtables
    │   │   ├── drv_irq.h              ← vendor-agnostic IRQ dispatch API
    │   │   ├── drv_rcc.h              ← RCC driver public API
    │   │   └── hal/stm32/
    │   │       ├── hal_rcc_stm32.h    ← SystemCoreClock, CMSIS symbols, PLL constants
    │   │       └── irq_chip_nvic.h    ← NVIC irq_chip: bind hwirq, set priority
    │   ├── irq/
    │   │   ├── irq_notify.h           ← IRQ ID macros, IRQ_ID_TOTAL, irq_notify_cb_t
    │   │   ├── irq_desc.h             ← irq_desc/chip/action structs, full API
    │   │   └── irq_table.h            ← irq_table_get_name() declaration
    │   ├── board/
    │   │   ├── board_config.h         ← stable board API (hand-written)
    │   │   ├── board_device_ids.h     ← stub for standalone kernel builds
    │   │   └── board_handles.h        ← stub for standalone kernel builds
    │   └── services/
    │       ├── uart_mgmt.h  iic_mgmt.h  spi_mgmt.h  gpio_mgmt.h
    │
    ├── services/                      ← Management service threads
    │   ├── uart_mgmt.c  iic_mgmt.c  spi_mgmt.c  gpio_mgmt.c
    │
    ├── kernel/                        ← FreeRTOS kernel + Newlib glue
    ├── init/                          ← main.c, app entry point
    ├── config/                        ← autoconf.h, FreeRTOSConfig.h, conf_os.h
    ├── irq/
    │   ├── irq_desc.c                 ← descriptor table, pool, flow handlers, dispatch
    │   ├── irq_notify.c               ← irq_register() trampoline adapter
    │   └── irq_table.c                ← GENERATED — IRQ name strings
    ├── scripts/  (project root)
    │   ├── gen_board_config.py        ← XML → BSP code generator
    │   ├── gen_irq_table.py           ← irq_table.xml → irq_table.c + irq_hw_init_generated.c
    │   └── gen_active_debug.py        ← autoconf → build/active_debug.cfg
    └── docs/                          ← This documentation
```

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                      Application / Protocol Stack                   │
│       uart_sync_transmit()   iic_sync_receive()   gpio_read()  …   │
└──────────────────────────┬──────────────────────────────────────────┘
                           │  drv_app/ wrappers
┌──────────────────────────▼──────────────────────────────────────────┐
│              Management Service Threads  (services/)                │
│   uart_mgmt · iic_mgmt · spi_mgmt · gpio_mgmt                     │
│   — self-register all peripherals from board_get_config() at boot  │
│   — serialise bus access through FreeRTOS message queues           │
└──────────────────────────┬──────────────────────────────────────────┘
                           │  drv_xxx_handle.ops->…
┌──────────────────────────▼──────────────────────────────────────────┐
│               Generic Driver Layer  (drivers/)           LAYER 3   │
│   drv_uart · drv_iic · drv_spi · drv_gpio                         │
│   drv_rcc  · drv_time · drv_cpu                                    │
└──────────────────────────┬──────────────────────────────────────────┘
                           │  const drv_xxx_hal_ops_t *ops
┌──────────────────────────▼──────────────────────────────────────────┐
│              Vendor HAL Backends  (drivers/hal/)         LAYER 2   │
│  STM32: hal_rcc_stm32  hal_uart_stm32  hal_iic_stm32               │
│         hal_spi_stm32  hal_gpio_stm32  hal_msp_stm32               │
│         hal_timebase_stm32  hal_it_stm32                           │
│  Infineon: stubs (TODO)   Microchip: stubs (TODO)                  │
└──────────────────────────┬──────────────────────────────────────────┘
                           │  #include <device.h>
┌──────────────────────────▼──────────────────────────────────────────┐
│            Architecture Support  (arch/)                 LAYER 0   │
│  arch/devices/device.h          — vendor-detection umbrella        │
│  arch/devices/device_conf/      — HAL module conf per vendor       │
│  arch/devices/STM/STM32F4xx/    — CMSIS device header + startup   │
│  arch/devices/STM/stm32f4xx-hal-driver/ — ST HAL library          │
│  arch/arm/CMSIS_6/              — ARM CMSIS-Core headers           │
└─────────────────────────────────────────────────────────────────────┘
                    Physical hardware (STM32F411xE)
```

**Board configuration is the single source of truth.** Management threads call `board_get_config()` at startup and self-register every peripheral defined in the board XML. No user code is needed to initialise peripherals.

---

## Quick Start

```bash
# 1. Install ARM toolchain and OpenOCD
sudo bash scripts/install_arm_gcc.sh
sudo bash scripts/install_openocd.sh
bash scripts/install_debug_tools.sh   # SVD + VSCode extensions

# 2. Configure for your board/MCU
make menuconfig          # opens Kconfig ncurses menu
make config-outputs      # generates config/autoconf.h

# 3. Generate BSP from board XML
make board-gen           # writes app/board/{board_config.c, board_device_ids.h,
                         #                   board_handles.h, mcu_config.h}

# 4. Build
make app                 # full build → build/kernel.elf

# 5. Flash and debug (VSCode)
# Press F5 — select "STM32F411 — Build, Flash & Debug"

# Or flash from CLI
make flash
```

---

## Build System

The build system is a hand-written recursive `make`. The top-level `Makefile` in `FreeRTOS-OS/` drives everything.

| Target | Effect |
|--------|--------|
| `make app` | Full build (board-gen + compile + link) → `build/app.elf` |
| `make board-gen` | Regenerate BSP files from board XML |
| `make irq_gen` | Regenerate `irq_table.c` + `irq_hw_init_generated.c` from `irq_table.xml` |
| `make menuconfig` | Kconfig ncurses menu |
| `make config-outputs` | Regenerate `config/autoconf.h` and `autoconf.mk` from `.config` |
| `make flash-app` | Flash `build/app.elf` via OpenOCD |
| `make clean` | Remove `build/` and generated BSP files |

The build system injects two key preprocessor symbols at compile time:

```makefile
TARGET_SYMBOL_DEF += -D$(CONFIG_TARGET_MCU)   # e.g. -DSTM32F411xE
SYMBOL_DEF        += -DUSE_HAL_DRIVER
```

`device.h` uses `-DSTM32F411xE` (or the equivalent for other chips) to select the correct vendor/family include block at the preprocessor level.

---

## Configuration (Kconfig)

Configuration lives in `FreeRTOS-OS/.config` and is managed by `make menuconfig`. The `.config` file is processed into:

- `config/autoconf.h` — `#define CONFIG_*` macros included by all C source files
- `autoconf.mk` — `CONFIG_*=value` lines included by the Makefile

Key configuration sections (see `arch/Kconfig` and `kernel/Kconfig`):

| Section | Key symbols |
|---------|-------------|
| MCU selection | `CONFIG_TARGET_MCU`, `CONFIG_CPU_ARCH` |
| Clock tree | `CONFIG_HSI_VALUE`, `CONFIG_HSE_VALUE` |
| HAL modules | `CONFIG_HAL_UART_MODULE_ENABLED`, `CONFIG_HAL_I2C_MODULE_ENABLED`, … |
| OS service layer | `CONFIG_INC_SERVICE_UART_MGMT`, `CONFIG_INC_SERVICE_IIC_MGMT`, … |
| Peripheral counts | `CONFIG_NO_OF_UART`, `CONFIG_NO_OF_IIC` |
| FreeRTOS kernel | `CONFIG_RTOS_TOTAL_HEAP_SIZE`, `CONFIG_RTOS_TICK_RATE_HZ`, … |

**Default template** (STM32F411VET6 @ 100 MHz): UART and I2C are both enabled by default (`default y`). Run `make menuconfig` to customise.

---

## Board Configuration

All hardware peripheral parameters live in `app/board/<board>.xml`. Running `make board-gen` invokes `scripts/gen_board_config.py` which produces four files in `app/board/`:

| Generated file | Contents |
|----------------|----------|
| `board_config.c` | HAL handle definitions, descriptor tables, `board_get_config()`, callback API |
| `board_device_ids.h` | `#define UART_DEBUG 0`, `BOARD_UART_COUNT 2`, etc. |
| `board_handles.h` | `extern UART_HandleTypeDef huart1;` etc. |
| `mcu_config.h` | `CONFIG_DEVICE_VARIANT`, `MCU_VAR_STM`, `UART_N_EN`, peripheral count macros |

`board_config.c` selects the correct vendor HAL via `CONFIG_DEVICE_VARIANT`:

```c
#include <board/mcu_config.h>

#if   (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <device.h>    /* → device_conf/stm32f4xx_hal_conf.h + stm32f4xx_hal.h */
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <device.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_MICROCHIP)
#  include <device.h>
#else
#  error "unknown CONFIG_DEVICE_VARIANT"
#endif
```

See [BOARD.md](BOARD.md) for the full XML format, generator architecture, and Makefile integration.

---

## Driver Stack

The driver stack has four layers:

| Layer | Location | Role |
|-------|----------|------|
| **Layer 0** | `arch/` | Silicon: CMSIS headers, startup, `device.h`, HAL library |
| **Layer 1** | `include/drivers/drv_handle.h` | Handle structs, ops vtables, vendor hw context types |
| **Layer 2** | `drivers/hal/<vendor>/` | Vendor HAL backends — all `HAL_xxx_*()` calls are here |
| **Layer 3** | `drivers/drv_*.c` | Vendor-agnostic generic drivers — dispatch through ops vtables |

### Clock Initialisation (RCC)

`drv_rcc_clock_init()` is called from `main()` after `HAL_Init()`. It dispatches to the vendor backend registered by `_hal_rcc_stm32_register()`:

```
main()
  └─ HAL_Init()
       └─ drv_rcc_clock_init()
            └─ _hal_rcc_stm32_register(&ops)
                 └─ ops->clock_init()   [hal_rcc_stm32.c]
                      └─ PLL: HSI→PLLM=16→PLLN=200→PLLP=2 = 100 MHz SYSCLK
```

`hal_rcc_stm32.c` also provides the CMSIS system symbols (`SystemInit`, `SystemCoreClockUpdate`, `SystemCoreClock`, `AHBPrescTable`, `APBPrescTable`), absorbing the former `system_stm32f4xx.c`.

### `device.h` — Vendor Detection

`arch/devices/device.h` is the single include used by all driver-layer files. It detects the vendor from the chip symbol injected at compile time (`-DSTM32F411xE` etc.):

```c
#if defined(STM32F0) || defined(STM32F4) || defined(STM32F411xE) || …
    #define DEVICE_VENDOR_STM32  1
    #include <stm32f4xx.h>
    #include <stm32f4xx_hal_conf.h>
    #include <stm32f4xx_hal.h>
    …
#elif defined(XMC4800) || …
    #define DEVICE_VENDOR_INFINEON  1
    /* TODO */
#elif defined(__SAMD21__) || …
    #define DEVICE_VENDOR_MICROCHIP  1
    /* TODO */
#else
    #error "unrecognised MCU"
#endif
#include <board/board_handles.h>
```

See [DRIVERS.md](DRIVERS.md) for the complete driver architecture, vtable interfaces, and porting guides.

---

## OS Services

Four FreeRTOS management threads own the bus hardware and serialise access:

| Thread | Startup delay | Peripheral type |
|--------|--------------|----------------|
| `gpio_mgmt` | 3 000 ms | GPIO (runs first — clocks GPIO before other MSP inits) |
| `uart_mgmt` | 4 000 ms | UART / USART |
| `spi_mgmt` | 5 500 ms | SPI |
| `iic_mgmt` | 6 500 ms | I2C |

Application code calls `drv_app/` wrappers (`uart_sync_transmit()`, `iic_sync_receive()`, etc.) rather than hitting the management queues directly.

Enable/disable threads in `config/conf_os.h` or via `make menuconfig` (OS Service Layer menu). Both UART and I2C are enabled by default.

See [DEV_MGMT.md](DEV_MGMT.md) for message protocols, sync/async patterns, and configuration knobs.

---

## Debug Setup

The debug system auto-detects the target MCU from `config/autoconf.h` and generates `build/active_debug.cfg` for OpenOCD:

```bash
# Prerequisite: run once
bash scripts/install_debug_tools.sh

# Then in VSCode: press F5
# preLaunchTask: "detect-target" → reads config/autoconf.h → writes build/active_debug.cfg
# then: build → flash → GDB connects → halts at main()
```

**Features:** FreeRTOS task awareness (RTOS Threads panel), peripheral register viewer (SVD), SWO/ITM output, command-line GDB support.

See [DEBUG.md](DEBUG.md) for the complete setup, configuration, and troubleshooting guide.

---

## Documentation Index

| Document | Covers |
|----------|--------|
| [IRQ.md](IRQ.md) | IRQ descriptor chain, irq_desc/chip/action structs, dispatch path, EXTI example, generator |
| [BOARD.md](BOARD.md) | XML board description, code generator, generated files, Makefile integration |
| [DRIVERS.md](DRIVERS.md) | Full driver architecture, Layer 0–3 details, vendor porting, include paths |
| [DEV_MGMT.md](DEV_MGMT.md) | Management service threads, message protocols, sync/async patterns |
| [SHELL_CLI.md](SHELL_CLI.md) | OS shell CLI over UART, FreeRTOS+CLI integration, printk, registering commands |
| [DEBUG.md](DEBUG.md) | VSCode debug setup, OpenOCD config, FreeRTOS task awareness, SWO |
| [OS_THREAD.md](OS_THREAD.md) | OS thread creation, priorities, scheduling |
| [QUEUE.md](QUEUE.md) | IPC queues and ring buffers |

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
