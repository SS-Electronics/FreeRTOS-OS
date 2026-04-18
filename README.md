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
| [docs/DEBUG.md](docs/DEBUG.md) | VSCode debug setup, OpenOCD, GDB, ITM/SWO |

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
#include <services/uart_mgmt.h>
#include <services/gpio_mgmt.h>

static void heartbeat(void *arg)
{
    for (;;) {
        gpio_mgmt_post(0, GPIO_MGMT_CMD_TOGGLE, 0, 0);   /* toggle LED_BOARD */
        os_thread_delay(500);
    }
}

int app_main(void)
{
    os_thread_create(heartbeat, "heartbeat", 256, 2, NULL);
    return 0;
}
```

See [docs/OS_THREAD.md](docs/OS_THREAD.md) for the full thread API and [docs/QUEUE.md](docs/QUEUE.md) for IPC patterns.

---

## Prerequisites

```bash
sudo bash scripts/install_arm_gcc.sh     # arm-none-eabi-gcc
sudo bash scripts/install_kconfig.sh     # ncurses Kconfig front-end
sudo bash scripts/install_openocd.sh    # OpenOCD for flash / debug
sudo bash scripts/install_doxygen.sh    # Doxygen (optional, for make docs)
```

See [docs/DEBUG.md](docs/DEBUG.md) for VS Code integration and GDB setup.

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

## License

FreeRTOS-OS is distributed under the **GNU General Public License v3.0**.
See `COPYING` or <https://www.gnu.org/licenses/gpl-3.0.html>.

Submodule licenses: FreeRTOS-Kernel — MIT · STM32F4xx HAL — BSD 3-Clause · CMSIS-6 — Apache 2.0 · CANopen stack — Apache 2.0 · ringbuffer — GPL v2.

---

*Author: Subhajit Roy — subhajitroy005@gmail.com*
