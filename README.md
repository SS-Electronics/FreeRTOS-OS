# FreeRTOS-OS

A structured, Linux-inspired OS layer built on top of the FreeRTOS kernel for ARM Cortex-M4 embedded targets. Provides generic thread management, an intrusive linked-list, an IPC message-queue subsystem, a hardware abstraction layer driven by Kconfig, and a service-thread architecture — all sitting above FreeRTOS without replacing it.

> **Default target:** STM32F411VET6 · ARM Cortex-M4F · 100 MHz · 512 KB Flash · 128 KB RAM

---

## Documentation

| Document | Description |
|---|---|
| [docs/OS_THREAD.md](docs/OS_THREAD.md) | Thread API, linked list, `os_thread_create()` usage |
| [docs/QUEUE.md](docs/QUEUE.md) | IPC message queues, ring buffer, management service queues |
| [docs/DRIVERS.md](docs/DRIVERS.md) | 3-layer driver architecture, HAL ops vtables, vendor backends |
| [docs/BOARD.md](docs/BOARD.md) | Board XML schema, BSP code generator, adding boards/vendors |
| [docs/DEV_MGMT.md](docs/DEV_MGMT.md) | UART / I2C / SPI / GPIO management service threads |
| [docs/SHELL_CLI.md](docs/SHELL_CLI.md) | OS shell CLI, FreeRTOS+CLI, printk, registering commands |
| [docs/IRQ.md](docs/IRQ.md) | Linux-style IRQ dispatch, irq_desc chain, request_irq, irq_register |
| [docs/DEBUG.md](docs/DEBUG.md) | VSCode debug setup, OpenOCD, GDB, ITM/SWO |
| [docs/OS_INSIDE.md](docs/OS_INSIDE.md) | Internals deep-dive: ISR priority rules, post-mortems, debug recipes |

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                       User Application                          │
│   app_main() · uart_mgmt_async_transmit() · gpio_mgmt_post()   │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                      Service Layer                               │
│     UART Mgmt · I2C Mgmt · SPI Mgmt · GPIO Mgmt                │
│     OS Shell (FreeRTOS+CLI, ring-buffer I/O over shell UART)    │
│     (FreeRTOS tasks; self-register from board config at boot)   │
└───────────┬──────────────────────────────┬───────────────────────┘
            │                              │
┌───────────▼──────────┐     ┌─────────────▼──────────────────────┐
│    OS Kernel API     │     │          IPC Subsystem             │
│  os_thread_create()  │     │   ipc_mqueue_register()            │
│  os_thread_delete()  │     │   ipc_mqueue_send_item()           │
│  os_suspend_thread() │     │   ipc_mqueue_receive_item()        │
│  os_resume_thread()  │     │   Ring-buffer (HW byte streams)    │
│  os_thread_delay()   │     │   xQueue (inter-task messages)     │
└───────────┬──────────┘     └─────────────┬──────────────────────┘
            │                              │
┌───────────▼──────────────────────────────▼───────────────────────┐
│                  Memory Management (mm)                          │
│   Intrusive doubly-circular linked list (Linux list_head style)  │
│   kmalloc() / kfree() — wrappers over pvPortMalloc              │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                      FreeRTOS Kernel                             │
│   Tasks · Queues · Mutexes · Semaphores · Timers · heap_4       │
│   Portable layer: GCC/ARM_CM4F                                   │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                 Board Configuration (BSP)                        │
│   boards/<board>.xml → gen_board_config.py →                    │
│   board_config.c + board_device_ids.h + board_handles.h         │
└────────────────────────────┬─────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│              Hardware Abstraction Layer (HAL)                    │
│   STM32F4xx HAL — driven by Kconfig → autoconf.h                │
│   CMSIS-Core · Startup · Linker script                           │
└──────────────────────────────────────────────────────────────────┘
```

---

## Quick Start

```bash
# 1. Clone with submodules
git clone --recurse-submodules <repo-url>
cd FreeRTOS-OS-App

# 2. Configure kernel (run once, or when hardware options change)
make menuconfig

# 3. Generate autoconf.mk and autoconf.h from .config
make config-outputs

# 4. Generate BSP from board XML (run once, or after editing the XML)
#    Default board: FreeRTOS-OS/boards/stm32f411_devboard.xml
make board-gen

# 5. Build
make app              # OS + app/  →  FreeRTOS-OS/build/app.elf
make kernel           # OS only    →  FreeRTOS-OS/build/kernel.elf

# 6. Flash
make flash-app

# 7. Clean
make clean
```

> Steps 2–4 are one-time setup. After that, only `make app` / `make flash-app` is needed.

---

## Writing an Application

Implement `app_main()` in `app/app_main.c`. The OS calls it after the scheduler starts; the function should create tasks and return.

```c
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <services/uart_mgmt.h>
#include <services/gpio_mgmt.h>
#include <services/os_shell_management.h>
#include <board/board_device_ids.h>

static void heartbeat(void *arg)
{
    uint32_t count = 0;
    for (;;) {
        gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        printk("heartbeat: tick %lu\n", (unsigned long)count++);
        os_thread_delay(500);
    }
}

int app_main(void)
{
    os_shell_mgmt_start();                                     /* start shell CLI */
    os_thread_create(heartbeat, "heartbeat", 256, 2, NULL);
    return 0;
}
```

`printk()` accepts `printf`-style variadic format arguments. Output goes to `COMM_PRINTK_HW_ID` (UART_DEBUG by default); the shell CLI runs independently on `UART_SHELL_HW_ID` (UART_APP). Both use interrupt-driven ring-buffer TX.

See [docs/OS_THREAD.md](docs/OS_THREAD.md) for the full thread API, [docs/QUEUE.md](docs/QUEUE.md) for IPC patterns, and [docs/SHELL_CLI.md](docs/SHELL_CLI.md) for the shell system.

---

## Prerequisites

### One-command install (recommended)

```bash
# From FreeRTOS-OS/ directory
make install-prerequisites
```

Installs in order: ARM GCC toolchain → OpenOCD → kconfig-frontends → debug tools (SVD + VS Code extensions).

### Individual targets

| Make target | What it installs |
|---|---|
| `make install-toolchain` | `arm-none-eabi-gcc`, `arm-none-eabi-gdb` (via `gdb-multiarch` + symlink on Debian/Ubuntu) |
| `make install-openocd` | OpenOCD flash/debug server |
| `make install-kconfig` | `kconfig-frontends` — provides `make menuconfig` |
| `make install-debug-tools` | STM32F411 SVD file + Cortex-Debug VS Code extension |
| `make install-doxygen` | Doxygen + Graphviz (optional, for `make docs`) |

### Manual install (if make targets are not available yet)

```bash
sudo bash scripts/install_arm_gcc.sh     # ARM GCC + GDB
sudo bash scripts/install_openocd.sh     # OpenOCD
sudo bash scripts/install_kconfig.sh     # kconfig-frontends (menuconfig)
bash scripts/install_debug_tools.sh      # SVD file + VS Code extensions (no sudo needed)
sudo bash scripts/install_doxygen.sh     # Doxygen (optional)
```

### ST-Link udev rules (Linux — required for non-root USB access)

```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
# Disconnect and reconnect ST-Link USB after this step.
```

### VS Code debug setup

1. Open the repo root (`FreeRTOS-OS-App/`) in VS Code.
2. Run the task **install-debug-tools** (Terminal → Run Task) to download the SVD file.
3. Connect your ST-Link to the board (SWDIO, SWDCLK, GND — NRST optional).
4. Press **F5** → select **Auto — Build, Flash & Debug**.

See [docs/DEBUG.md](docs/DEBUG.md) for full GDB, ITM/SWO, and peripheral register viewer setup.

---

## Supported Devices

Default: **STM32F411xE** (⭐). Run `make menuconfig` → *Target MCU part number* to select another.

| Config ID | Core | Flash | RAM |
|---|---|---|---|
| `STM32F411xE` ⭐ | Cortex-M4F @ 100 MHz | 512 KB | 128 KB |
| `STM32F405xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| `STM32F407xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| `STM32F446xx` | Cortex-M4F @ 180 MHz | 512 KB | 128 KB |
| `STM32F429xx` | Cortex-M4F @ 180 MHz | 2 MB | 256 KB |

Full device list: run `make menuconfig` and browse *Target MCU part number*.

---

## ISR Priority Rules

FreeRTOS on Cortex-M uses `BASEPRI` to implement critical sections. Any peripheral ISR that calls FreeRTOS API (including `FROM_ISR` variants) **must** have an NVIC priority numerically **greater than or equal to** `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY` (5 on this project). Priorities 1–4 are reserved for bare-metal ISRs that call zero FreeRTOS API.

All peripheral IRQ priorities are set in `app/board/irq_table.xml`. After any change run `make irq_gen` to regenerate `irq_hw_init_generated.c`.

| Priority range | FreeRTOS API allowed? | Example use |
|---|---|---|
| 1–4 | **No** — runs above BASEPRI mask | HAL timebase timer |
| 5–14 | **Yes** — `FROM_ISR` variants only | UART, SPI, I2C, EXTI |
| 15 | Lowest — kernel use | SysTick, PendSV |

See [docs/OS_INSIDE.md](docs/OS_INSIDE.md) for the detailed post-mortem and prevention guidelines.

---

## License

FreeRTOS-OS is distributed under the **GNU General Public License v3.0**.
See `COPYING` or <https://www.gnu.org/licenses/gpl-3.0.html>.

Submodule licenses: FreeRTOS-Kernel — MIT · STM32F4xx HAL — BSD 3-Clause · CMSIS-6 — Apache 2.0 · CANopen stack — Apache 2.0 · ringbuffer — GPL v2.

---

*Author: Subhajit Roy — subhajitroy005@gmail.com*
