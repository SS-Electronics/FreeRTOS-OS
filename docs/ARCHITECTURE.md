# FreeRTOS-OS — Architecture Reference

This document is the deep-dive companion to the top-level
[`README.md`](../README.md).  README is the *user* guide (what to type,
which directories to touch).  This document is the *architect's*
guide — every layer is dissected, the critical invariants are named,
and every non-obvious design choice is justified.

---

## Contents

1. [Scope & Goals](#scope--goals)
2. [Repository Layout](#repository-layout)
3. [Layer Model](#layer-model)
4. [Build Modes — Standalone vs. With-App](#build-modes--standalone-vs-with-app)
5. [Code Generation Pipeline](#code-generation-pipeline)
6. [Driver Layer](#driver-layer)
7. [Service Threads](#service-threads)
8. [IRQ Subsystem](#irq-subsystem)
9. [IPC](#ipc)
10. [Memory Management](#memory-management)
11. [Boot Sequence (Boot FSM)](#boot-sequence-boot-fsm)
12. [Medical-Grade Safety Layer](#medical-grade-safety-layer)
13. [Logging](#logging)
14. [Shell](#shell)
15. [Architecture Support — `/arch`](#architecture-support--arch)
16. [Adding a New MCU](#adding-a-new-mcu)
17. [Adding a New Vendor](#adding-a-new-vendor)
18. [Critical Invariants](#critical-invariants)
19. [Known Bug Catalogue](#known-bug-catalogue)

---

## Scope & Goals

FreeRTOS-OS is a thin OS layer above the FreeRTOS kernel that provides
the things FreeRTOS itself deliberately omits:

  - a Linux-inspired driver model with vendor-agnostic vtables;
  - service threads that own peripherals and accept commands via
    IPC queues;
  - an interactive shell over UART_APP (FreeRTOS-Plus-CLI);
  - a code-generation pipeline that turns `board/*.xml` into BSP, IRQ
    tables, and STM32 HAL configuration;
  - a medical-grade safety stack (IWDG + software watchdog + safe-state
    shutdown + .noinit fault record) suitable for IEC 62304 / 60601-1
    submissions.

It does **not** replace the FreeRTOS kernel or schedule tasks itself.

---

## Repository Layout

```
FreeRTOS-OS/
├── arch/                 CPU + MCU vendor support  (see arch/README.md)
├── boot/                 Reserved for ROM bootloader hooks
├── com_stack/            Optional comm stacks (ROS, CANopen, …)
├── config/               Kconfig outputs (.config, autoconf.h, autoconf.mk)
├── demo/                 Self-contained "standalone OS" application shim
├── docs/                 Hand-written documentation (this file lives here)
├── drv_app/              Application helpers wrapping driver init
├── drv_ext_chips/        Off-MCU peripheral drivers (sensors, EEPROM, …)
├── drivers/              Vendor-agnostic driver vtables + HAL backends
│   └── hal/
│       ├── stm32/        STM32 HAL backend
│       └── infineon/     Infineon HAL backend (work-in-progress)
├── include/              Public headers (mirrors source layout)
├── init/                 main.c — boot FSM, scheduler entry
├── ipc/                  Ring buffer + xQueue message queues
├── irq/                  Software IRQ table + irq_desc chain
├── kernel/               FreeRTOS-Kernel submodule + glue (kernel_thread.c, syscalls.c, …)
├── lib/                  First-party + vendored libraries (CMSIS-DSP, lib-fsm, …)
├── log/                  Structured ring-buffer logger
├── mm/                   Heap helpers, list.h, DMA pool
├── safety/               IWDG, software watchdog, safe-state
├── scripts/              All Python / shell generators + installers
├── services/             Service threads (UART/I2C/SPI/GPIO/ADC, shell mgmt)
├── shell/                FreeRTOS-Plus-CLI integration
└── tools/                Stand-alone host utilities (rare)
```

---

## Layer Model

```
┌──────────────────────────────────────────────────────────────────────┐
│                     User Application (app/)                          │
│   app_main() · os_thread_create() · ipc_mqueue_send()                │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│                          Service Layer                               │
│   UART / I2C / SPI / GPIO / ADC management threads                   │
│   OS Shell (FreeRTOS+CLI, ring-buffer I/O on UART_APP)               │
└──────────────┬────────────────────────────────┬──────────────────────┘
               │                                │
┌──────────────▼──────────┐     ┌───────────────▼──────────────────────┐
│       OS Kernel API     │     │            IPC Subsystem             │
│  os_thread_create()     │     │   ipc_mqueue_register()              │
│  os_thread_delete()     │     │   ipc_mqueue_send_item()             │
│  os_suspend_thread()    │     │   ipc_mqueue_receive_item()          │
│  os_resume_thread()     │     │   Ring buffer (HW byte streams)      │
│  os_thread_delay()      │     │   xQueue   (inter-task messages)     │
└──────────────┬──────────┘     └───────────────┬──────────────────────┘
               │                                │
┌──────────────▼────────────────────────────────▼──────────────────────┐
│                    Memory Management (mm)                            │
│   Intrusive doubly-circular linked list (Linux list_head style)      │
│   kmalloc() / kfree() — wrappers over pvPortMalloc                   │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│                       FreeRTOS Kernel                                │
│   Tasks · Queues · Mutexes · Semaphores · Timers · heap_4            │
│   Portable layer: GCC/ARM_CM4F or GCC/ARM_CM7/r0p1                   │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│                  Driver Layer (drivers/)                             │
│   drv_uart / drv_iic / drv_spi / drv_gpio / drv_adc / drv_dma        │
│   Vendor-agnostic vtables; CONFIG_DEVICE_VARIANT selects the backend │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│                 HAL Backend (drivers/hal/<vendor>/)                  │
│   STM32 HAL  or  Infineon XMC (stub)                                 │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│                  Board Configuration (BSP)                           │
│   app/board/*.xml  →  gen_board_config.py / gen_irq_table.py  →      │
│   board_config.c · board_device_ids.h · irq_hw_init_generated.c      │
└────────────────────────────┬─────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────┐
│               Architecture Support (arch/)                           │
│   CMSIS-Core · MCU vendor SDK · startup_*.c · linker script          │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Build Modes — Standalone vs. With-App

There are two top-level build modes; the same OS tree services both.

### 1. Standalone OS — `make os TARGET=<mcu>`

  - `APP_DIR` defaults to `demo/`, which ships a minimal `app_main`
    plus the per-target board XML / Kconfig preset.
  - Used by CI to prove the OS builds for every supported MCU
    without needing an out-of-tree application.
  - Output: `build/<target>.elf` (e.g. `build/stm32f411.elf`,
    `build/stm32h723.elf`).
  - Each `make os` invocation chains: board-gen → config-outputs →
    clean → compile.

### 2. With-App — `make app TARGET=<mcu>` (or `make all APP_DIR=...`)

  - `APP_DIR` points at a sibling repository's `app/` directory.
  - The application supplies its own `app_main`, board XML, IRQ table,
    and Kconfig preset under `app/board/` and `app/`.
  - Used by ECG device-full-stack, Cookster, etc.
  - The OS tree itself is added as a git submodule of the consumer
    repository; nothing in `FreeRTOS-OS/` changes per project.

The dual-mode contract lives in the top-level `Makefile`:

  - `BOARD_XML / BOARD_BSP_*` resolve under `$(APP_DIR)/board/` when
    `APP_DIR` is set, otherwise the standalone stubs in `include/board/`
    apply.
  - `OBJS` always includes the OS's own `obj-y`; `APP_OBJS` is added
    only when `APP_DIR` is set.

---

## Code Generation Pipeline

Three independent generators turn human-edited inputs into C source:

| Generator | Input | Output |
|---|---|---|
| `scripts/gen_stm32_hal_conf.py` | `config/autoconf.h` | `stm32{f4,h7}xx_hal_conf.h` |
| `scripts/gen_board_config.py` | `<app>/board/<board>.xml` | `board_config.{c,h}`, `board_device_ids.h`, `board_handles.h`, `mcu_config.h` |
| `scripts/gen_irq_table.py` | `<app>/board/irq_table.xml` | `irq/irq_table.c`, `irq_hw_init_generated.{c,h}`, `irq_periph_handlers_generated.h`, `irq_periph_vectors_generated.inc` |
| `scripts/arm_dsp_gen.py` | `<app>/board/dsp_dev.xml` | `dsp_config.h`, `dsp_config.mk` |

Plus `scripts/gen_active_debug.py` which reads `CONFIG_TARGET_MCU` from
`autoconf.h` and writes `build/active_debug.cfg` + a symlink at
`arch/debug/target/active_target.svd` for the VS Code peripheral viewer.

All generators are idempotent — rerunning with unchanged inputs produces
byte-identical outputs.  CI verifies this so generator drift is caught.

---

## Driver Layer

The driver layer (`drivers/drv_*.c` + `include/drivers/drv_*.h`) defines
vendor-agnostic data types and a vtable per peripheral class.  Concrete
implementations live under `drivers/hal/<vendor>/`.

```c
typedef struct
{
    int (*init)        (drv_handle_t *h);
    int (*deinit)      (drv_handle_t *h);
    int (*transmit)    (drv_handle_t *h, const uint8_t *buf, size_t n, uint32_t timeout);
    int (*receive_it)  (drv_handle_t *h, uint8_t *buf, size_t n);
    /* … */
} drv_uart_ops_t;
```

`CONFIG_DEVICE_VARIANT` selects which backend's vtables get linked in:

| Value | Backend |
|---|---|
| `MCU_VAR_STM` | `drivers/hal/stm32/` (production) |
| `MCU_VAR_INFINEON` | `drivers/hal/infineon/` (stubs) |

When adding a new peripheral, the rule is: **drivers/drv_*.c must never
include vendor headers.**  All vendor code lives under `drivers/hal/<vendor>/`.

---

## Service Threads

Every owned peripheral has a dedicated FreeRTOS task that:

  1. Sleeps on its inbound xQueue (`*_mgmt_post()` → command struct).
  2. Drives the underlying driver call (`drv_uart_transmit_it()` …).
  3. Waits on a `TaskNotify` event posted from the driver's IT callback.
  4. Publishes the result back on the outbound xQueue.

This isolates ISR / DMA completion handling from application code and
serialises peripheral access across tasks.

Standard priorities (from `config/conf_os.h`):

| Service | Priority | Stack | Notes |
|---|---|---|---|
| `uart_mgmt`  | 3 | 384 words | One per UART device on the board |
| `iic_mgmt`   | 3 | 384       | |
| `spi_mgmt`   | 3 | 384       | |
| `gpio_mgmt`  | 2 | 256       | EXTI dispatch + bit-bang IO |
| `adc_mgmt`   | 3 | 512       | ADC1 DMA-driven |
| `wdog`       | 4 | 512       | Above app tasks — see Safety section |
| `os_shell`   | 2 | 1024      | FreeRTOS-Plus-CLI driver |

---

## IRQ Subsystem

`irq/` implements a Linux-style two-layer IRQ model:

  - **Hardware vector table** — generated from `irq_table.xml` into
    `irq_periph_vectors_generated.inc` and linked verbatim from the
    startup file.
  - **Software IRQ descriptor chain** — `irq_desc[NR_IRQS]` from
    `irq/irq_desc.c`.  `request_irq(irqn, handler, name)` registers a
    handler;  `free_irq(irqn)` removes it.
  - **Generated NVIC initialisation** — `irq_hw_init_generated.c`
    programs each enabled IRQ's NVIC priority then enables it.

### Critical: `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY = 5`

Every peripheral IRQ priority must be **≥ 5** (numerically).  IRQs at
priority 1–4 run above the FreeRTOS BASEPRI mask and must not call any
FreeRTOS API including the `_FROM_ISR` variants.

| NVIC Priority | FreeRTOS API? | Used for |
|---|---|---|
| 1–4 | **No** — above BASEPRI mask | HAL timebase (TIM6 on H7 / TIM1 on F4) |
| 5–14 | **Yes** — `FROM_ISR` only | UART, SPI, I2C, EXTI |
| 15 | Lowest — kernel reserved | SysTick, PendSV |

Set priorities in `<app>/board/irq_table.xml`, regenerate, rebuild.

---

## IPC

`ipc/` provides two complementary primitives:

  - **`ringbuffer.h`** — lock-free SPSC byte ring buffer for raw
    hardware streams (UART RX, etc.).  No copying, no allocation.
  - **`mqueue.h`** — registry of named xQueue handles
    (`ipc_mqueue_register(name, depth, elem_size)`).  Tasks look up by
    handle and exchange typed message structs.

The registry isolates queue ownership from queue access — services
register their queues at boot; application code looks them up later
via `ipc_mqueue_lookup()`.

---

## Memory Management

`mm/` adds:

  - **Linux-style `list.h`** — intrusive doubly-circular linked list.
    Same API as Linux kernel (`list_add`, `list_for_each_entry`, …).
    Used by `irq_desc`, the IPC registry, service-thread free lists.
  - **`kernel_mem.c`** — `kmalloc()` / `kfree()` thin shims over
    `pvPortMalloc()` / `vPortFree()` so OS code can call out-of-the-box
    Linux-flavoured allocators.
  - **`dma_pool.c`** — fixed-block allocator carved from a dedicated
    DMA-safe RAM region declared in the linker script.

FreeRTOS itself uses `heap_4.c` (`configTOTAL_HEAP_SIZE` from
`FreeRTOSConfig.h`).

---

## Boot Sequence (Boot FSM)

`init/main.c` runs an explicit 11-phase state machine so every step has
an identity suitable for IEC 62304 software-unit design:

| Phase | Code | Action |
|---|---|---|
| 0 | `BOOT_HAL_INIT` | `HAL_Init()` — vendor HAL bring-up, FPU/cache enable |
| 1 | `BOOT_CLK_INIT` | Generic clock driver — PLL/HSI/HSE setup |
| 2 | `BOOT_PERIPH_CLK` | Global peripheral bus clock enable |
| 3 | `BOOT_IRQ_INIT` | NVIC priority group + generated IRQ table |
| 4 | `BOOT_IPC_INIT` | Register IPC queues |
| 5 | `BOOT_SLOG_INIT` | Ring-buffer logger — *first `LOG_*` call legal here* |
| 6 | `BOOT_PREV_FAULT_CHK` | Inspect `.noinit` records left by a prior fault / safe-state |
| 7 | `BOOT_SERVICES_INIT` | Start all enabled service threads (wdog first) |
| 8 | `BOOT_WDOG_HW_INIT` | Start IWDG — *irreversible after this point* |
| 9 | `BOOT_APP_INIT` | Call `app_main()` (creates app tasks, registers WDOG slots) |
| 10 | `BOOT_SCHEDULER` | `vTaskStartScheduler()` — never returns |

Any phase failure calls `safety_enter_safe_state(SAFE_REASON_BOOT_FAIL)`.

---

## Medical-Grade Safety Layer

The safety stack is Kconfig-gated (`CONFIG_INC_SERVICE_WDOG`,
`CONFIG_HAL_IWDG_MODULE_ENABLED`) so it costs nothing in builds that
don't need it (e.g. the F411 standalone demo).

### Components

| File | Role |
|---|---|
| `log/slog.c` + `include/log/slog.h` | 32-entry ring-buffer logger.  Severities `ERROR/WARN/INFO/DEBUG`.  Thread-safe via `__disable_irq()` (must work pre-scheduler and from ISRs — a mutex would not). |
| `safety/wdog.c` + `include/safety/wdog.h` | HW IWDG + per-task bitmask software watchdog.  `wdog_service_thread` at priority 4.  Slot constants in the header. |
| `safety/safe_state.c` + `include/safety/safe_state.h` | `safety_enter_safe_state(reason)` is noreturn: disables IRQs → writes `.noinit` record (magic `0xC0DEBEEF`) → flushes a direct-register UART message → triggers AIRCR NVIC reset. |
| `drivers/hal/stm32/stm32_exceptions.c` | Naked fault trampolines + `.noinit` `fault_record_t` (magic `0xDEADC0DE`). |
| `include/safety/fault_handler.h` | Boot-time API to query / clear the persistent fault record. |
| `include/def_err.h` | 15 error codes (`OS_ERR_NONE` … `OS_ERR_NOT_INIT`) used for IEC 62304 traceability. |

### `.noinit` RAM section

Both the fault record and the safe-state record live in a `.noinit`
section placed **after** `._user_heap_stack` in the linker script
(DTCM on H7).  The startup zero-loop walks `[__bss_start__, __bss_end__]`
only, so `.noinit` survives an NVIC soft reset.  `BOOT_PREV_FAULT_CHK`
checks both magic words at boot.

### Watchdog design

```
wdog_hw_init()    Called from main() BEFORE the scheduler.  Once IWDG
                  is enabled it CANNOT be stopped.  H7: IWDG1, LSI/64,
                  ~4 s nominal.  F4: IWDG, same prescaler.

wdog_sw_init()    Called from app_main() to register per-task bitmask
                  slots.  Slot constants live in
                  include/safety/wdog.h:
                      WDOG_SLOT_HEARTBEAT (0)
                      WDOG_SLOT_ECG_ACQ   (1)
                      WDOG_SLOT_ECG_INF   (2)
                  Mask = (1 << SLOT).

wdog_task_kick()  Called inside every critical task each iteration.
                  Sets its bitmask slot.

wdog_service_thread (priority 4, period 1000 ms):
    all slots set    →  kick IWDG + clear bitmask
    any slot missing →  increment missed_checks
    missed >= 3      →  safety_enter_safe_state(SAFE_REASON_WATCHDOG)
```

### IWDG timing caveat (H7)

H723 LSI actual frequency ≈ 38 kHz (vs 32 kHz nominal) → actual IWDG
timeout ≈ **3.37 s** (vs 4 s spec).  `WDOG_CHECK_PERIOD_MS = 1000 ms` is
the max safe kick gap.  Do **not** call `vTaskDelay(5000)` at boot
without explicitly refreshing the IWDG inside the delay — that would
trigger a hardware reset loop.

### App-task notification timeout rule

`ecg_infer_task` (and any task driven by `ulTaskNotifyTake`) must use a
timeout **< `WDOG_CHECK_PERIOD_MS`** — use 900 ms.  `portMAX_DELAY` or
2000 ms causes spurious missed-check warnings.

---

## Logging

`slog` is a 32-entry circular ring buffer.  Each entry carries:

| Field | Width | Notes |
|---|---|---|
| `tick`   | u32 | `xTaskGetTickCount()` or `HAL_GetTick()` if pre-scheduler |
| `level`  | u8  | ERROR / WARN / INFO / DEBUG |
| `module` | char[8] | Caller-supplied tag |
| `msg`    | char[56] | Pre-formatted via `vsnprintf` |

Post-scheduler, ERROR and WARN entries are echoed to `printk` immediately.
Pre-scheduler entries are buffered silently and can be dumped via the
shell command `log dump` once UART is up.

`SLOG_PRINTK_MIN_LEVEL` (Kconfig) sets the printk echo threshold —
default `1` (ERROR + WARN).

---

## Shell

`shell/shell_task_mgmt.c` wires up FreeRTOS-Plus-CLI on UART_APP.  The
service task:

  - calls `os_shell_management_putc_isr()` from the UART RX callback,
  - drains the ring buffer in 16-byte chunks,
  - invokes `FreeRTOS_CLIProcessCommand` for each `\n`-terminated line.

Built-in commands are registered in `services/os_shell_management.c`
(`ps`, `mem`, `log dump`, `reset`, …).  Application code adds commands
with `FreeRTOS_CLIRegisterCommand`.

---

## Architecture Support — `/arch`

See [`arch/README.md`](../arch/README.md) for the canonical contract.
Summary:

  - `arch/arm/` — ARM architecture (CMSIS-Core, CMSIS-NN).
  - `arch/devices/<VENDOR>/<FAMILY>/<PART>/` — per-part startup, linker,
    CMSIS chip header.
  - `arch/devices/<VENDOR>/<family>-hal-driver/` — vendor HAL SDK
    (submodule).
  - `arch/debug/{interface,target}/` — OpenOCD scripts + SVD files.

Currently shipped vendors: **STM** (production).  Placeholders for
**NXP**, **TI**, **Infineon** (partial HAL shim).

---

## Adding a New MCU

Within an already-supported vendor — e.g. adding STM32F407 to the STM
family:

  1. Drop the part directory:
     ```
     arch/devices/STM/STM32F4xx/STM32F407/
         startup_stm32f407xx.c
         stm32_f407_FLASH.ld        (KEEP(.init) / KEEP(.fini)!)
         stm32_f407xx.h             (CMSIS chip header — vendor-supplied)
     ```
  2. Add an `elif defined(STM32F407xx)` branch to
     `arch/devices/device.h`.
  3. Add an `ifeq ($(CONFIG_TARGET_MCU),"STM32F407xx")` block to
     `arch/Makefile` that mirrors the F411 block but points at the F407
     startup + linker.
  4. Set `CC_TARGET_PROP` (Cortex-M variant flags) in the top-level
     `Makefile`.
  5. Drop an OpenOCD target at `arch/debug/target/stm32_f407_debug.cfg`.
  6. Add an entry to `scripts/gen_active_debug.py:MCU_DEBUG_MAP`.

The rest of the OS is unchanged.

---

## Adding a New Vendor

See `arch/README.md` → *Adding a New Vendor — Worked Example*
(LPC55S69).  Summary of touched files:

  1. `arch/devices/<VENDOR>/<FAMILY>/...` — startup + linker + CMSIS headers.
  2. `arch/devices/<VENDOR>/<sdk>-submodule` — vendor HAL SDK.
  3. `drivers/hal/<vendor>/` — generic-driver vtable backend.
  4. `arch/devices/device.h` — vendor-detection branch.
  5. `arch/Makefile` — INCLUDES / obj-y / linker.
  6. `Makefile` — `CC_TARGET_PROP` for the Cortex-M variant.
  7. `arch/debug/target/<part>_debug.cfg`.
  8. `scripts/gen_active_debug.py:MCU_DEBUG_MAP`.
  9. `include/doxygen_groups.h` — add `@defgroup hal_<vendor>` if you
     want the generated docs to surface the new HAL.

---

## Critical Invariants

These are non-obvious and have bitten the project before; treat them as
architectural constants.

### 1. Linker scripts must `KEEP(.init)` and `KEEP(.fini)`

See `arch/README.md` → *Critical: KEEP(.init) / KEEP(.fini)*.  Discarding
`crtn.o`'s anonymous epilog causes `__libc_init_array` to fall through
into adjacent rodata and BusFault before `main()`.

### 2. ISR priority ≥ `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY` (5)

Any peripheral ISR that calls a FreeRTOS API must have NVIC priority
≥ 5.  Priorities 1–4 are above the BASEPRI mask and are FreeRTOS-unsafe.

### 3. HAL timebase IRQ uses TIM6 on H7, TIM1 on F4

`HAL_InitTick` on H7 uses `TIM6_DAC_IRQHandler` (`TIM6_DAC_IRQn = 54`),
not `TIM1_UP_IRQHandler`.  Generated dispatch table must route TIM6 to
`HAL_TIM_IRQHandler`, leave `TIM1_UP_IRQHandler` as an empty stub.

### 4. `__PACKED` / `__INLINE` / `__WEAK` guard names

`include/def_compiler.h` guards must use the `__`-prefixed names that
CMSIS uses (`#ifndef __PACKED`, not `#ifndef PACKED`).  Otherwise every
CMSIS include produces a `"__PACKED redefined"` warning and silently
overrides CMSIS's stronger `aligned(1)` definition.

### 5. `CONFIG_BOARD` is a Make variable, not a Kconfig symbol

`CONFIG_BOARD` MUST be passed on the `make` command line.  Putting it
in `.config` causes the wrong board XML to be picked up by `make all`.

### 6. `wdog_service_thread` priority 4

Above the app tasks (priority 1–2) so it can preempt a stuck app task
and detect the stall.

### 7. `slog` thread-safety uses `__disable_irq()` not a mutex

`slog` must work pre-scheduler (during boot), from ISRs, and from
fault handlers.  A FreeRTOS mutex would fail in all three contexts.
The IRQ-disabled window is ~30 instructions, well below the BASEPRI
boundary.

---

## Known Bug Catalogue

Recent debugging episodes worth preserving — `docs/OS_INSIDE.md` has
the full post-mortems.

| Date | Component | Root Cause | Fix |
|---|---|---|---|
| 2026-05-17 | Linker scripts | `*(.init)`/`*(.fini)` not in `KEEP()` → crtn.o epilog gc'd → BusFault in `__libc_init_array`. | Wrap `*(.init)` / `*(.fini)` in `KEEP(...)`. |
| 2026-05-17 | `def_compiler.h` | Header guards used bare `PACKED` etc., not `__PACKED`.  CMSIS macros redefined every TU. | Switch guards to `__PACKED` / `__INLINE` / `__WEAK`. |
| 2026-05-16 | Build system | `CONFIG_BOARD` defaults to F411; H723 ECG builds picked wrong XML. | Pass `CONFIG_BOARD=stm32h723_devboard` on every `make all`. |
| 2026-05-16 | `irq_periph_handlers_generated.h` | Only 5 active handlers declared; vector table references ~75 unused ISR names → linker error. | Emit weak alias declarations for unused vectors. |
| 2026-05-16 | `hal_iic_stm32.c` | File-level guard missing `defined(HAL_I2C_MODULE_ENABLED)`. | Add the HAL-enable conjunction; mirror `hal_spi_stm32.c`. |
| 2026-05-16 | `demo/kconfig.conf` | Missing HAL_CORTEX/PWR/EXTI/FLASH/ADC, clock CONFIGs, RTOS CONFIGs. | Rewrote with full required set. |
| 2026-05-16 | `drivers/Makefile` | `drv_adc.o` / `hal_adc_stm32.o` compiled unconditionally; F411 demo has no ADC HAL. | Gate behind `CONFIG_HAL_ADC_MODULE_ENABLED`. |
| 2026-05-16 | `hal_rcc_stm32.c` | H7-only symbols (`DRV_RCC_PERIPH_TIM6`, USART3, GPIOF) compiled on F4. | Wrap in `#if defined(STM32H7)`. |
| 2026-05-16 | `services/kernel_service_core.c` | `iic_mgmt_start()` guarded only by `BOARD_IIC_COUNT > 0`, not by the service Kconfig. | Require both `BOARD_IIC_COUNT > 0` and `CONFIG_INC_SERVICE_IIC_MGMT == 1`. |

---

*Last updated: 2026-05-19.  Maintainer: Subhajit Roy <subhajitroy005@gmail.com>.*
