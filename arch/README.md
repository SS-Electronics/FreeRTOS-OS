# `arch/` — Architecture & MCU Vendor Support

This directory holds everything the build needs to retarget FreeRTOS-OS
from one ARM Cortex-M MCU to another:

  - the **CPU architecture support** (ARM CMSIS-Core, CMSIS-NN, Cortex-M
    glue);
  - the **debug-probe / target** OpenOCD scripts;
  - one or more **vendor packages** (`devices/<VENDOR>/<FAMILY>/<PART>`),
    each shipping a CMSIS device header, a startup file, and a linker
    script.

`arch/Makefile` is the single dispatcher.  It reads `CONFIG_TARGET_MCU`
out of `autoconf.mk` and selects the right `obj-y`, `INCLUDES`,
`LINKER_SCRIPT`, `OPENOCD_TARGET`, and `OPENOCD_INTERFACE` for the
chosen part.

---

## Directory Layout

```
arch/
├── Makefile                    Dispatcher — gated by CONFIG_TARGET_MCU
├── Kconfig                     Top-level MCU & clock-tree configuration
├── README.md                   (this file)
│
├── arm/                        ARM architecture support
│   ├── CMSIS_6/                CMSIS-Core (submodule)
│   └── CMSIS-NN/               CMSIS Neural Network (submodule)
│
├── debug/                      OpenOCD probe + target scripts
│   ├── interface/              (probe-side cfg — currently uses system stlink.cfg)
│   ├── target/                 Per-MCU cfg + SVD files
│   └── README.md
│
└── devices/                    MCU vendor packages
    ├── device.h                Vendor / family selection umbrella header
    ├── STM/                    STMicroelectronics — supported (F4xx, H7xx)
    │   ├── stm32f4xx-hal-driver/   HAL SDK (submodule)
    │   ├── stm32h7xx-hal-driver/   HAL SDK (submodule)
    │   ├── STM32F4xx/
    │   │   ├── stm32f4xx.h        CMSIS family header
    │   │   ├── STM32F401/         (part) startup + linker + chip header
    │   │   └── STM32F411/         (part)
    │   └── STM32H7xx/
    │       ├── stm32h7xx.h        CMSIS family header
    │       ├── system_stm32h7xx.c CMSIS system_init
    │       └── STM32H723/         (part)
    │
    ├── NXP/                    NXP — placeholder (see NXP/README.md)
    ├── TI/                     Texas Instruments — placeholder
    └── Infineon/               Infineon — partial HAL shim in drivers/hal/infineon/
```

---

## The Per-Part Contract

For every supported MCU part the directory
`arch/devices/<VENDOR>/<FAMILY>/<PART>/` MUST provide:

| File | Role |
|---|---|
| `<part>.h` | CMSIS chip-variant header — peripheral registers, IRQ numbers, base addresses. Vendor-supplied; do not hand-edit. |
| `startup_<part>.c` (or `.s`) | Reset_Handler, vector table, .data copy + .bss clear, call to `SystemInit` and `__libc_init_array`, then `main()`. |
| `<part>_FLASH.ld` | GNU linker script — flash & RAM regions, section layout, `KEEP(.init)` / `KEEP(.fini)` wrappers (see below). |

The **family** directory `arch/devices/<VENDOR>/<FAMILY>/` provides the
shared umbrella headers (`<family>.h`, `system_<family>.c|.h`).  Some
vendors also publish a HAL SDK as a sibling submodule, e.g.
`stm32f4xx-hal-driver/` next to `STM32F4xx/`.

### Critical: `KEEP(.init)` / `KEEP(.fini)` in every linker script

GCC's bare-metal CRT splits `_init` / `_fini` across **two** input
sections — `crti.o` ships the prolog (with the symbol) and `crtn.o`
ships an anonymous epilog (`pop {r3-r7}; pop {r3}; mov lr,r3; bx lr`).
With `-Wl,--gc-sections`, the anonymous fragment from `crtn.o` is
discarded, the prolog falls through into the next code section (HAL
prescaler tables), the CPU executes garbage as Thumb, and the board
takes a BusFault inside `__libc_init_array` before `main()` runs.

Every part-level `.ld` MUST therefore wrap both `*(.init)` and
`*(.fini)` in `KEEP(...)`.  Symptom of the regression: zero VCP output,
board appears dead, OpenOCD struggles to halt.

---

## How `arch/Makefile` Dispatches

Each supported part has an `ifeq ($(CONFIG_TARGET_MCU),"<PART>")` block.
The block:

  1. Appends `-I` paths for the SDK, family, and part headers to
     `INCLUDES`.
  2. Appends `-DUSE_HAL_DRIVER` and any other build symbols to
     `SYMBOL_DEF`.
  3. Adds the startup + system + SDK objects to `obj-y`.
  4. Sets `LINKER_SCRIPT` to the per-part `.ld`.
  5. Sets `OPENOCD_TARGET` / `OPENOCD_INTERFACE` to scripts under
     `arch/debug/`.
  6. (Top-level `Makefile`) Sets `CC_TARGET_PROP` to the Cortex-M
     variant flags (`-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 …` etc.).

When you add a new part you replicate this block — usually starting by
copying the closest existing one.

---

## Adding a New Vendor — Worked Example

Suppose you want to bring up NXP LPC55S69 (Cortex-M33 dual-core).

  1. Drop the LPC55xx CMSIS / SDK as a submodule:
     ```bash
     git submodule add https://github.com/nxp-mcuxpresso/LPC55S69 \
         arch/devices/NXP/lpc55s69-sdk
     ```
  2. Stage the family + part headers:
     ```
     arch/devices/NXP/LPC55xx/
         LPC55S69.h                CMSIS family header
         LPC55S69/
             LPC55S69_cm33_core0.h CMSIS chip header (CM33 core 0)
             startup_LPC55S69.c    Reset_Handler + vector table
             LPC55S69_FLASH.ld     Linker script (KEEP(.init), KEEP(.fini)!)
     ```
  3. Implement the generic driver vtables under
     `drivers/hal/nxp/` — mirror the structure of `drivers/hal/stm32/`.
  4. Add a dispatch block to `arch/Makefile`:
     ```make
     ifeq ($(CONFIG_TARGET_MCU),"LPC55S69")
     INCLUDES   += -I$(CURDIR)/arch/devices/NXP/LPC55xx
     INCLUDES   += -I$(CURDIR)/arch/devices/NXP/LPC55xx/LPC55S69
     INCLUDES   += -I$(CURDIR)/arch/devices/NXP/lpc55s69-sdk/devices
     SYMBOL_DEF += -DUSE_NXP_DRIVER
     obj-y      += arch/devices/NXP/LPC55xx/LPC55S69/startup_LPC55S69.o
     LINKER_SCRIPT  := $(CURDIR)/arch/devices/NXP/LPC55xx/LPC55S69/LPC55S69_FLASH.ld
     OPENOCD_TARGET += arch/debug/target/lpc55s69_debug.cfg
     endif
     ```
  5. Add a matching block to the top-level `Makefile` setting
     `CC_TARGET_PROP` for Cortex-M33:
     ```make
     ifeq ($(CONFIG_TARGET_MCU),"LPC55S69")
     CC_TARGET_PROP += -mthumb -mcpu=cortex-m33 \
                       -mfpu=fpv5-sp-d16 -mfloat-abi=hard
     endif
     ```
  6. Drop a probe + target config in `arch/debug/target/lpc55s69_debug.cfg`.
  7. Register the part in `scripts/gen_active_debug.py` so VS Code can
     pick up the right SVD and OpenOCD cfg automatically.

The rest of the OS (services, drivers, IPC, safety, shell) is
vendor-agnostic and does not change.

---

## The `device.h` Selection Header

`arch/devices/device.h` is the single header that translation units
include via `<device.h>`.  It branches on the chip symbol injected by
the build system (`-D$(CONFIG_TARGET_MCU)`) and pulls in the right
CMSIS + HAL headers for the active part.

Add a new vendor block whenever you add a new family — the existing
STM32 / Infineon / Microchip blocks are the template.

---

## Cross-References

  - `docs/ARCHITECTURE.md` — full architectural overview, layer model,
    safety layer, boot FSM.
  - `docs/DEBUG.md` — VS Code launch.json, OpenOCD options, SWO ITM.
  - `docs/OS_INSIDE.md` — internal post-mortems (linker `KEEP`,
    ISR priority rule, HAL timebase TIM6 vs TIM1).
