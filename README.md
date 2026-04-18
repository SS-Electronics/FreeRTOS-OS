# FreeRTOS-OS

A structured, Linux-inspired OS layer built on top of the FreeRTOS kernel for ARM Cortex-M4 embedded targets. The project provides generic thread management, intrusive linked-list data structures, an IPC message-queue subsystem, a hardware abstraction layer driven by Kconfig, and a service-thread architecture — all sitting above FreeRTOS without replacing it.

> **Default target:** STM32F411VET6 (ARM Cortex-M4F · 100 MHz · 512 KB Flash · 128 KB RAM)

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Directory Structure](#directory-structure)
- [Layer Descriptions](#layer-descriptions)
  - [Boot & Startup](#boot--startup)
  - [Architecture (arch)](#architecture-arch)
  - [Kernel](#kernel)
  - [Memory Management (mm)](#memory-management-mm)
  - [IPC Subsystem](#ipc-subsystem)
  - [Services Layer](#services-layer)
  - [Drivers](#drivers)
  - [Driver Implementation Example](#driver-implementation-example)
  - [Communication Stacks](#communication-stacks)
- [Driver Architecture](DRIVERS.md)
- [Board Configuration System](BOARD.md)
- [Thread API — Linux-Style Design](#thread-api--linux-style-design)
- [Linked List — Intrusive Design](#linked-list--intrusive-design)
- [Message Queue (IPC)](#message-queue-ipc)
- [Kconfig / Build Configuration](#kconfig--build-configuration)
- [Build System](#build-system)
- [Supported Devices](#supported-devices)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Detailed Build Process](#detailed-build-process)
- [Debugging](#debugging)
- [License](#license)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      User Application                          │
│   app_main() · board_uart_register_rx_cb() · gpio_mgmt_post() │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                    Service Layer                                │
│   UART Mgmt · I2C Mgmt · SPI Mgmt · GPIO Mgmt                 │
│   OS Shell · Diagnostics                                       │
│   (FreeRTOS tasks; self-register peripherals from board config) │
└───────────┬───────────────────────────────┬────────────────────┘
            │                               │
┌───────────▼──────────┐      ┌─────────────▼──────────────────┐
│    OS Kernel API     │      │        IPC Subsystem           │
│  os_thread_create()  │      │  ipc_mqueue_register()         │
│  os_thread_delete()  │      │  ipc_mqueue_send_item()        │
│  os_suspend_thread() │      │  ipc_mqueue_receive_item()     │
│  os_resume_thread()  │      │  Ring-buffer (HW queues)       │
│  os_thread_delay()   │      │  xQueue (inter-task queues)    │
└───────────┬──────────┘      └─────────────┬──────────────────┘
            │                               │
┌───────────▼───────────────────────────────▼──────────────────┐
│               Memory Management (mm)                         │
│   Intrusive doubly-circular linked list (Linux list_head)    │
│   kmaloc() / kfree()  — wrappers over pvPortMalloc          │
│   DMA pool (stub, pluggable)                                 │
└───────────────────────────────┬──────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────┐
│                  FreeRTOS Kernel                             │
│   Tasks · Queues · Mutexes · Semaphores · Timers · heap_4   │
│   Portable layer: GCC/ARM_CM4F                               │
└───────────────────────────────┬──────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────┐
│              Board Configuration (BSP)                       │
│   boards/<board>.xml → gen_board_config.py →                 │
│   board_config.c + board_device_ids.h + board_handles.h      │
│   HAL handle definitions · Descriptor tables · Callbacks     │
└───────────────────────────────┬──────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────┐
│              Hardware Abstraction Layer (HAL)                │
│   STM32F4xx HAL — driven by Kconfig → autoconf.h            │
│   CMSIS-Core · Startup · Linker script                       │
└──────────────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
FreeRTOS-OS/
│
├── Kconfig                    # Top-level Kconfig (sources arch + kernel)
├── Makefile                   # Top-level build: compile, link, docs
├── .config                    # Pre-filled config for STM32F411VET6
├── autoconf.mk                # Generated — Makefile CONFIG_* variables
│
├── arch/                      # Architecture and device-specific code
│   ├── Kconfig                # MCU, clock, HAL module selection
│   ├── Makefile               # Per-MCU object lists, includes, linker script
│   ├── arm/
│   │   └── CMSIS_6/           # ARM CMSIS-Core headers (submodule)
│   ├── debug_cfg/             # OpenOCD .cfg files
│   └── devices/STM/
│       ├── STM32F4xx/         # Startup, system, IT, MSP, HAL timebase
│       │   ├── stm32f4xx_hal_conf.h   # HAL config — reads autoconf.h
│       │   └── STM32F411/     # Linker script, startup for F411VET6
│       └── stm32f4xx-hal-driver/      # STM32 HAL driver (submodule)
│
├── boot/                      # Bootloader stubs
│
├── com_stack/                 # Protocol stacks
│   ├── canopen-stack/         # CANopen (submodule)
│   └── ROS/                   # ROS bridge stubs
│
├── boards/                    # Board support packages (BSP)
│   ├── stm32f411_devboard.xml # Board XML description (source of truth)
│   ├── Makefile               # Adds boards/<board>/board_config.o to link
│   └── stm32f411_devboard/
│       └── board_config.c     # AUTO-GENERATED — run 'make board-gen'
│
├── docs/                      # Documentation assets, Doxygen output
│
├── drivers/                   # Hardware driver implementations
│   ├── drv_uart.c             # UART driver
│   ├── drv_iic.c              # I2C driver
│   ├── drv_spi.c              # SPI driver
│   ├── drv_can.c              # CAN driver
│   ├── drv_gpio.c             # GPIO driver
│   ├── drv_time.c             # Timer / time-base driver
│   └── drv_cpu.c              # CPU utility (reset, WFI, etc.)
│
├── include/                   # All project-wide headers
│   ├── Makefile               # Exports INCLUDES paths
│   ├── autoconf.h             # (generated) Kconfig → C symbols
│   ├── board/
│   │   ├── board_config.h     # Board API types, callback typedefs, prototypes
│   │   ├── board_device_ids.h # AUTO-GENERATED device ID #defines + BOARD_*_COUNT
│   │   └── board_handles.h    # AUTO-GENERATED extern HAL handles (huart1, hi2c1, …)
│   ├── config/
│   │   ├── autoconf.h         # Generated by make config-outputs
│   │   ├── FreeRTOSConfig.h   # FreeRTOS tuning — reads autoconf.h
│   │   ├── mcu_config.h       # Peripheral counts from BOARD_*_COUNT (re-exported)
│   │   ├── os_config.h        # OS service flags and buffer sizes
│   │   └── board.h            # Legacy board-level pin / UART assignments
│   ├── def_attributes.h       # GCC section / keep attributes
│   ├── def_err.h              # OS error codes (OS_ERR_NONE, OS_ERR_OP …)
│   ├── def_std.h              # Standard C includes + status_t typedef
│   ├── def_hw.h               # Hardware-level definitions
│   ├── mm/
│   │   ├── list.h             # Intrusive doubly-circular list
│   │   └── dma_pool.h         # DMA pool interface
│   ├── os/
│   │   ├── kernel.h           # Thread API — os_thread_create() etc.
│   │   ├── kernel_mem.h       # kmaloc() / kfree() / os_get_free_mem()
│   │   ├── kernel_object.h    # Kernel object base (stub)
│   │   ├── kernel_services.h  # Service thread registration
│   │   ├── kernel_syscall.h   # printk / console interface
│   │   └── irq.h              # IRQ descriptor and registration API
│   └── ipc/
│       ├── mqueue.h           # Message queue API + descriptor types
│       ├── ringbuffer.h       # Byte-stream ring buffer
│       ├── ipc_types.h        # IPC PDU structures (I2C, CAN, …)
│       └── global_var.h       # Global queue ID arrays
│
├── init/
│   └── main.c                 # OS entry point — app_main() weak hook
│
├── ipc/
│   ├── mqueue.c               # Message queue implementation
│   └── global_var.c           # Global queue ID array definitions
│
├── kernel/
│   ├── Kconfig                # FreeRTOS tuning + OS service knobs
│   ├── Makefile               # FreeRTOS objects + portable layer
│   ├── kernel_thread.c        # Thread create/delete/suspend/resume
│   ├── irq.c                  # IRQ descriptor registration
│   └── FreeRTOS-Kernel/       # Official FreeRTOS (submodule)
│
├── lib/                       # Shared utility libraries
│
├── mm/
│   ├── list.c                 # Doubly-circular list implementation
│   ├── kernel_mem.c           # pvPortMalloc / vPortFree wrappers
│   └── dma_pool.c             # DMA pool (pluggable stub)
│
├── net/                       # Network stack stubs
│
├── scripts/                   # Installation, debug, and code generation
│   ├── gen_board_config.py    # XML → BSP code generator
│   ├── install_arm_gcc.sh
│   ├── install_kconfig.sh
│   ├── install_openocd.sh
│   ├── install_doxygen.sh
│   ├── run_openocd_server.sh
│   ├── run_gdb.sh
│   └── vscode_support/        # launch.json + tasks.json
│
├── services/
│   ├── kernel_service_core.c        # Registers all service threads at boot
│   ├── kernel_service_task_manager.c
│   ├── kernel_syscall.c             # printk implementation
│   ├── serv_uart_mgmt.c             # UART management service thread
│   └── serv_iic_mgmt.c             # I2C management service thread
│
└── tools/                     # Development tooling
```

---

## Layer Descriptions

### Boot & Startup

`arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.s` sets up the vector table and calls `SystemInit()` → `main()`. The linker script (`stm32_f411_FLASH.ld`) places the OS and application code into dedicated named sections (`.boot`, `.os`, `.os_data`, `.app`, `.app_data`) defined in `include/def_attributes.h`.

### Architecture (arch)

Provides the MCU-level glue:

| File | Purpose |
|---|---|
| `system_stm32f4xx.c` | SystemInit() — PLL, flash latency setup |
| `stm32f4xx_it.c` | Exception / IRQ handlers |
| `stm32f4xx_hal_msp.c` | HAL peripheral MSP init hooks |
| `stm32f4xx_hal_timebase_tim.c` | TIM-based HAL tick (replaces SysTick for FreeRTOS compatibility) |
| `stm32f4xx_hal_conf.h` | HAL module enable/disable — **generated from Kconfig** |

### Kernel

The kernel layer wraps FreeRTOS primitives behind a Linux-inspired API:

- **`kernel_thread.c`** — thread lifecycle management over a doubly-circular intrusive list (`thread_list` sentinel). Each `thread_handle_t` embeds a `struct list_node` so `list_for_each_entry()` can walk all live threads with no extra allocation.
- **`irq.c`** — hardware interrupt descriptor (`type_irq_desc`) registration. Callers provide a chip-level descriptor and a callback; the OS stores it in a linked structure for dispatch.
- **`FreeRTOS-Kernel/`** — official FreeRTOS (git submodule). The portable layer used is `GCC/ARM_CM4F` (hard-float ABI).

### Memory Management (mm)

| Module | Description |
|---|---|
| `list.c` / `list.h` | Intrusive doubly-circular linked list — same design as the Linux kernel `list_head`. Nodes embed `struct list_node`; `container_of()` recovers the parent structure. |
| `kernel_mem.c` | `kmaloc()` / `kfree()` — thin wrappers over `pvPortMalloc` / `vPortFree` placed in the `.os` section. |
| `dma_pool.c` | DMA-coherent memory pool (pluggable stub). |

### IPC Subsystem

All inter-task communication is registered through a central queue registry (`ipc/mqueue.c`). Two backend types are supported:

| Queue type | Backend | Use case |
|---|---|---|
| `IPC_MQUEUE_TYPE_PROC_TXN` | FreeRTOS `xQueueCreate` | Typed messages between tasks |
| `IPC_MQUEUE_TYPE_UART_HW` | RT-Thread `ringbuffer` | Byte stream from UART ISR |

The queue descriptor list is itself an intrusive doubly-circular list (each `type_message_queue_descriptor` embeds `struct list_node list`), so `ipc_mqueue_register` / `ipc_mqueue_unregister` are O(1) for append/remove.

### Services Layer

`services/kernel_service_core.c` is called at OS start to register static service threads:

| Thread | Stack | Priority | Purpose |
|---|---|---|---|
| `thread_uart_mgmt` | `CONF_THREAD_UART_MGMT_STACK_SIZE` | configurable | UART RX/TX orchestration |
| `therad_iic_mgmt` | `CONF_THREAD_IIC_MGMT_STACK_SIZE` | configurable | I2C transaction management |

Additional service threads (OS Shell, Diagnostics, CAN, ETH) are conditionally compiled via Kconfig `INC_SERVICE_*` flags.

### Board Configuration System

Hardware peripheral descriptions live in `boards/<board>.xml`. Running `make board-gen` (or simply `make all`) calls `scripts/gen_board_config.py` which produces:

- `boards/<board>/board_config.c` — HAL handle definitions (`huart1`, `hi2c1`, `hspi1`), peripheral descriptor tables, clock-enable wrappers, callback tables
- `include/board/board_device_ids.h` — user-facing device ID `#defines` and `BOARD_*_COUNT` constants
- `include/board/board_handles.h` — `extern` declarations for every HAL handle, guarded by `HAL_xxx_MODULE_ENABLED`; included automatically by `device.h`

Management service threads (`uart_mgmt`, `iic_mgmt`, `spi_mgmt`, `gpio_mgmt`) read `board_get_config()` at startup and register all peripherals automatically. Application code uses the device ID constants to post commands and register callbacks — no peripheral setup code is needed anywhere else.

For complete documentation of the XML schema, code generator architecture, generated file formats, callback registration, and how to add new boards or vendors — see **[BOARD.md](BOARD.md)**.

### Drivers

Hardware drivers implement a **3-layer architecture**: management service threads → generic driver layer → vendor HAL backend. Generic handles abstract all vendor-specific types; backends are swapped at compile time via `CONFIG_DEVICE_VARIANT`.

```
ISR  →  ringbuffer (IPC_MQUEUE_TYPE_UART_HW)  →  uart_mgmt thread
ISR  →  board_get_uart_cbs()->on_rx_byte()    →  application callback
Task →  xQueue (management queue)             →  management thread → HAL
```

For full driver subsystem documentation — handle structs, HAL ops vtables, vendor backends (STM32 / Infineon), management service thread lifecycles, and porting guides — see **[DRIVERS.md](DRIVERS.md)**.

### Driver Implementation Example

With the board configuration system, peripheral setup is fully automatic — the management threads register all peripherals from the board XML at startup. Application code only needs device IDs and the management thread API.

#### Board XML definition

```xml
<uart id="UART_DEBUG" dev_id="0" instance="USART1" baudrate="115200"
      word_len="8" stop_bits="1" parity="none" mode="tx_rx">
  <tx port="A" pin="9"  af="7" speed="very_high" pull="none"/>
  <rx port="A" pin="10" af="7" speed="very_high" pull="none"/>
  <irq name="USART1_IRQn" priority="2"/>
</uart>

<i2c id="I2C_SENSOR_BUS" dev_id="0" instance="I2C1" clock_hz="400000"
     addr_mode="7bit" dual_addr="disable">
  <scl port="B" pin="6" af="4" speed="very_high" pull="none"/>
  <sda port="B" pin="7" af="4" speed="very_high" pull="none"/>
</i2c>
```

Run `make board-gen` to generate `board_config.c` and `board_device_ids.h`. After that:

#### Step 1 — Async UART transmit

```c
#include <board/board_device_ids.h>   /* UART_DEBUG */
#include <services/uart_mgmt.h>

void my_task(void *arg)
{
    static const uint8_t hello[] = "Hello FreeRTOS-OS\r\n";
    for (;;) {
        /* Non-blocking — posts to management queue, returns immediately */
        uart_mgmt_async_transmit(UART_DEBUG, hello, sizeof(hello) - 1);
        os_thread_delay(1000);
    }
}
```

#### Step 2 — Register RX / error callbacks

```c
#include <board/board_config.h>

static void on_rx(uint8_t dev_id, uint8_t byte)
{
    /* ISR context — keep short */
}

static void on_error(uint8_t dev_id, uint32_t err)
{
    /* log or recover */
}

board_uart_register_rx_cb(UART_DEBUG, on_rx);
board_uart_register_error_cb(UART_DEBUG, on_error);
```

#### Step 3 — I2C sensor read (synchronous, task-notify pattern)

```c
#include <board/board_device_ids.h>   /* I2C_SENSOR_BUS */
#include <services/iic_mgmt.h>

#define INA230_ADDR   0x40
#define INA230_REG_V  0x02

void sensor_task(void *arg)
{
    uint8_t buf[2];

    for (;;) {
        /* Blocking read — suspends this task until iic_mgmt completes */
        int32_t ret = iic_mgmt_sync_receive(I2C_SENSOR_BUS,
                                             INA230_ADDR,
                                             INA230_REG_V,
                                             1,          /* use_reg = yes */
                                             buf, 2, 10);
        if (ret == OS_ERR_NONE) {
            uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
            /* process voltage reading */
        }
        os_thread_delay(100);
    }
}
```

#### Data flow summary

```
sensor_task
    │  iic_mgmt_sync_receive()   ← posts iic_mgmt_msg_t to queue
    │  ulTaskNotifyTake()        ← task suspends here
    ▼
iic_mgmt thread
    │  drv_iic_receive()        ← calls ops->receive()
    │  HAL_I2C_Mem_Read()       ← STM32 HAL
    │  xTaskNotifyGive(caller)  ← wakes sensor_task
    ▼
sensor_task resumes with result in buf[]
```

### Communication Stacks

- **CANopen** — integrated as a git submodule (`com_stack/canopen-stack/`)
- **ROS bridge** — stub headers for STM32Hardware and ROS message bridging

---

## Thread API — Linux-Style Design

The thread API mirrors POSIX / Linux conventions. Every created thread is linked into a global sentinel list (`thread_list`) using the same intrusive-list pattern as `struct task_struct` in the Linux kernel.

```c
#include "os/kernel.h"

/* Create a thread — returns positive thread_id or negative OS_ERR_* */
int32_t id = os_thread_create(my_func, "sensor", 512, 2, NULL);
//                             ^entry   ^name    ^words ^prio ^arg

/* Suspend / resume by ID (walks intrusive thread_list internally) */
os_suspend_thread(id);
os_resume_thread(id);

/* Millisecond delay from within a thread */
os_thread_delay(100);

/* Delete and free resources */
os_thread_delete(id);
```

**`thread_handle_t` structure:**

```c
typedef struct {
    struct list_node  list;           // intrusive sentinel-list node
    uint32_t          thread_id;      // monotonic unique ID
    uint32_t          priority;       // FreeRTOS UBaseType_t priority
    TaskHandle_t      thread_handle;  // underlying FreeRTOS handle
    void             *init_parameter; // opaque arg passed to entry
} thread_handle_t;
```

---

## Linked List — Intrusive Design

`mm/list.h` provides a doubly-circular sentinel list identical in design to the Linux kernel `include/linux/list.h`. Any struct can become a list member by embedding `struct list_node`:

```c
struct list_node {
    struct list_node *next;
    struct list_node *prev;
};
```

**Key operations:**

```c
LIST_NODE_HEAD(thread_list);                   // declare sentinel

list_add_tail(&handle->list, &thread_list);    // O(1) append

list_delete(&handle->list);                    // O(1) unlink

// Type-safe iteration via container_of:
thread_handle_t *pos;
list_for_each_entry(pos, &thread_list, list) {
    // pos is a valid thread_handle_t*
}
```

The `container_of(ptr, type, member)` macro recovers the containing struct from an embedded member pointer — no wrapper pointers, no extra heap allocation per node.

---

## Message Queue (IPC)

```c
#include "ipc/mqueue.h"

/* System init (called once at startup) */
ipc_mqueue_init();

/* Register a typed inter-task queue (10 items, 4 bytes each) */
int32_t qid = ipc_mqueue_register(IPC_MQUEUE_TYPE_PROC_TXN, -1, 4, 10);

/* Register a UART byte-stream ring buffer (512 bytes) */
int32_t uart_qid = ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, UART_1, 1, 512);

/* Non-blocking send from task context */
uint32_t value = 42;
ipc_mqueue_send_item(qid, &value);

/* Non-blocking receive */
ipc_mqueue_receive_item(qid, &value);

/* Iterate all queues via intrusive list */
type_message_queue_descriptor *pos;
list_for_each_entry(pos, ipc_get_mqueue_head(), list) {
    // pos->mqueue.queue_id, pos->mqueue.queue_type, ...
}

/* Unregister and free all resources */
ipc_mqueue_unregister(qid);
```

---

## Kconfig / Build Configuration

The configuration system works like the Linux kernel: a Kconfig menu drives `.config`, which is transformed into `include/config/autoconf.h` and `autoconf.mk`. Every C header (`stm32f4xx_hal_conf.h`, `FreeRTOSConfig.h`, `mcu_config.h`) reads from `autoconf.h` so there is a **single source of truth**.

```
make menuconfig          →  edits .config (ncurses TUI)
make config-outputs      →  generates autoconf.mk + include/config/autoconf.h
make all                 →  compiles using CONFIG_* from both files
```

### Kconfig menu layout

```
Kconfig  (top-level)
├── arch/Kconfig
│   ├── MCU Selection        TARGET_MCU, CPU_ARCH
│   ├── Clock Configuration  HSE_VALUE, HSI_VALUE, LSI/LSE, timeouts
│   ├── Core Modules         CORTEX, PWR, RCC, GPIO, EXTI, FLASH, DMA, WDG, CRC
│   ├── Communication        UART, USART, SPI, I2C, I2S, IrDA, SmartCard, SMBus
│   ├── Timer Peripherals    TIM
│   ├── Analog Peripherals   ADC
│   ├── USB                  PCD (device), HCD (host)
│   ├── Storage              SD/SDIO
│   ├── RTC and Security     RTC, RNG
│   └── System Configuration VDD, tick priority, caches, assert
└── kernel/Kconfig
    ├── FreeRTOS Kernel      tick rate, heap, priorities, stack, mutexes, timers, NVIC
    ├── OS Service Layer     NO_OF_UART/IIC, service thread enables
    ├── IPC Buffer Sizes     UART RX/TX buffer sizes, ITM buffer
    └── Debug Options        driver/app/ITM debug flags
```

### Pre-configured for STM32F411VET6

The committed `.config` sets:

| Setting | Value | Notes |
|---|---|---|
| `TARGET_MCU` | `STM32F411xE` | HAL device symbol |
| `HSE_VALUE` | `25 000 000 Hz` | 25 MHz crystal (common on VET6 boards) |
| Clock | 25 MHz → PLL → 100 MHz | PLLM=25, PLLN=200, PLLP=2 |
| `VDD_VALUE` | `3300 mV` | 3.3 V supply |
| `RTOS_TICK_RATE_HZ` | `1000` | 1 ms tick |
| `RTOS_TOTAL_HEAP_SIZE` | `80 000 bytes` | ~62% of 128 KB RAM |
| `HAL_UART_MODULE_ENABLED` | `y` | USART1/2/6 |
| `HAL_TIM_MODULE_ENABLED` | `y` | TIM1..TIM11 |
| CAN / ETH / DAC / FMC | `n` | Not present on STM32F411 |

---

## Build System

### Toolchain

| Tool | Role |
|---|---|
| `arm-none-eabi-gcc` | C/Assembly compiler |
| `arm-none-eabi-g++` | Linker (for C++ runtime init) |
| `arm-none-eabi-objcopy` | ELF → HEX conversion |
| `arm-none-eabi-objdump` | Disassembly generation |

Compiler flags for STM32F411VET6:
```
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
-O0 -g3 -std=gnu99 -Wall --specs=nano.specs
```

### Build outputs

| File | Description |
|---|---|
| `build/kernel.elf` | Linked ELF image |
| `build/kernel.elf.hex` | Intel HEX for flashing |
| `build/kernel.elf.asm` | Full disassembly |

### Sub-directory Makefiles

Each directory contributes objects to the top-level `obj-y` list:

| Makefile | Objects contributed |
|---|---|
| `arch/Makefile` | Startup, system, HAL driver sources (MCU-conditional) |
| `kernel/Makefile` | FreeRTOS core + portable layer + `kernel_thread.o` |
| `mm/Makefile` | `list.o`, `kernel_mem.o` |
| `include/Makefile` | Adds `-I` flags only (no objects) |
| `init/Makefile` | `main.o` |
| `boards/Makefile` | `boards/$(CONFIG_BOARD)/board_config.o` (auto-generated BSP) |
| `services/Makefile` | Management threads (uart_mgmt, iic_mgmt, spi_mgmt, gpio_mgmt) |
| `drivers/Makefile` | Generic driver layer + vendor HAL backends |

---

## Supported Devices

> Run `make menuconfig` and set **Target MCU part number** to the Config ID below.

| Series | Config ID | Core | Flash | RAM |
|---|---|---|---|---|
| STM32F4 | `STM32F411xE` ⭐ | Cortex-M4F @ 100 MHz | 512 KB | 128 KB |
| STM32F4 | `STM32F405xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| STM32F4 | `STM32F407xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| STM32F4 | `STM32F415xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| STM32F4 | `STM32F417xx` | Cortex-M4F @ 168 MHz | 1 MB | 192 KB |
| STM32F4 | `STM32F427xx` | Cortex-M4F @ 180 MHz | 1 MB | 256 KB |
| STM32F4 | `STM32F429xx` | Cortex-M4F @ 180 MHz | 2 MB | 256 KB |
| STM32F4 | `STM32F437xx` | Cortex-M4F @ 180 MHz | 1 MB | 256 KB |
| STM32F4 | `STM32F439xx` | Cortex-M4F @ 180 MHz | 2 MB | 256 KB |
| STM32F4 | `STM32F401xC` | Cortex-M4F @ 84 MHz | 256 KB | 64 KB |
| STM32F4 | `STM32F401xE` | Cortex-M4F @ 84 MHz | 512 KB | 96 KB |
| STM32F4 | `STM32F412Cx` | Cortex-M4F @ 100 MHz | 512 KB | 256 KB |
| STM32F4 | `STM32F412Rx` | Cortex-M4F @ 100 MHz | 512 KB | 256 KB |
| STM32F4 | `STM32F412Vx` | Cortex-M4F @ 100 MHz | 512 KB | 256 KB |
| STM32F4 | `STM32F412Zx` | Cortex-M4F @ 100 MHz | 512 KB | 256 KB |
| STM32F4 | `STM32F413xx` | Cortex-M4F @ 100 MHz | 1.5 MB | 320 KB |
| STM32F4 | `STM32F423xx` | Cortex-M4F @ 100 MHz | 1.5 MB | 320 KB |
| STM32F4 | `STM32F446xx` | Cortex-M4F @ 180 MHz | 512 KB | 128 KB |
| STM32F4 | `STM32F469xx` | Cortex-M4F @ 180 MHz | 2 MB | 384 KB |
| STM32F4 | `STM32F479xx` | Cortex-M4F @ 180 MHz | 2 MB | 384 KB |

⭐ Pre-configured default.

---

## Prerequisites

All installation scripts are in `scripts/`. Run them on the development host (Debian/Ubuntu/Fedora/Arch):

```bash
# ARM cross-compiler (arm-none-eabi-gcc)
sudo bash scripts/install_arm_gcc.sh

# Kconfig ncurses front-end (for make menuconfig)
sudo bash scripts/install_kconfig.sh

# OpenOCD (for flashing and debugging via ST-Link)
sudo bash scripts/install_openocd.sh

# Doxygen + Graphviz (for documentation generation)
sudo bash scripts/install_doxygen.sh
```

### VS Code integration

Copy `scripts/vscode_support/launch.json` and `scripts/vscode_support/tasks.json` into your `.vscode/` folder. The launch configuration connects GDB to an already-running OpenOCD server on port 3333.

---

## Quick Start

```bash
# 1. Clone with submodules
git clone --recurse-submodules <repo-url>
cd FreeRTOS-OS

# 2. (Optional) Customise hardware settings
#    Run from a real terminal, not VS Code integrated terminal
make menuconfig

# 3. Generate autoconf.h and autoconf.mk from .config
make config-outputs

# 4. (Optional) Edit your board XML and regenerate BSP
#    Default board: boards/stm32f411_devboard.xml
vim boards/stm32f411_devboard.xml
make board-gen                          # regenerates board_config.c, board_device_ids.h, board_handles.h

# 5. Build (board-gen runs automatically if XML is newer than generated files)
make all

# Build output
#   build/kernel.elf      — flash this
#   build/kernel.elf.hex  — Intel HEX
#   build/kernel.elf.asm  — disassembly

# 6. Clean
make clean
```

---

---

## Detailed Build Process

### Step 1 — Install prerequisites

```bash
sudo bash scripts/install_arm_gcc.sh    # arm-none-eabi-gcc toolchain
sudo bash scripts/install_kconfig.sh    # ncurses Kconfig front-end
sudo bash scripts/install_openocd.sh   # OpenOCD for flashing / debugging
```

### Step 2 — Configure kernel and HAL

Run the interactive Kconfig menu to select the target MCU, HAL modules (UART, I2C, SPI, GPIO), FreeRTOS heap size, tick rate, and service thread options:

```bash
make menuconfig
```

> Skip this step to use the pre-configured `.config` for STM32F411VET6 (100 MHz, 512 KB Flash, 128 KB RAM).

Generate C/Make configuration outputs from `.config`:

```bash
make config-outputs
# Produces: include/config/autoconf.h   — #define CONFIG_xxx
#           autoconf.mk                — CONFIG_xxx = y/0/value for Makefile
```

### Step 3 — Write or edit your board XML

Copy the reference board as a starting point:

```bash
cp boards/stm32f411_devboard.xml boards/my_board.xml
# Edit boards/my_board.xml to match your hardware wiring
```

Key elements to customise:
- `<board vendor="STM" mcu="STM32F411VET6">` — set vendor and MCU
- `<uart>` / `<i2c>` / `<spi>` / `<gpio>` — add, remove, or edit peripherals
- Pin `port`, `pin`, `af`, `speed`, `pull` attributes — match your schematic

### Step 4 — Generate BSP from XML

```bash
make CONFIG_BOARD=my_board board-gen
# OR: make CONFIG_BOARD=my_board all   ← generates automatically before compile
```

This runs `python3 scripts/gen_board_config.py boards/my_board.xml` and produces:

| File | Contents |
|------|----------|
| `boards/my_board/board_config.c` | HAL handle definitions, peripheral descriptor tables, callback tables, board API |
| `include/board/board_device_ids.h` | `#define UART_DEBUG 0`, `#define BOARD_UART_COUNT 2`, … |
| `include/board/board_handles.h` | `extern UART_HandleTypeDef huart1;`, `extern I2C_HandleTypeDef hi2c1;`, … |

> All three files are auto-generated — never edit them manually. Re-run `make board-gen` after any XML change.

### Step 5 — Build firmware

```bash
# Default board (stm32f411_devboard):
make all

# Custom board:
make CONFIG_BOARD=my_board all
```

Build output:

| File | Description |
|------|-------------|
| `build/kernel.elf` | Linked ELF image (flash this) |
| `build/kernel.elf.hex` | Intel HEX format |
| `build/kernel.elf.asm` | Full disassembly |

### Step 6 — Use device IDs in application code

```c
#include <board/board_device_ids.h>   // UART_DEBUG, LED_BOARD, I2C_SENSOR_BUS, …
#include <board/board_config.h>       // board_uart_register_rx_cb(), …
#include <services/uart_mgmt.h>
#include <services/gpio_mgmt.h>

/* Register an RX callback — called from UART ISR */
static void on_rx(uint8_t dev_id, uint8_t byte)
{
    /* process byte */
}

/* Async transmit — returns immediately, management thread sends */
uart_mgmt_async_transmit(UART_DEBUG, "Hello\r\n", 7);

/* Register callback */
board_uart_register_rx_cb(UART_DEBUG, on_rx);

/* GPIO — toggle LED with 500 ms delay */
gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 500);

/* I2C synchronous read */
uint8_t buf[2];
iic_mgmt_sync_receive(I2C_SENSOR_BUS, 0x40, 0x02, 1, buf, 2, 10);
```

### Step 7 — Flash

```bash
# In terminal 1
sudo bash scripts/run_openocd_server.sh

# In terminal 2
bash scripts/run_gdb.sh
# (gdb) load
# (gdb) continue
```

### Step 8 — Clean

```bash
make clean           # removes build artifacts and generated BSP files
make clean-docs      # removes Doxygen output
```

---

## Debugging

### Flash and start GDB server (ST-Link via OpenOCD)

```bash
# In terminal 1 — start OpenOCD
sudo bash scripts/run_openocd_server.sh

# In terminal 2 — connect GDB
bash scripts/run_gdb.sh
# or: arm-none-eabi-gdb build/kernel.elf -ex 'target remote localhost:3333'
```

### Generate documentation

```bash
make docs          # Doxygen HTML → docs/generated/html/index.html
make clean-docs    # Remove generated docs
```

---

## License

FreeRTOS-OS is free software distributed under the **GNU General Public License v3.0**.
See `COPYING` or <https://www.gnu.org/licenses/gpl-3.0.html>.

Submodules carry their own licenses:
- **FreeRTOS-Kernel** — MIT License
- **STM32F4xx HAL driver** — BSD 3-Clause
- **CMSIS-6** — Apache 2.0
- **CANopen stack** — Apache 2.0
- **ringbuffer** — GPL v2 (RT-Thread)

---

*Author: Subhajit Roy — subhajitroy005@gmail.com*
