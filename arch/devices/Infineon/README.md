# `arch/devices/Infineon/` — Infineon MCU Support (placeholder)

Reserved for Infineon Cortex-M MCU families (XMC1xxx Cortex-M0, XMC4xxx
Cortex-M4, PSoC 6, AURIX TC3xx in MCU-mode).

`include/drivers/hal/stm32/hal_iic_infineon.h`,
`drivers/hal/infineon/hal_iic_infineon.c`, and
`drivers/hal/infineon/hal_uart_infineon.c` already hold the
work-in-progress generic-driver shims that an Infineon port would
plug into.

The directory layout follows the same convention as `arch/devices/STM/` —
see `arch/devices/NXP/README.md` for the step-by-step bring-up guide and
`docs/ARCHITECTURE.md` for the full vendor-integration contract.
