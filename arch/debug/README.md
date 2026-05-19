# `arch/debug/` — Debug Probe & Target Configuration

OpenOCD configuration files for flashing and debugging the supported MCUs,
organised by what they describe:

```
arch/debug/
├── interface/      Probe-side .cfg files (ST-Link, J-Link, CMSIS-DAP, etc.)
│                   Currently empty — we use OpenOCD's built-in
│                   `interface/stlink.cfg` resolved from the system search path.
│                   Drop a local override here if your distro's OpenOCD
│                   doesn't ship one for your probe.
│
└── target/         Per-MCU OpenOCD scripts (flash banks, reset strategy,
                    event hooks).  Also where install_debug_tools.sh stores
                    downloaded SVD register-description files.
                    Files: stm32_f401xx_debug.cfg, stm32_f411xx_debug.cfg,
                           stm32_h723xx_debug.cfg, STM32F401.svd, …
```

## Why split into `interface/` and `target/`

OpenOCD always needs two cooperating scripts: one describes the *debug probe*
hardware (USB transport, JTAG/SWD, speed); the other describes the *target*
chip (CPU type, flash banks, reset sequence).  Keeping the two roles in
separate directories means:

  - swapping a probe (ST-Link → J-Link) touches `interface/` only;
  - bringing up a new MCU touches `target/` only;
  - the `OPENOCD_INTERFACE` / `OPENOCD_TARGET` variables in
    `arch/Makefile` map cleanly onto the two directories.

## Adding a new MCU

1. Copy the closest existing `target/<vendor>_<family>_debug.cfg` and
   adapt CHIPNAME, flash base, and reset strategy.
2. Add an `ifeq ($(CONFIG_TARGET_MCU), …)` block in `arch/Makefile` that
   sets `OPENOCD_TARGET += arch/debug/target/<your>.cfg` and
   `OPENOCD_INTERFACE += interface/<probe>.cfg`.
3. If you want the VS Code peripheral viewer, register the part in
   `scripts/gen_active_debug.py` (`MCU_DEBUG_MAP`) along with the SVD path.

## Adding a new probe

Drop the probe-side `.cfg` under `interface/` and reference it from the
`OPENOCD_INTERFACE` variable in `arch/Makefile`.  OpenOCD's built-in
`interface/*.cfg` files are usually sufficient, so a local override is
only needed for custom adapters or distro packaging quirks.
