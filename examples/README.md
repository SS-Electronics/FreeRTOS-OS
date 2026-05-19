# `examples/` — Self-contained Devboard Examples

Each subdirectory is a complete, **standalone**, buildable application for
one development board.  No sibling `app/` repository is required.

```
examples/
├── stm32f411/                   STM32F411 Devboard (Cortex-M4F)
│   ├── app_main.c               Heartbeat + UART echo + shell
│   ├── Makefile                 App build fragment
│   ├── kconfig.conf             Kconfig preset (target = STM32F411xE)
│   ├── os_conf_include/         OS-config headers used by this build
│   │   ├── conf_board.h
│   │   └── def_compiler.h
│   └── board/
│       ├── stm32f411_devboard.xml   Hardware description  ◄── input
│       ├── irq_table.xml            NVIC priorities       ◄── input
│       ├── mcu_config.h             MCU peripheral counts ◄── tracked
│       │
│       └── (generated outputs — re-created by `make dev-stm32f411`)
│           board_config.{c,h}, board_device_ids.h, board_handles.h,
│           irq_hw_conf.h, irq_hw_init_generated.{c,h},
│           irq_periph_dispatch_generated.c,
│           irq_periph_handlers_generated.h,
│           irq_periph_vectors_generated.inc,
│           irq_table_generated.c
│
└── stm32h723/                   NUCLEO-H723ZG (Cortex-M7)
    │   …same layout as above…
    └── …
```

Crucially, **every devboard owns its own `board/`** containing both XML
inputs and generated outputs.  Building one example never touches files
under another devboard, so you can build/flash both back-to-back without
clobbering generated state.

---

## Build & flash

| Devboard | Build | Flash |
|---|---|---|
| STM32F411VET6 | `make dev-stm32f411` | `make dev-stm32f411-flash` |
| STM32H723ZGTx (NUCLEO) | `make dev-stm32h723` | `make dev-stm32h723-flash` |

Each `make dev-*` invocation chains internally:

  1. Regenerate `board_config.{c,h}` + `board_device_ids.h` +
     `board_handles.h` from `<board>.xml`.
  2. Regenerate the IRQ table + dispatch + vector outputs from
     `irq_table.xml`.
  3. Copy `kconfig.conf` to `.config` and run `make config-outputs`.
  4. `make clean`.
  5. Build `build/<target>.elf` + `.hex` + `.asm` + `.sym`.

Higher-level wrappers `make os TARGET=stm32f411` and
`make os TARGET=stm32h723` invoke the same flow.

---

## Adding a new devboard example

  1. `cp -r examples/stm32f411 examples/<new-target>`.
  2. Replace `board/stm32f411_devboard.xml` with the new board XML.
  3. Edit `board/irq_table.xml` for the new MCU's NVIC priorities.
  4. Edit `board/mcu_config.h` for the right peripheral counts.
  5. Edit `kconfig.conf` — set `CONFIG_TARGET_MCU` and any peripheral
     module flags.
  6. Optionally adapt `app_main.c` (most boards work unchanged).
  7. Add a `dev-<new-target>` block to the top-level `Makefile`
     pointing at `examples/<new-target>` and the right
     `CONFIG_BOARD` value.
