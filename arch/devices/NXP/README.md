# `arch/devices/NXP/` — NXP MCU Support (placeholder)

This directory is reserved for NXP MCU vendor packages and is currently
empty.  It mirrors the layout used by `arch/devices/STM/`:

```
arch/devices/NXP/
├── <family>-hal-driver/    (git submodule — vendor HAL SDK)
├── <family>/
│   ├── <family>.h          (CMSIS device header — all parts)
│   ├── system_<family>.c   (CMSIS system_init)
│   └── <part>/
│       ├── startup_<part>.c
│       ├── <part>.h        (CMSIS chip-variant header)
│       └── linker.ld       (flash & RAM map)
```

## To bring up a new NXP target

1. Add the SDK / HAL as a git submodule under `arch/devices/NXP/`.
2. Drop the CMSIS device + part headers and a linker script in the
   family / part subdirs above.
3. Implement the generic driver vtables in `drivers/hal/nxp/` (mirror
   the STM32 backend under `drivers/hal/stm32/`).
4. Add an `ifeq ($(CONFIG_TARGET_MCU),"<PART>")` block to
   `arch/Makefile` that:
     - extends `INCLUDES` with the SDK + family + part dirs;
     - adds startup + system .o entries to `obj-y`;
     - sets `CC_TARGET_PROP` for the Cortex-M variant
       (e.g. `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard`);
     - sets `LINKER_SCRIPT` to the per-part `.ld`;
     - sets `OPENOCD_TARGET` to a matching script under
       `arch/debug/target/`.
5. Register the vendor symbol in `include/doxygen_groups.h` if you
   want generated docs to surface the new HAL.

See `docs/ARCHITECTURE.md` → *Adding a new MCU vendor* for the full
worked example.
