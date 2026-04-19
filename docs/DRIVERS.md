# Driver Architecture — FreeRTOS-OS

This document describes the complete driver subsystem from silicon to application: the `arch/` device support layer, the Newlib kernel glue, the three driver layers, and the FreeRTOS management service threads.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Directory Layout](#directory-layout)
- [Layer 0 — Architecture Support (`arch/`)](#layer-0--architecture-support-arch)
  - [CMSIS-Core](#cmsis-core)
  - [Device Register Definitions](#device-register-definitions)
  - [Device Entry Point (`device.h`)](#device-entry-point-deviceh)
  - [Startup File](#startup-file)
  - [System Initialisation (`system_stm32f4xx.c`)](#system-initialisation-system_stm32f4xxc)
  - [HAL Peripheral Library](#hal-peripheral-library)
  - [Linker Script](#linker-script)
- [Kernel Glue (`kernel/`)](#kernel-glue-kernel)
  - [Newlib Syscall Stubs (`syscalls.c`)](#newlib-syscall-stubs-syscallsc)
  - [Heap Glue (`sysmem.c`)](#heap-glue-sysmemc)
- [Layer 1 — Generic Handle (`drv_handle.h`)](#layer-1--generic-handle-drv_handleh)
  - [Handle Structs](#handle-structs)
  - [HAL Ops Vtables](#hal-ops-vtables)
  - [Vendor Hardware Context](#vendor-hardware-context)
  - [Registration API](#registration-api)
- [Layer 2 — Vendor HAL Backends](#layer-2--vendor-hal-backends)
  - [STM32 HAL Backend](#stm32-hal-backend)
    - [UART — hal_uart_stm32](#uart--hal_uart_stm32)
    - [I2C — hal_iic_stm32](#i2c--hal_iic_stm32)
    - [SPI — hal_spi_stm32](#spi--hal_spi_stm32)
    - [GPIO — hal_gpio_stm32](#gpio--hal_gpio_stm32)
    - [MSP Callbacks — hal_msp_stm32](#msp-callbacks--hal_msp_stm32)
    - [HAL Timebase — hal_timebase_stm32](#hal-timebase--hal_timebase_stm32)
    - [ISR Dispatch — hal_it_stm32](#isr-dispatch--hal_it_stm32)
  - [Infineon Baremetal Backend (Stubs)](#infineon-baremetal-backend-stubs)
- [Layer 3 — Generic Driver API](#layer-3--generic-driver-api)
  - [UART Driver](#uart-driver-drv_uartc)
  - [I2C Driver](#i2c-driver-drv_iicc)
  - [SPI Driver](#spi-driver-drv_spic)
  - [GPIO Driver](#gpio-driver-drv_gpioc)
  - [Timer / Time-base Driver](#timer--time-base-driver-drv_timec)
  - [CPU Utility and Watchdog Driver](#cpu-utility-and-watchdog-driver-drv_cpuc)
- [Management Service Threads](#management-service-threads)
  - [UART Management — uart_mgmt](#uart-management--uart_mgmt)
  - [I2C Management — iic_mgmt](#i2c-management--iic_mgmt)
  - [SPI Management — spi_mgmt](#spi-management--spi_mgmt)
  - [GPIO Management — gpio_mgmt](#gpio-management--gpio_mgmt)
- [External Chip Drivers](#external-chip-drivers)
- [Adding a New Vendor](#adding-a-new-vendor)
- [Adding a New Peripheral Driver](#adding-a-new-peripheral-driver)
- [Include Path Reference](#include-path-reference)
- [Kconfig Flags That Affect Drivers](#kconfig-flags-that-affect-drivers)

---

## Architecture Overview

The system is built from the silicon up in four distinct layers. Higher layers never reach down past the boundary immediately below them.

```
┌────────────────────────────────────────────────────────────────────┐
│                  Application / Protocol Stack                      │
│          drv_serial_transmit()  drv_iic_transmit()  …             │
└───────────────────────────┬────────────────────────────────────────┘
                            │  Generic driver API
┌───────────────────────────▼────────────────────────────────────────┐
│            Management Service Threads  (services/)                 │
│   uart_mgmt · iic_mgmt · spi_mgmt · gpio_mgmt                    │
│   — own the handle arrays, run as FreeRTOS tasks                   │
│   — dispatch TX/RX via FreeRTOS queue messages                     │
└────────────┬───────────────────────────────┬───────────────────────┘
             │  drv_uart_handle_t.ops->…     │
┌────────────▼───────────────────────────────▼───────────────────────┐
│                Generic Driver Layer  (drivers/)                    │  LAYER 3
│   drv_uart.c  drv_iic.c  drv_spi.c  drv_gpio.c                   │
│   drv_time.c  drv_cpu.c                                           │
│   — vendor-agnostic                                                │
│   — owns static handle arrays                                      │
│   — dispatches every operation through ops vtable                  │
└────────────┬───────────────────────────────────────────────────────┘
             │  const drv_uart_hal_ops_t *ops  (function pointers)
┌────────────▼───────────────────────────────────────────────────────┐
│              Vendor HAL Backend  (drivers/hal/)                    │  LAYER 2
│                                                                    │
│  ┌─────────────────────────────┐  ┌───────────────────────────┐   │
│  │   STM32 HAL                 │  │  Infineon XMC (baremetal)  │   │
│  │  hal_uart_stm32.c           │  │  hal_uart_infineon.c       │   │
│  │  hal_iic_stm32.c            │  │  hal_iic_infineon.c        │   │
│  │  hal_spi_stm32.c            │  │  (stubs — TODO registers)  │   │
│  │  hal_gpio_stm32.c           │  └───────────────────────────┘   │
│  │  hal_msp_stm32.c            │                                   │
│  │  hal_timebase_stm32.c       │                                   │
│  │  hal_it_stm32.c             │                                   │
│  └─────────────────────────────┘                                   │
│   calls STM32 HAL_UART_Init() / HAL_I2C_Init() / HAL_TIM_… / …   │
└────────────────────────────────────────────────────────────────────┘
             │  #include <device.h>  (STM32 HAL + CMSIS)
┌────────────▼───────────────────────────────────────────────────────┐
│          Generic Handle Definitions  (include/drivers/)            │  LAYER 1
│   drv_handle.h — handle structs, vtables, hw context types        │
└────────────────────────────────────────────────────────────────────┘
             │  device.h / stm32f4xx.h
┌────────────▼───────────────────────────────────────────────────────┐
│         Architecture Support  (arch/)                              │  LAYER 0
│                                                                    │
│  arch/arm/CMSIS_6/           — CMSIS-Core headers (arm-common)    │
│  arch/devices/STM/STM32F4xx/ — STM32F4 register definitions       │
│    device.h                  — single include umbrella             │
│    stm32f4xx.h               — CMSIS device header                 │
│    stm32f4xx_hal_conf.h      — HAL module enable switches         │
│    system_stm32f4xx.c        — SystemInit, clock tables            │
│    STM32F411/                — chip-variant-specific files         │
│      startup_stm32f411vetx.c — exception vector table + Reset_Handler │
│      stm32_f411_FLASH.ld     — linker script                       │
│      stm32_f411xe.h          — chip-level register map override    │
│  arch/devices/STM/stm32f4xx-hal-driver/ — ST HAL library source   │
│                                                                    │
│  kernel/syscalls.c           — Newlib I/O stubs (_write, _read …) │
│  kernel/sysmem.c             — Newlib heap hook (_sbrk)            │
└────────────────────────────────────────────────────────────────────┘
                    Physical hardware (STM32F411xE)
```

**Board configuration is the single source of truth for all peripheral parameters.** Management threads read `board_get_config()` at startup and self-register every peripheral listed in the board XML. See [BOARD.md](BOARD.md) for the complete XML → BSP generation flow.

**The vendor is selected at compile time** via `CONFIG_DEVICE_VARIANT` in [`include/config/mcu_config.h`](include/config/mcu_config.h):

```c
#define MCU_VAR_STM         2   /* STM32 — uses STM32 HAL */
#define MCU_VAR_INFINEON    3   /* Infineon XMC — baremetal register access */

#define CONFIG_DEVICE_VARIANT   MCU_VAR_STM
```

Switching vendors requires only changing `CONFIG_DEVICE_VARIANT` — no changes to the generic driver layer or application code.

---

## Directory Layout

```
FreeRTOS-OS/
│
├── arch/                              ← LAYER 0 — Architecture support
│   ├── Makefile
│   ├── arm/
│   │   └── CMSIS_6/                   ← ARM CMSIS-Core headers (all Cortex-M)
│   │       └── CMSIS/Core/Include/
│   │           ├── cmsis_gcc.h
│   │           ├── core_cm4.h         ← Cortex-M4 core peripherals (SysTick, NVIC …)
│   │           └── …
│   └── devices/
│       └── STM/
│           ├── STM32F4xx/             ← STM32F4xx device support
│           │   ├── device.h           ← Single-include umbrella for device + HAL
│           │   ├── stm32f4xx.h        ← CMSIS device header (selects chip variant)
│           │   ├── stm32f4xx_hal_conf.h ← HAL module enable switches
│           │   ├── system_stm32f4xx.h
│           │   ├── system_stm32f4xx.c ← SystemInit + clock tables (no HAL handles)
│           │   └── STM32F411/         ← STM32F411xE variant
│           │       ├── startup_stm32f411vetx.c ← Vector table + Reset_Handler
│           │       ├── stm32_f411xe.h ← Chip-level peripheral addresses
│           │       └── stm32_f411_FLASH.ld     ← Linker script
│           └── stm32f4xx-hal-driver/  ← ST HAL peripheral library (Src/ + Inc/)
│
├── kernel/                            ← FreeRTOS kernel + bare-metal glue
│   ├── Makefile
│   ├── syscalls.c                     ← Newlib I/O stubs (arch-agnostic)
│   ├── sysmem.c                       ← Newlib _sbrk heap hook (arch-agnostic)
│   ├── kernel_thread.c
│   ├── FreeRTOS-Kernel/               ← FreeRTOS source
│   │   ├── tasks.c  queue.c  …
│   │   └── portable/
│   │       ├── GCC/ARM_CM4F/port.c    ← Cortex-M4 port
│   │       └── MemMang/heap_4.o
│   └── …
│
├── include/
│   ├── def_attributes.h               ← Linker section attributes (__SECTION_BOOT …)
│   ├── def_compiler.h                 ← Compiler abstraction stubs
│   ├── def_env_macros.h               ← Environment macro stubs
│   ├── drivers/
│   │   ├── drv_handle.h               ← LAYER 1: handles + vtables + hw contexts
│   │   │
│   │   ├── com/                       ← Communication peripherals
│   │   │   ├── drv_uart.h
│   │   │   ├── drv_iic.h
│   │   │   ├── drv_spi.h
│   │   │   └── hal/
│   │   │       ├── stm32/
│   │   │       │   ├── hal_uart_stm32.h
│   │   │       │   ├── hal_iic_stm32.h
│   │   │       │   └── hal_spi_stm32.h
│   │   │       └── infineon/
│   │   │           ├── hal_uart_infineon.h
│   │   │           └── hal_iic_infineon.h
│   │   │
│   │   ├── cpu/
│   │   │   ├── drv_gpio.h
│   │   │   ├── drv_cpu.h              ← CPU + WDG API
│   │   │   └── hal/stm32/
│   │   │       └── hal_gpio_stm32.h
│   │   │
│   │   ├── timer/
│   │   │   └── drv_time.h             ← Tick + per-instance timer API
│   │   │
│   │   └── hal/stm32/                 ← STM32 system-level HAL headers
│   │       ├── hal_it_stm32.h         ← ISR dispatcher declarations
│   │       └── hal_timebase_stm32.h   ← Timebase IRQ dispatcher declaration
│   │
│   └── services/
│       ├── uart_mgmt.h
│       ├── iic_mgmt.h
│       ├── spi_mgmt.h
│       └── gpio_mgmt.h
│
├── drivers/                           ← LAYER 2 + LAYER 3
│   ├── Makefile
│   ├── drv_uart.c                     ← Generic UART driver
│   ├── drv_iic.c
│   ├── drv_spi.c
│   ├── drv_gpio.c
│   ├── drv_time.c                     ← Tick utilities + ops-table timer driver
│   ├── drv_cpu.c                      ← CPU utils + fault handlers + WDG driver
│   └── hal/
│       ├── stm32/
│       │   ├── hal_uart_stm32.c
│       │   ├── hal_iic_stm32.c
│       │   ├── hal_spi_stm32.c
│       │   ├── hal_gpio_stm32.c
│       │   ├── hal_msp_stm32.c        ← HAL_MspInit + peripheral MSP callbacks
│       │   ├── hal_timebase_stm32.c   ← TIM1-based HAL tick override
│       │   └── hal_it_stm32.c         ← Peripheral ISR dispatch table
│       └── infineon/
│           ├── hal_uart_infineon.c
│           └── hal_iic_infineon.c
│
└── services/
    ├── Makefile
    ├── uart_mgmt.c
    ├── iic_mgmt.c
    ├── spi_mgmt.c
    └── gpio_mgmt.c
```

---

## Layer 0 — Architecture Support (`arch/`)

The `arch/` subtree contains everything that is specific to the silicon and toolchain. Nothing above this layer references register addresses, CMSIS types, or STM32 HAL handles directly — those are hidden inside `drivers/hal/stm32/`.

### CMSIS-Core

**Path:** [`arch/arm/CMSIS_6/CMSIS/Core/Include/`](arch/arm/CMSIS_6/CMSIS/Core/Include/)

ARM CMSIS-Core provides the portable Cortex-M interface:

| Header | Purpose |
|---|---|
| `core_cm4.h` | Cortex-M4 core register definitions: SysTick, NVIC, SCB, FPU, ITM |
| `cmsis_gcc.h` | GCC-specific intrinsics (`__NOP`, `__DSB`, `__ISB`, `__get_PRIMASK`, …) |
| `cmsis_compiler.h` | Compiler detection and attribute mapping |
| `cmsis_version.h` | CMSIS version constants |

These headers are pulled in transitively through `stm32f4xx.h`. Application code and the driver layer must not include them directly — always use `<device.h>`.

**Makefile contribution:**
```makefile
INCLUDES += -I$(CURDIR)/arch/arm/CMSIS_6
```

---

### Device Register Definitions

**Path:** [`arch/devices/STM/STM32F4xx/`](arch/devices/STM/STM32F4xx/)

| File | Purpose |
|---|---|
| `stm32f4xx.h` | CMSIS device header — includes the correct chip variant header based on the `STM32F411xE` preprocessor symbol, then includes `core_cm4.h` |
| `STM32F411/stm32_f411xe.h` | STM32F411xE-specific peripheral base addresses and register struct typedefs (USART, I2C, SPI, TIM, RCC, GPIO, …) |
| `stm32f4xx_hal_conf.h` | HAL module enable switches (`HAL_UART_MODULE_ENABLED`, `HAL_TIM_MODULE_ENABLED`, etc.) and clock source constants (`HSE_VALUE`, `HSI_VALUE`) |

The device define `STM32F411xE` must be passed at compile time (via `SYMBOL_DEF` in [`arch/Makefile`](arch/Makefile)):

```makefile
SYMBOL_DEF += -DSTM32F411xE -DUSE_HAL_DRIVER
```

---

### Device Entry Point (`device.h`)

**File:** [`arch/devices/STM/STM32F4xx/device.h`](arch/devices/STM/STM32F4xx/device.h)

This is the single include that every driver-layer and HAL-layer file uses to access device types. It guards itself with `#if defined(STM32F411xE)` so it is inert for other targets.

```c
#include "def_attributes.h"           // __SECTION_BOOT, __SECTION_BOOT_DATA
#include "stm32f4xx.h"                // CMSIS device header + register structs
#include "STM32F411/stm32_f411xe.h"   // chip-level peripheral addresses
#include "system_stm32f4xx.h"         // SystemCoreClock declaration
#include "stm32f4xx_hal_conf.h"       // HAL module selection
#include <stm32f4xx_hal.h>            // STM32 HAL (includes all enabled modules)

extern uint32_t      SystemCoreClock;
extern const uint8_t AHBPrescTable[16];
extern const uint8_t APBPrescTable[8];

#include <board/board_handles.h>      // board descriptor declarations
```

> `stm32f4xx_it.h` is **not** included here. Interrupt handler declarations live in [`include/drivers/hal/stm32/hal_it_stm32.h`](include/drivers/hal/stm32/hal_it_stm32.h), included directly by `hal_it_stm32.c`.

---

### Startup File

**File:** [`arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c`](arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c)

Compiled as the very first translation unit. Responsibilities:

1. **Exception vector table** — placed in `.isr_vector` section, linked to the start of Flash. Contains `_estack` as the initial MSP value, then all Cortex-M4 and STM32F411 peripheral interrupt vectors.
2. **`Reset_Handler`** — copies `.data` from Flash to RAM, zeroes `.bss`, then calls `SystemInit()` (clock setup) and finally `main()`.
3. **Default fault handlers** — `HardFault_Handler`, `MemManage_Handler`, `BusFault_Handler`, `UsageFault_Handler` are defined as weak aliases pointing to an infinite loop. `drv_cpu.c` overrides the Cortex-M4 fault handlers with diagnostics.

The peripheral interrupt vectors (`USART1_IRQHandler`, `TIM1_UP_TIM10_IRQHandler`, etc.) are declared `weak` in the vector table. The strong definitions are provided by [`drivers/hal/stm32/hal_it_stm32.c`](drivers/hal/stm32/hal_it_stm32.c).

---

### System Initialisation (`system_stm32f4xx.c`)

**File:** [`arch/devices/STM/STM32F4xx/system_stm32f4xx.c`](arch/devices/STM/STM32F4xx/system_stm32f4xx.c)

Called by `Reset_Handler` before `main()`. Provides:

| Symbol | Description |
|---|---|
| `SystemInit()` | Enables FPU co-processor, configures Flash latency and AHB/APB prescalers, brings up PLL from HSE/HSI, configures VTOR |
| `SystemCoreClockUpdate()` | Recomputes `SystemCoreClock` from RCC register state — call after any clock change |
| `SystemCoreClock` | Global variable — current core clock in Hz (initialised to `HSI_VALUE`) |
| `AHBPrescTable[16]` | Lookup table: RCC AHB prescaler field → actual divider |
| `APBPrescTable[8]` | Lookup table: RCC APB prescaler field → actual divider |

**What this file does NOT contain:**

- No `UART_HandleTypeDef`, `TIM_HandleTypeDef`, or any other HAL peripheral handle. Those were removed and now live inside the driver handle structs (`drv_hw_uart_ctx_t`, `drv_hw_timer_ctx_t`) owned by the driver layer.
- No `HAL_Init()` call — that is done by `kernel_thread.c` after the RTOS scheduler starts and the HAL timebase (TIM1) is ready.

---

### HAL Peripheral Library

**Path:** [`arch/devices/STM/stm32f4xx-hal-driver/`](arch/devices/STM/stm32f4xx-hal-driver/)

The ST-supplied HAL library. Compiled conditionally in [`arch/Makefile`](arch/Makefile) based on `CONFIG_TARGET_MCU`. Only the modules enabled in `stm32f4xx_hal_conf.h` are compiled:

```makefile
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_rcc.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_gpio.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_cortex.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_uart.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_tim.o
obj-y += arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal_dma.o
# … flash, pwr, cortex
```

Application code and the generic driver layer never call HAL functions directly — all calls are routed through the ops vtables in `drivers/hal/stm32/`.

---

### Linker Script

**File:** [`arch/devices/STM/STM32F4xx/STM32F411/stm32_f411_FLASH.ld`](arch/devices/STM/STM32F4xx/STM32F411/stm32_f411_FLASH.ld)

Defines the memory map for code in Flash, with data/BSS in SRAM. Key symbols exported to C:

| Linker symbol | Used by |
|---|---|
| `_estack` | `startup_stm32f411vetx.c` (initial MSP), `sysmem.c` (stack guard) |
| `_end` | `sysmem.c` (heap start — first address after `.bss`) |
| `_Min_Stack_Size` | `sysmem.c` (reserved stack headroom for `_sbrk` guard) |
| `_sidata`, `_sdata`, `_edata` | `Reset_Handler` (copy `.data` initialised values from Flash to RAM) |
| `_sbss`, `_ebss` | `Reset_Handler` (zero-fill `.bss`) |

The `__SECTION_BOOT` and `__SECTION_BOOT_DATA` attributes (from `def_attributes.h`) place critical pre-scheduler code and data into dedicated linker sections so they land in low Flash / RAM addresses:

```c
#define __SECTION_BOOT       __attribute__((section(".boot_text")))
#define __SECTION_BOOT_DATA  __attribute__((section(".boot_data")))
```

---

## Kernel Glue (`kernel/`)

These two files provide the C runtime integration that Newlib requires on a bare-metal ARM target. They are architecture-agnostic — they use only standard POSIX/C headers and linker symbols, so they compile unchanged for any arm-none-eabi target.

### Newlib Syscall Stubs (`syscalls.c`)

**File:** [`kernel/syscalls.c`](kernel/syscalls.c)

Newlib's `printf`, `malloc`, `fopen`, and related functions call a set of low-level POSIX hooks. On a hosted OS these hooks are supplied by the kernel. On bare metal they must be provided by the application. `syscalls.c` supplies minimal stubs that satisfy the linker without implementing actual I/O:

| Function | Behaviour |
|---|---|
| `_write(file, ptr, len)` | Calls `__io_putchar()` per byte — `__weak`, override to redirect stdout to UART |
| `_read(file, ptr, len)` | Calls `__io_getchar()` per byte — `__weak`, override for UART stdin |
| `_close`, `_lseek`, `_fstat`, `_isatty` | Return sentinel values (no filesystem) |
| `_open`, `_unlink`, `_link` | Set `errno = ENOENT / EMLINK`, return -1 |
| `_fork`, `_execve` | Set `errno = EAGAIN / ENOMEM`, return -1 |
| `_getpid` | Returns 1 |
| `_kill(pid, sig)` | Sets `errno = EINVAL`, returns -1 |
| `_exit(status)` | Calls `_kill`, then spins forever |
| `_times`, `_wait` | Set `errno = ECHILD`, return -1 |
| `initialise_monitor_handles` | Empty stub (semihosting placeholder) |

`__io_putchar` and `__io_getchar` are declared `__weak` so board-level code can provide UART-backed implementations without modifying this file.

**Override example (in a board-level file):**

```c
int __io_putchar(int ch)
{
    drv_serial_transmit(UART_DBG, (uint8_t *)&ch, 1);
    return ch;
}
```

---

### Heap Glue (`sysmem.c`)

**File:** [`kernel/sysmem.c`](kernel/sysmem.c)

`_sbrk(incr)` is the heap extension hook used by `malloc` and the C library. It grows the heap upward from `_end` (top of `.bss`) while guarding against encroaching on the reserved MSP stack:

```c
void *_sbrk(ptrdiff_t incr)
{
    // heap grows from _end upward
    // stack limit = _estack - _Min_Stack_Size
    // returns (void *)-1 with errno = ENOMEM if heap would exceed stack
}
```

> **Note:** FreeRTOS heap (`heap_4.c`) does not use `_sbrk`. Only Newlib `malloc` / `new` calls hit this path. Prefer `pvPortMalloc` inside tasks.

---

## Layer 1 — Generic Handle (`drv_handle.h`)

**File:** [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h)

The single most important file in the driver subsystem. Everything else depends on it. It defines:

1. Forward-declared handle types
2. Vendor hardware context types (selected by `CONFIG_DEVICE_VARIANT`)
3. HAL ops vtables (function pointer structs)
4. Concrete handle structs that embed context + vtable pointer
5. Registration and accessor API declarations

### Handle Structs

Each peripheral class has one generic handle struct. The generic driver layer owns a static array of these; upper layers access them only through the public driver API.

```c
/* UART */
struct drv_uart_handle {
    uint8_t                   dev_id;
    uint8_t                   initialized;
    drv_hw_uart_ctx_t         hw;           // vendor hardware context
    const drv_uart_hal_ops_t *ops;          // bound HAL ops vtable
    uint32_t                  baudrate;
    uint32_t                  timeout_ms;
    int32_t                   last_err;
};

/* I2C */
struct drv_iic_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_iic_ctx_t         hw;
    const drv_iic_hal_ops_t *ops;
    uint32_t                 clock_hz;
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/* SPI */
struct drv_spi_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_spi_ctx_t         hw;
    const drv_spi_hal_ops_t *ops;
    uint32_t                 clock_hz;
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/* GPIO */
struct drv_gpio_handle {
    uint8_t                   dev_id;
    uint8_t                   initialized;
    drv_hw_gpio_ctx_t         hw;
    const drv_gpio_hal_ops_t *ops;
    int32_t                   last_err;
};

/* Timer — per-instance (up to NO_OF_TIMER instances) */
struct drv_timer_handle {
    uint8_t                    dev_id;
    uint8_t                    initialized;
    drv_hw_timer_ctx_t         hw;
    const drv_timer_hal_ops_t *ops;
    int32_t                    last_err;
};

/* Watchdog — singleton (one per MCU, no dev_id array) */
struct drv_wdg_handle {
    uint8_t                  initialized;
    drv_hw_wdg_ctx_t         hw;
    const drv_wdg_hal_ops_t *ops;
    int32_t                  last_err;
};
```

Initialiser macros zero-fill a handle and set `dev_id`:

```c
#define DRV_TIMER_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = 0 }

#define DRV_WDG_HANDLE_INIT() \
    { .initialized = 0, .ops = NULL, .last_err = 0 }
```

### HAL Ops Vtables

Each peripheral class has one vtable. The generic driver layer only calls through this table — it never calls `HAL_UART_Transmit()` or any vendor function directly.

```c
/* UART vtable */
typedef struct drv_uart_hal_ops {
    int32_t (*hw_init)    (drv_uart_handle_t *h);
    int32_t (*hw_deinit)  (drv_uart_handle_t *h);
    int32_t (*transmit)   (drv_uart_handle_t *h, const uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);
    int32_t (*receive)    (drv_uart_handle_t *h, uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);
    int32_t (*start_rx_it)(drv_uart_handle_t *h, uint8_t *rx_byte);
    void    (*rx_isr_cb)  (drv_uart_handle_t *h, uint8_t rx_byte, void *rb);
} drv_uart_hal_ops_t;

/* I2C vtable */
typedef struct drv_iic_hal_ops {
    int32_t (*hw_init)        (drv_iic_handle_t *h);
    int32_t (*hw_deinit)      (drv_iic_handle_t *h);
    int32_t (*transmit)       (drv_iic_handle_t *h, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               const uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
    int32_t (*receive)        (drv_iic_handle_t *h, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
    int32_t (*is_device_ready)(drv_iic_handle_t *h, uint16_t dev_addr,
                               uint32_t timeout_ms);
} drv_iic_hal_ops_t;

/* SPI vtable */
typedef struct drv_spi_hal_ops {
    int32_t (*hw_init)  (drv_spi_handle_t *h);
    int32_t (*hw_deinit)(drv_spi_handle_t *h);
    int32_t (*transmit) (drv_spi_handle_t *h, const uint8_t *data,
                         uint16_t len, uint32_t timeout_ms);
    int32_t (*receive)  (drv_spi_handle_t *h, uint8_t *data,
                         uint16_t len, uint32_t timeout_ms);
    int32_t (*transfer) (drv_spi_handle_t *h, const uint8_t *tx,
                         uint8_t *rx, uint16_t len, uint32_t timeout_ms);
} drv_spi_hal_ops_t;

/* GPIO vtable */
typedef struct drv_gpio_hal_ops {
    int32_t (*hw_init)  (drv_gpio_handle_t *h);
    int32_t (*hw_deinit)(drv_gpio_handle_t *h);
    void    (*set)      (drv_gpio_handle_t *h);
    void    (*clear)    (drv_gpio_handle_t *h);
    void    (*toggle)   (drv_gpio_handle_t *h);
    uint8_t (*read)     (drv_gpio_handle_t *h);
    void    (*write)    (drv_gpio_handle_t *h, uint8_t state);
} drv_gpio_hal_ops_t;

/* Timer vtable */
typedef struct drv_timer_hal_ops {
    int32_t  (*hw_init)     (drv_timer_handle_t *h);
    int32_t  (*hw_deinit)   (drv_timer_handle_t *h);
    uint32_t (*get_counter) (drv_timer_handle_t *h);
    void     (*set_counter) (drv_timer_handle_t *h, uint32_t val);
} drv_timer_hal_ops_t;

/* Watchdog vtable (singleton — no dev_id) */
typedef struct drv_wdg_hal_ops {
    int32_t (*hw_init)  (drv_wdg_handle_t *h);
    int32_t (*hw_deinit)(drv_wdg_handle_t *h);
    void    (*refresh)  (drv_wdg_handle_t *h);
} drv_wdg_hal_ops_t;
```

### Vendor Hardware Context

The `hw` field inside each handle embeds the vendor-specific hardware state. `CONFIG_DEVICE_VARIANT` selects which concrete type is compiled — the generic layer treats it as opaque.

| `CONFIG_DEVICE_VARIANT` | Type | STM32 contents | Infineon contents |
|---|---|---|---|
| — | `drv_hw_uart_ctx_t` | `UART_HandleTypeDef huart` | `uint32_t channel_base; uint32_t baudrate` |
| — | `drv_hw_iic_ctx_t` | `I2C_HandleTypeDef hi2c` | `uint32_t channel_base; uint32_t clock_hz` |
| — | `drv_hw_spi_ctx_t` | `SPI_HandleTypeDef hspi` | `uint32_t channel_base; uint32_t clock_hz` |
| — | `drv_hw_gpio_ctx_t` | `GPIO_TypeDef *port; uint16_t pin; uint32_t mode/pull/speed; uint8_t active_state` | `uint32_t port_base; uint32_t pin; …` |
| — | `drv_hw_timer_ctx_t` | `TIM_HandleTypeDef htim` (under `HAL_TIM_MODULE_ENABLED`) | `uint32_t timer_base; uint32_t period` |
| — | `drv_hw_wdg_ctx_t` | `IWDG_HandleTypeDef hiwdg` (under `HAL_IWDG_MODULE_ENABLED`) | `uint32_t wdg_base; uint32_t timeout_ms` |

### Registration API

```c
/* Bind an ops vtable and call hw_init() */
int32_t drv_uart_register (uint8_t dev_id, const drv_uart_hal_ops_t *ops,
                            uint32_t baudrate, uint32_t timeout_ms);
int32_t drv_iic_register  (uint8_t dev_id, const drv_iic_hal_ops_t  *ops,
                            uint32_t clock_hz, uint32_t timeout_ms);
int32_t drv_spi_register  (uint8_t dev_id, const drv_spi_hal_ops_t  *ops,
                            uint32_t clock_hz, uint32_t timeout_ms);
int32_t drv_gpio_register (uint8_t dev_id, const drv_gpio_hal_ops_t *ops);
int32_t drv_timer_register(uint8_t dev_id, const drv_timer_hal_ops_t *ops);
int32_t drv_wdg_register  (const drv_wdg_hal_ops_t *ops);   // singleton — no dev_id

/* Retrieve a handle by device index (NULL if out of range) */
drv_uart_handle_t  *drv_uart_get_handle (uint8_t dev_id);
drv_iic_handle_t   *drv_iic_get_handle  (uint8_t dev_id);
drv_spi_handle_t   *drv_spi_get_handle  (uint8_t dev_id);
drv_gpio_handle_t  *drv_gpio_get_handle (uint8_t dev_id);
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id);
drv_wdg_handle_t   *drv_wdg_get_handle  (void);             // singleton
```

All registration functions return `OS_ERR_NONE` (0) on success or a negative `OS_ERR_*` code on failure.

---

## Layer 2 — Vendor HAL Backends

These files contain all vendor-specific code. Headers live in `include/drivers/`. Sources live in `drivers/hal/<vendor>/`. All files are guarded with `#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)`.

### STM32 HAL Backend

---

#### UART — hal_uart_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_uart_stm32.h`](include/drivers/com/hal/stm32/hal_uart_stm32.h) |
| Source | [`drivers/hal/stm32/hal_uart_stm32.c`](drivers/hal/stm32/hal_uart_stm32.c) |

**API:**

```c
const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void);

void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef *instance,
                               uint32_t word_len, uint32_t stop_bits,
                               uint32_t parity, uint32_t mode);

/* ISR dispatcher — called by hal_it_stm32.c */
void hal_uart_stm32_irq_handler(USART_TypeDef *instance);
```

**Underlying STM32 HAL calls made by the ops table:**

| Op | STM32 HAL call |
|---|---|
| `hw_init` | `HAL_UART_Init()` |
| `hw_deinit` | `HAL_UART_DeInit()` |
| `transmit` | `HAL_UART_Transmit()` |
| `receive` | `HAL_UART_Receive()` |
| `start_rx_it` | `UART_Start_Receive_IT()` |
| `rx_isr_cb` | `HAL_UART_Receive_IT()` + `ringbuffer_putchar()` |

**IRQ dispatch:** `hal_uart_stm32_irq_handler(USART_TypeDef *instance)` walks the handle array to find the matching device and calls `HAL_UART_IRQHandler()` on its `huart`. This keeps `hal_it_stm32.c` free of any direct handle references.

```c
void hal_uart_stm32_irq_handler(USART_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_UART; id++) {
        drv_uart_handle_t *h = drv_uart_get_handle(id);
        if (h && h->initialized && h->hw.huart.Instance == instance) {
            HAL_UART_IRQHandler(&h->hw.huart);
            return;
        }
    }
}
```

**Usage example:**

```c
drv_uart_handle_t *h = drv_uart_get_handle(UART_1);
hal_uart_stm32_set_config(h, USART1,
                          UART_WORDLENGTH_8B, UART_STOPBITS_1,
                          UART_PARITY_NONE,   UART_MODE_TX_RX);
drv_uart_register(UART_1, hal_uart_stm32_get_ops(), 115200, 10);
```

---

#### I2C — hal_iic_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_iic_stm32.h`](include/drivers/com/hal/stm32/hal_iic_stm32.h) |
| Source | [`drivers/hal/stm32/hal_iic_stm32.c`](drivers/hal/stm32/hal_iic_stm32.c) |

**API:**

```c
const drv_iic_hal_ops_t *hal_iic_stm32_get_ops(void);

void hal_iic_stm32_set_config(drv_iic_handle_t *h,
                              I2C_TypeDef *instance,
                              uint32_t addr_mode, uint32_t dual_addr);
```

**Underlying STM32 HAL calls:**

| Op | `use_reg == 0` | `use_reg == 1` |
|---|---|---|
| `transmit` | `HAL_I2C_Master_Transmit()` | `HAL_I2C_Mem_Write()` |
| `receive` | `HAL_I2C_Master_Receive()` | `HAL_I2C_Mem_Read()` |
| `is_device_ready` | `HAL_I2C_IsDeviceReady()` (3 retries) | — |

Device addresses are **7-bit** (not shifted); the HAL backend applies the `<< 1` shift internally.

---

#### SPI — hal_spi_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_spi_stm32.h`](include/drivers/com/hal/stm32/hal_spi_stm32.h) |
| Source | [`drivers/hal/stm32/hal_spi_stm32.c`](drivers/hal/stm32/hal_spi_stm32.c) |

**API:**

```c
const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void);

void hal_spi_stm32_set_config(drv_spi_handle_t *h,
                               SPI_TypeDef *instance,
                               uint32_t mode, uint32_t direction,
                               uint32_t data_size, uint32_t clk_polarity,
                               uint32_t clk_phase, uint32_t nss,
                               uint32_t baud_prescaler, uint32_t bit_order);
```

**Underlying STM32 HAL calls:**

| Op | STM32 HAL call |
|---|---|
| `hw_init` | `HAL_SPI_Init()` |
| `transmit` | `HAL_SPI_Transmit()` |
| `receive` | `HAL_SPI_Receive()` |
| `transfer` | `HAL_SPI_TransmitReceive()` (full-duplex) |

> Chip-select toggling is the caller's responsibility.

---

#### GPIO — hal_gpio_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/cpu/hal/stm32/hal_gpio_stm32.h`](include/drivers/cpu/hal/stm32/hal_gpio_stm32.h) |
| Source | [`drivers/hal/stm32/hal_gpio_stm32.c`](drivers/hal/stm32/hal_gpio_stm32.c) |

**API:**

```c
const drv_gpio_hal_ops_t *hal_gpio_stm32_get_ops(void);

void hal_gpio_stm32_set_config(drv_gpio_handle_t *h,
                                GPIO_TypeDef *port, uint16_t pin,
                                uint32_t mode, uint32_t pull,
                                uint32_t speed, uint8_t active_state);
```

`set_config` stores parameters only — does **not** call `HAL_GPIO_Init()`. That happens inside `hw_init`, which is called by `drv_gpio_register()`.

> GPIO peripheral clocks must be enabled before `hw_init()` runs. This is done in `hal_msp_stm32.c` via `HAL_MspInit()` or the board-specific `board_init()`.

---

#### MSP Callbacks — hal_msp_stm32

| File | Path |
|---|---|
| Source | [`drivers/hal/stm32/hal_msp_stm32.c`](drivers/hal/stm32/hal_msp_stm32.c) |

The ST HAL library calls `HAL_MspInit()` and per-peripheral `HAL_xxx_MspInit()` weak callbacks during `HAL_Init()` and `HAL_xxx_Init()`. These callbacks configure the GPIO alternate functions and enable the peripheral clocks.

By keeping them here (not in `arch/`) the STM32-specific peripheral setup is co-located with the rest of the STM32 HAL backend.

**Callbacks implemented:**

| Callback | Responsibility |
|---|---|
| `HAL_MspInit()` | Enable SysTick, configure global NVIC priority grouping |
| `HAL_UART_MspInit(huart)` | Enable UART GPIO alternate functions (TX/RX pins), enable UART peripheral clock, configure NVIC entry — looks up pin config via `board_find_uart()` |
| `HAL_UART_MspDeInit(huart)` | Reset peripheral, disable GPIO and clock |
| `HAL_I2C_MspInit(hi2c)` | Enable I2C GPIO open-drain AF, enable clock — looks up via `board_find_iic()` |
| `HAL_I2C_MspDeInit(hi2c)` | Symmetric teardown |
| `HAL_SPI_MspInit(hspi)` | Enable SPI GPIO AF, enable clock — looks up via `board_find_spi()` |
| `HAL_SPI_MspDeInit(hspi)` | Symmetric teardown |

The board descriptor lookups (`board_find_uart`, `board_find_iic`, `board_find_spi`) return a pointer to the board descriptor entry whose peripheral instance matches the one inside the HAL handle. This makes `hal_msp_stm32.c` board-agnostic — pin assignments live only in the board XML / board descriptor tables.

---

#### HAL Timebase — hal_timebase_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/hal/stm32/hal_timebase_stm32.h`](include/drivers/hal/stm32/hal_timebase_stm32.h) |
| Source | [`drivers/hal/stm32/hal_timebase_stm32.c`](drivers/hal/stm32/hal_timebase_stm32.c) |

The STM32 HAL library uses SysTick as its 1 ms tick source by default. FreeRTOS also uses SysTick for its scheduler tick. This conflict is resolved by overriding `HAL_InitTick()` to use **TIM1** instead, leaving SysTick entirely for FreeRTOS.

**How it works:**

1. `HAL_InitTick(priority)` is a `__weak` function defined in `stm32f4xx_hal.c`. The strong definition here replaces it.
2. TIM1 is configured to generate an update interrupt at exactly 1 kHz (1 ms period).
3. The TIM1 handle (`static TIM_HandleTypeDef _htim1`) is a module-local static — no other file needs to reference it.
4. `HAL_SuspendTick()` and `HAL_ResumeTick()` are also overridden to disable/enable the TIM1 update interrupt rather than touching SysTick.

```c
/* Called from hal_it_stm32.c TIM1_UP_TIM10_IRQHandler */
void hal_timebase_stm32_irq_handler(void);
```

The IRQ handler calls `HAL_TIM_IRQHandler(&_htim1)` which in turn calls `HAL_IncTick()`, advancing `uwTick`. `HAL_GetTick()` (used by `drv_time_get_ticks()`) reads `uwTick` — so the entire tick chain works through TIM1.

**Tick source summary:**

| Source | Used by |
|---|---|
| TIM1 (1 kHz) | STM32 HAL (`HAL_GetTick`, `HAL_Delay`, timeout logic) |
| SysTick | FreeRTOS scheduler (`xTaskGetTickCount`, `vTaskDelay`) |

---

#### ISR Dispatch — hal_it_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/hal/stm32/hal_it_stm32.h`](include/drivers/hal/stm32/hal_it_stm32.h) |
| Source | [`drivers/hal/stm32/hal_it_stm32.c`](drivers/hal/stm32/hal_it_stm32.c) |

This file is the **only** place where STM32F411xE peripheral interrupt handler names appear. Each body is a single function call to the appropriate dispatcher in the relevant HAL backend — no business logic, no direct handle references.

```c
/* STM32F4xx peripheral interrupt handlers */

void TIM1_UP_TIM10_IRQHandler(void)
{
    hal_timebase_stm32_irq_handler();    // in hal_timebase_stm32.c
}

void USART1_IRQHandler(void)
{
    hal_uart_stm32_irq_handler(USART1);  // in hal_uart_stm32.c
}

void USART2_IRQHandler(void)
{
    hal_uart_stm32_irq_handler(USART2);
}
```

The `startup_stm32f411vetx.c` vector table declares all peripheral handlers as `__weak` aliases. The strong definitions here override them at link time. Any interrupt vector that is not overridden continues to point to the default handler (infinite loop) in the startup file.

**Adding a new peripheral ISR:**

1. Declare the dispatcher function prototype in the relevant `hal_xxx_stm32.h`.
2. Implement the dispatcher in `hal_xxx_stm32.c` (walk the handle array, call `HAL_xxx_IRQHandler`).
3. Add a one-line body to `hal_it_stm32.c` calling the dispatcher.
4. Declare the ISR name in `hal_it_stm32.h` if callers outside this file need it.

---

### Infineon Baremetal Backend (Stubs)

Compiled only when `CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON`. All function bodies contain `TODO` comments with the register sequences to implement for Infineon XMC4xxx / XMC1xxx targets.

| Header | Source | Peripheral |
|---|---|---|
| [`include/drivers/com/hal/infineon/hal_uart_infineon.h`](include/drivers/com/hal/infineon/hal_uart_infineon.h) | [`drivers/hal/infineon/hal_uart_infineon.c`](drivers/hal/infineon/hal_uart_infineon.c) | USIC-UART |
| [`include/drivers/com/hal/infineon/hal_iic_infineon.h`](include/drivers/com/hal/infineon/hal_iic_infineon.h) | [`drivers/hal/infineon/hal_iic_infineon.c`](drivers/hal/infineon/hal_iic_infineon.c) | USIC-I2C |

---

## Layer 3 — Generic Driver API

These are the functions called by application code and management service threads. None reference any vendor type.

### UART Driver (`drv_uart.c`)

**Header:** [`include/drivers/com/drv_uart.h`](include/drivers/com/drv_uart.h)
**Source:** [`drivers/drv_uart.c`](drivers/drv_uart.c)

```c
int32_t drv_uart_register(uint8_t dev_id, const drv_uart_hal_ops_t *ops,
                           uint32_t baudrate, uint32_t timeout_ms);

int32_t drv_serial_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);
int32_t drv_serial_receive (uint8_t dev_id, uint8_t *data, uint16_t len);

void drv_uart_rx_isr_dispatch(uint8_t dev_id, uint8_t rx_byte);

drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id);
```

**RX data flow:**

```
UART peripheral → HAL_UART_RxCpltCallback()
                → drv_uart_rx_isr_dispatch(dev_id, byte)
                → ops->rx_isr_cb(h, byte, rb)
                → ringbuffer_putchar(rb, byte)    [IPC ring buffer]
                → uart_mgmt thread reads from ring buffer and delivers to app
```

**Peripheral count:** `NO_OF_UART` in `mcu_config.h`. Handle array is statically sized.

---

### I2C Driver (`drv_iic.c`)

**Header:** [`include/drivers/com/drv_iic.h`](include/drivers/com/drv_iic.h)
**Source:** [`drivers/drv_iic.c`](drivers/drv_iic.c)

```c
int32_t drv_iic_register(uint8_t dev_id, const drv_iic_hal_ops_t *ops,
                          uint32_t clock_hz, uint32_t timeout_ms);

int32_t drv_iic_transmit    (uint8_t dev_id, uint16_t dev_addr,
                              uint8_t reg_addr, const uint8_t *data, uint16_t len);
int32_t drv_iic_receive     (uint8_t dev_id, uint16_t dev_addr,
                              uint8_t reg_addr, uint8_t *data, uint16_t len);
int32_t drv_iic_device_ready(uint8_t dev_id, uint16_t dev_addr);

drv_iic_handle_t *drv_iic_get_handle(uint8_t dev_id);
```

**Peripheral count:** `NO_OF_IIC`.

---

### SPI Driver (`drv_spi.c`)

**Header:** [`include/drivers/com/drv_spi.h`](include/drivers/com/drv_spi.h)
**Source:** [`drivers/drv_spi.c`](drivers/drv_spi.c)

```c
int32_t drv_spi_register(uint8_t dev_id, const drv_spi_hal_ops_t *ops,
                          uint32_t clock_hz, uint32_t timeout_ms);

int32_t drv_spi_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);
int32_t drv_spi_receive (uint8_t dev_id, uint8_t *data, uint16_t len);
int32_t drv_spi_transfer(uint8_t dev_id, const uint8_t *tx, uint8_t *rx,
                          uint16_t len);

drv_spi_handle_t *drv_spi_get_handle(uint8_t dev_id);
```

---

### GPIO Driver (`drv_gpio.c`)

**Header:** [`include/drivers/cpu/drv_gpio.h`](include/drivers/cpu/drv_gpio.h)
**Source:** [`drivers/drv_gpio.c`](drivers/drv_gpio.c)

```c
int32_t drv_gpio_register  (uint8_t dev_id, const drv_gpio_hal_ops_t *ops);

int32_t drv_gpio_set_pin   (uint8_t dev_id);
int32_t drv_gpio_clear_pin (uint8_t dev_id);
int32_t drv_gpio_toggle_pin(uint8_t dev_id);
int32_t drv_gpio_write_pin (uint8_t dev_id, uint8_t state);
uint8_t drv_gpio_read_pin  (uint8_t dev_id);

drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);
```

---

### Timer / Time-base Driver (`drv_time.c`)

**Header:** [`include/drivers/timer/drv_time.h`](include/drivers/timer/drv_time.h)
**Source:** [`drivers/drv_time.c`](drivers/drv_time.c)

The timer driver has two independent sections:

**Section 1 — Tick utilities** (always available, no registration required):

```c
/* Returns HAL_GetTick() — milliseconds since reset, driven by TIM1 */
uint32_t drv_time_get_ticks(void);

/* Blocking millisecond delay via HAL_Delay */
void drv_time_delay_ms(uint32_t ms);
```

**Section 2 — Per-instance hardware timers** (compiled when `NO_OF_TIMER > 0`):

Each hardware timer instance follows the same ops-table pattern as UART/SPI/I2C. A static `_timer_handles[NO_OF_TIMER]` array holds the handles.

```c
/* Bind ops and call hw_init — e.g. HAL_TIM_Base_Init() for encoder or counter */
int32_t drv_timer_register(uint8_t dev_id, const drv_timer_hal_ops_t *ops);

/* Access a timer handle by index */
drv_timer_handle_t *drv_timer_get_handle(uint8_t dev_id);

/* Read/write the hardware counter through the ops vtable */
uint32_t drv_timer_get_counter(uint8_t dev_id);
void     drv_timer_set_counter(uint8_t dev_id, uint32_t val);
```

`NO_OF_TIMER` is set by:

```c
// include/config/mcu_config.h
#define NO_OF_TIMER   CONFIG_MCU_NO_OF_TIMER_PERIPHERAL
```

**Usage example (quadrature encoder on TIM2):**

```c
// 1. Populate hardware context
drv_timer_handle_t *h = drv_timer_get_handle(TIMER_ENC_LEFT);
h->hw.htim.Instance = TIM2;
// configure TIM2 encoder mode fields in h->hw.htim.Init …

// 2. Register
drv_timer_register(TIMER_ENC_LEFT, hal_timer_stm32_get_ops());

// 3. Use
uint32_t ticks = drv_timer_get_counter(TIMER_ENC_LEFT);
drv_timer_set_counter(TIMER_ENC_LEFT, 0);   // reset count
```

---

### CPU Utility and Watchdog Driver (`drv_cpu.c`)

**Header:** [`include/drivers/cpu/drv_cpu.h`](include/drivers/cpu/drv_cpu.h)
**Source:** [`drivers/drv_cpu.c`](drivers/drv_cpu.c)

The CPU driver has two sections: always-on CPU utilities and an optional watchdog subsystem.

**Section 1 — CPU utilities** (always compiled):

```c
/* Configure NVIC interrupt priorities for all used peripherals */
void drv_cpu_interrupt_prio_set(void);

/* Software reset via NVIC_SystemReset() */
void reset_mcu(void);
```

Additionally `drv_cpu.c` provides strong definitions for the Cortex-M4 fault handlers, overriding the default infinite-loop stubs from the startup file:

| Handler | Action |
|---|---|
| `HardFault_Handler` | Captures MSP/PSP, executes BKPT (debug) then resets |
| `MemManage_Handler` | Same pattern |
| `BusFault_Handler` | Same pattern |
| `UsageFault_Handler` | Same pattern |

**Section 2 — Watchdog (IWDG)** (compiled when `CONFIG_MCU_WDG_EN == 1`):

The watchdog is a singleton resource — there is only one IWDG peripheral per MCU, so the WDG driver uses no array and no `dev_id`.

```c
/* Bind the WDG ops vtable and call hw_init (HAL_IWDG_Init) */
int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops);

/* Retrieve the singleton handle (for direct hw access if needed) */
drv_wdg_handle_t *drv_wdg_get_handle(void);

/* Pet the watchdog — calls ops->refresh() → HAL_IWDG_Refresh() */
void drv_wdg_refresh(void);
```

When `CONFIG_MCU_WDG_EN == 0`, all three functions are compiled as no-op stubs so that call sites need no `#if` guards.

**Typical usage from the kernel management thread:**

```c
drv_wdg_register(hal_wdg_stm32_get_ops());   // once at startup

// in each management thread's main loop:
drv_wdg_refresh();
```

---

## Management Service Threads

Each peripheral class has a dedicated FreeRTOS service thread in `services/`. These threads:

1. **Self-register all peripherals** — at startup they call `board_get_config()` and iterate the board descriptor tables. For every entry they call `hal_xxx_set_config()` then `drv_xxx_register()`. No user code is required to register peripherals.
2. **Serialise hardware access** — application tasks post messages to the management queue instead of calling the driver API directly.
3. **Recover from errors** — the `REINIT` command triggers `hw_deinit()` + `hw_init()` through the management queue.

---

### UART Management — `uart_mgmt`

**Header:** [`include/services/uart_mgmt.h`](include/services/uart_mgmt.h)
**Source:** [`services/uart_mgmt.c`](services/uart_mgmt.c)

#### Thread lifecycle

```
os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT)
    │
    ├─ ops = hal_uart_stm32_get_ops()
    │   for each entry in board->uart_table:
    │       hal_uart_stm32_set_config(h, instance, …)
    │       drv_uart_register(dev_id, ops, baudrate, timeout)
    │           └─ ops->hw_init(h) → HAL_UART_Init() + start_rx_it()
    │
    └─ loop: xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY)
                ├─ UART_MGMT_CMD_TRANSMIT → ops->transmit()
                ├─ UART_MGMT_CMD_REINIT  → hw_deinit() + hw_init()
                └─ UART_MGMT_CMD_DEINIT  → hw_deinit()
```

**RX callback dispatch chain:**

```
USART1_IRQHandler() [hal_it_stm32.c]
  └─ hal_uart_stm32_irq_handler(USART1) [hal_uart_stm32.c]
       └─ HAL_UART_IRQHandler(&h->hw.huart)
            └─ HAL_UART_RxCpltCallback() [drv_uart.c]
                 └─ drv_uart_rx_isr_dispatch(dev_id, byte)
                      ├─ ringbuffer_putchar()      [IPC ring buffer]
                      └─ board_get_uart_cbs()
                             └─ on_rx_byte(dev_id, byte)  [app callback]
```

#### Message structure

```c
typedef enum {
    UART_MGMT_CMD_TRANSMIT,
    UART_MGMT_CMD_REINIT,
    UART_MGMT_CMD_DEINIT,
} uart_mgmt_cmd_t;

typedef struct {
    uart_mgmt_cmd_t  cmd;
    uint8_t          dev_id;
    const uint8_t   *data;
    uint16_t         len;
} uart_mgmt_msg_t;
```

#### Public API

```c
int32_t       uart_mgmt_start(void);
int32_t       uart_mgmt_async_transmit(uint8_t dev_id,
                                        const uint8_t *data, uint16_t len);
QueueHandle_t uart_mgmt_get_queue(void);
```

**Stack / priority:**

| Parameter | Macro | Default |
|---|---|---|
| Stack depth (words) | `PROC_SERVICE_SERIAL_MGMT_STACK_SIZE` | 512 |
| Priority | `PROC_SERVICE_SERIAL_MGMT_PRIORITY` | 1 |
| Startup delay | `TIME_OFFSET_SERIAL_MANAGEMENT` | 4000 ms |

---

### I2C Management — `iic_mgmt`

**Header:** [`include/services/iic_mgmt.h`](include/services/iic_mgmt.h)
**Source:** [`services/iic_mgmt.c`](services/iic_mgmt.c)

Iterates `board->iic_table` at startup, registers all I2C buses. Adds a **synchronous receive** path via FreeRTOS task notification.

#### Message structure

```c
typedef enum {
    IIC_MGMT_CMD_TRANSMIT,
    IIC_MGMT_CMD_RECEIVE,
    IIC_MGMT_CMD_PROBE,
    IIC_MGMT_CMD_REINIT,
} iic_mgmt_cmd_t;

typedef struct {
    iic_mgmt_cmd_t   cmd;
    uint8_t          bus_id;
    uint16_t         dev_addr;
    uint8_t          reg_addr;
    uint8_t          use_reg;
    uint8_t         *data;
    uint16_t         len;
    TaskHandle_t     result_notify;
    int32_t         *result_code;
} iic_mgmt_msg_t;
```

#### Public API

```c
int32_t iic_mgmt_start(void);
QueueHandle_t iic_mgmt_get_queue(void);

int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len);

int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
```

**Stack / priority:**

| Parameter | Macro | Default |
|---|---|---|
| Stack depth | `PROC_SERVICE_IIC_MGMT_STACK_SIZE` | 1024 |
| Priority | `PROC_SERVICE_IIC_MGMT_PRIORITY` | 1 |
| Startup delay | `TIME_OFFSET_IIC_MANAGEMENT` | 6500 ms |

---

### SPI Management — `spi_mgmt`

**Header:** [`include/services/spi_mgmt.h`](include/services/spi_mgmt.h)
**Source:** [`services/spi_mgmt.c`](services/spi_mgmt.c)

Iterates `board->spi_table` at startup. Supports both fire-and-forget async transmit and blocking full-duplex transfer with task-notification synchronisation.

```c
int32_t       spi_mgmt_start(void);
QueueHandle_t spi_mgmt_get_queue(void);

int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len);

int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms);
```

> CS pin must be asserted by the caller before posting the message and deasserted after `spi_mgmt_sync_transfer()` returns.

---

### GPIO Management — `gpio_mgmt`

**Header:** [`include/services/gpio_mgmt.h`](include/services/gpio_mgmt.h)
**Source:** [`services/gpio_mgmt.c`](services/gpio_mgmt.c)

At startup, for each entry in `board->gpio_table`:
1. `board_gpio_clk_enable(port)` — enables RCC peripheral clock
2. `hal_gpio_stm32_set_config(h, port, pin, mode, pull, speed, active_state)` — stores params
3. `drv_gpio_register(dev_id, ops)` — calls `hw_init()` → `HAL_GPIO_Init()`
4. Applies `initial_state` for output pins configured as initially active

Handles asynchronous and **delayed** commands (e.g. "assert reset for 10 ms then release").

```c
int32_t       gpio_mgmt_start(void);
QueueHandle_t gpio_mgmt_get_queue(void);

int32_t gpio_mgmt_post(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                        uint8_t state, uint32_t delay_ms);
```

```c
typedef enum {
    GPIO_MGMT_CMD_SET,
    GPIO_MGMT_CMD_CLEAR,
    GPIO_MGMT_CMD_TOGGLE,
    GPIO_MGMT_CMD_WRITE,
    GPIO_MGMT_CMD_REINIT,
} gpio_mgmt_cmd_t;
```

**Example — timed reset pulse:**

```c
gpio_mgmt_post(GPIO_ID_RESET, GPIO_MGMT_CMD_CLEAR, 0, 0);   // assert immediately
gpio_mgmt_post(GPIO_ID_RESET, GPIO_MGMT_CMD_SET,   0, 10);  // release after 10 ms
```

---

## External Chip Drivers

Device-specific drivers sit above the generic driver layer in `include/drivers/ext_chips/`. They use `drv_iic_transmit()` / `drv_iic_receive()` or `drv_spi_transfer()` and expose a chip-specific API.

| Driver | Protocol | IC | Purpose |
|---|---|---|---|
| `PCA9685_IIC` | I2C | NXP PCA9685 | 16-channel PWM controller |
| `MCP45HVX1` | I2C | Microchip MCP45HVX1 | High-voltage digital potentiometer |
| `M95M02_SPI` | SPI | ST M95M02 | 2 Mbit SPI EEPROM |
| `MCP3427_IIC` | I2C | Microchip MCP3427 | 16-bit delta-sigma ADC |
| `MCP23017_IIC` | I2C | Microchip MCP23017 | 16-bit GPIO expander |
| `MCP4441_IIC` | I2C | Microchip MCP4441 | Quad 8-bit digital potentiometer |
| `INA230_IIC` | I2C | TI INA230 | Power monitor (voltage + current) |
| `DS3202_IIC` | I2C | Maxim DS3502 | 7-bit digital potentiometer |

---

## Adding a New Vendor

1. **Add a `MCU_VAR_*` constant** in [`include/config/mcu_config.h`](include/config/mcu_config.h).

2. **Extend the `drv_hw_xxx_ctx_t` structs** in [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h) with a new `#elif` branch for the vendor's hardware register types.

3. **Create HAL backend files** under `drivers/hal/<vendor>/`:
   - Implement all vtable function pointers.
   - Return `OS_ERR_NONE` / `OS_ERR_OP` from every function.
   - Add `hal_msp_<vendor>.c`, `hal_timebase_<vendor>.c`, `hal_it_<vendor>.c` following the STM32 pattern.

4. **Place headers** in the correct `include/drivers/` subdirectory.

5. **Update `drivers/Makefile`** with a new `ifeq` block for the new MCU.

6. **Update the management threads** to include and select `hal_xxx_<vendor>_get_ops()` under a new `#elif`.

---

## Adding a New Peripheral Driver

1. **Create a vtable struct** (`drv_xxx_hal_ops_t`) in [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h).

2. **Add a vendor hardware context** type and forward-declare the handle.

3. **Implement the generic driver** in `drivers/drv_xxx.c` — static handle array (or singleton for one-per-MCU resources), `drv_xxx_register()`, `drv_xxx_get_handle()`, and the public API functions.

4. **Add a public API header** in the correct `include/drivers/<category>/` subfolder.

5. **Implement vendor HAL backends** in `drivers/hal/<vendor>/hal_xxx_<vendor>.{h,c}`.

6. **Create a management thread** in `services/xxx_mgmt.{h,c}`.

7. **Update Makefiles:**
   - `drivers/Makefile` — add `drv_xxx.o` and the HAL `.o` entries.
   - `services/Makefile` — add `xxx_mgmt.o` under the appropriate Kconfig guard.

---

## Include Path Reference

| Makefile `-I` flag | Covers |
|---|---|
| `-I$(CURDIR)/include` | All project headers (`<drivers/drv_handle.h>`, `<services/uart_mgmt.h>`, …) |
| `-I$(CURDIR)/include/config` | `<autoconf.h>`, `<mcu_config.h>`, `<os_config.h>` |
| `-I$(CURDIR)` | Cross-directory driver-layer includes (`<drivers/com/hal/stm32/…>`) |
| `-I$(CURDIR)/arch/devices/STM/stm32f4xx-hal-driver/Inc` | STM32 HAL headers |
| `-I$(CURDIR)/arch/devices/STM/STM32F4xx` | `<device.h>`, `<stm32f4xx.h>` |
| `-I$(CURDIR)/arch/arm/CMSIS_6` | ARM CMSIS-Core |
| `-I$(CURDIR)/kernel/FreeRTOS-Kernel/include` | FreeRTOS public headers |
| `-I$(CURDIR)/kernel/FreeRTOS-Kernel/portable/GCC/ARM_CM4F` | FreeRTOS Cortex-M4 port |

---

## Kconfig Flags That Affect Drivers

Run `make menuconfig` to configure these interactively.

| Kconfig symbol | C macro | Effect |
|---|---|---|
| `CONFIG_TARGET_MCU` | (string) | Selects which arch/ objects and linker script to compile |
| `CONFIG_DEVICE_VARIANT` | `CONFIG_DEVICE_VARIANT` | Selects STM32 vs Infineon HAL backend at compile time |
| `CONFIG_INC_SERVICE_UART_MGMT` | `INC_SERVICE_UART_MGMT` | Compiles `uart_mgmt.c` and starts the thread |
| `CONFIG_INC_SERVICE_IIC_MGMT` | `INC_SERVICE_IIC_MGMT` | Compiles `iic_mgmt.c` and starts the thread |
| `CONFIG_HAL_UART_MODULE_ENABLED` | `HAL_UART_MODULE_ENABLED` | Enables STM32 HAL UART module |
| `CONFIG_HAL_I2C_MODULE_ENABLED` | `HAL_I2C_MODULE_ENABLED` | Enables STM32 HAL I2C module |
| `CONFIG_HAL_SPI_MODULE_ENABLED` | `HAL_SPI_MODULE_ENABLED` | Enables STM32 HAL SPI module |
| `CONFIG_HAL_TIM_MODULE_ENABLED` | `HAL_TIM_MODULE_ENABLED` | Enables timer driver and `drv_hw_timer_ctx_t.htim` |
| `CONFIG_HAL_IWDG_MODULE_ENABLED` | `CONFIG_MCU_WDG_EN` | Enables WDG driver and `drv_hw_wdg_ctx_t.hiwdg` |
| `CONFIG_MCU_NO_OF_TIMER_PERIPHERAL` | `NO_OF_TIMER` | Number of hardware timer instances |

> `spi_mgmt.c` and `gpio_mgmt.c` are always compiled. C-level guards (`#if BOARD_SPI_COUNT > 0`) eliminate dead code when no buses are defined in the board XML.

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
