# Boot Flow — Reset_Handler to vTaskStartScheduler

This document traces the complete execution path from the first instruction
executed after power-on/reset all the way to `vTaskStartScheduler()` in
[`init/main.c`](../init/main.c). Every function call is grounded in the actual
source files in this repository.

---

## Table of Contents

- [High-Level Flow](#high-level-flow)
- [Phase 0 — Hardware Reset](#phase-0--hardware-reset)
- [Phase 1 — Reset_Handler (before main)](#phase-1--reset_handler-before-main)
  - [1.1 Stack Pointer Initialisation](#11-stack-pointer-initialisation)
  - [1.2 SystemInit — FPU and VTOR](#12-systeminit--fpu-and-vtor)
  - [1.3 .data Section Copy (Flash → RAM)](#13-data-section-copy-flash--ram)
  - [1.4 .bss Zero-Fill](#14-bss-zero-fill)
  - [1.5 C++ Static Constructors](#15-c-static-constructors)
- [Phase 2 — main()](#phase-2--main)
  - [2.1 HAL_Init](#21-hal_init)
  - [2.2 drv_rcc_clock_init — PLL Bring-Up](#22-drv_rcc_clock_init--pll-bring-up)
  - [2.3 drv_cpu_interrupt_prio_set — NVIC Configuration](#23-drv_cpu_interrupt_prio_set--nvic-configuration)
  - [2.4 ipc_queues_init — IPC Ring Buffers](#24-ipc_queues_init--ipc-ring-buffers)
  - [2.5 uart_mgmt_start — Management Thread Creation](#25-uart_mgmt_start--management-thread-creation)
  - [2.6 printk_init](#26-printk_init)
  - [2.7 app_main — Application Task Creation](#27-app_main--application-task-creation)
- [Phase 3 — vTaskStartScheduler](#phase-3--vtaskstartscheduler)
- [Phase 4 — Post-Scheduler: Management Thread Startup](#phase-4--post-scheduler-management-thread-startup)
- [Memory Map and Section Placement](#memory-map-and-section-placement)
- [Interrupt State Through Boot](#interrupt-state-through-boot)
- [Call Stack Summary](#call-stack-summary)

---

## High-Level Flow

```
                    ┌─────────────────────────────────────────────────────┐
                    │            POWER-ON / RESET                         │
                    │   Cortex-M4 fetches initial SP from 0x08000000      │
                    │   Cortex-M4 fetches Reset_Handler from 0x08000004   │
                    └───────────────────┬─────────────────────────────────┘
                                        │
          ┌─────────────────────────────▼──────────────────────────────────┐
          │  PHASE 1 — Reset_Handler()                                     │
          │  arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c │
          │                                                                 │
          │  1. Re-load SP from g_pfnVectors[0]                            │
          │  2. SystemInit()        → FPU enable, optional VTOR            │
          │  3. Copy .boot_data, .os_data, .app_data, .data  Flash → RAM   │
          │  4. Zero-fill .bss                                              │
          │  5. __libc_init_array() → C++ static constructors              │
          │  6. → main()                                                    │
          └─────────────────────────────┬──────────────────────────────────┘
                                        │
          ┌─────────────────────────────▼──────────────────────────────────┐
          │  PHASE 2 — main()                                              │
          │  init/main.c                                                   │
          │                                                                 │
          │  1. HAL_Init()                → TIM1 tick, NVIC groups         │
          │  2. drv_rcc_clock_init()      → HSI → PLL → 100 MHz SYSCLK    │
          │  3. drv_cpu_interrupt_prio_set() → SVCall priority             │
          │  4. ipc_queues_init()         → ring buffers for UART TX/RX    │
          │  5. uart_mgmt_start()         → create queue + FreeRTOS task   │
          │  6. printk_init()             → bind printk to UART TX queue   │
          │  7. app_main()                → create application tasks        │
          └─────────────────────────────┬──────────────────────────────────┘
                                        │
          ┌─────────────────────────────▼──────────────────────────────────┐
          │  PHASE 3 — vTaskStartScheduler()                               │
          │  Does not return. Scheduler takes over.                        │
          └─────────────────────────────┬──────────────────────────────────┘
                                        │
          ┌─────────────────────────────▼──────────────────────────────────┐
          │  PHASE 4 — Tasks running                                       │
          │                                                                 │
          │  t=3000 ms  gpio_mgmt  wakes → registers GPIO pins             │
          │  t=4000 ms  uart_mgmt  wakes → HAL_UART_Init + start_rx_it    │
          │  t=5500 ms  spi_mgmt   wakes → HAL_SPI_Init                    │
          │  t=6500 ms  iic_mgmt   wakes → HAL_I2C_Init                    │
          └────────────────────────────────────────────────────────────────┘
```

---

## Phase 0 — Hardware Reset

When the STM32F411 is released from reset (power-on, NRST pulse, or software
`NVIC_SystemReset()`), the Cortex-M4 hardware executes a fixed bootstrap
sequence in silicon — no software involved yet:

| Step | What the CPU does | Source |
|------|-------------------|----|
| 1 | Load initial MSP from address `0x08000000` (first word of the vector table) | ARM Architecture Reference Manual |
| 2 | Load PC from address `0x08000004` (second word — the Reset_Handler pointer) | ARM Architecture Reference Manual |
| 3 | Begin executing at `Reset_Handler` with interrupts **disabled** (`PRIMASK=1`) | Cortex-M4 TRM |

The vector table is placed at `0x08000000` by the linker script because the
`__attribute__((section(".isr_vector")))` on `g_pfnVectors[]` maps to the
`.isr_vector` output section, which the linker script places at `FLASH_BOOT`
origin. The first entry is `&_estack` (the initial stack pointer value, a
linker symbol at the top of SRAM).

**File:** [arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c:124](../arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c#L124)

---

## Phase 1 — Reset_Handler (before main)

**File:** [arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c:233](../arch/devices/STM/STM32F4xx/STM32F411/startup_stm32f411vetx.c#L233)

`Reset_Handler` is declared `__SECTION_BOOT __USED __attribute__((noreturn))`.
The `__SECTION_BOOT` attribute places it in the `.boot_text` section so it lands
in low Flash addresses, adjacent to the vector table it belongs to.

### 1.1 Stack Pointer Initialisation

```c
__asm volatile (
    "ldr r0, =g_pfnVectors\n\t"
    "ldr sp, [r0]"
);
```

The first word of `g_pfnVectors` is `&_estack` — the top of SRAM as defined in
the linker script. This re-loads SP from the vector table entry, ensuring that
SP is correct even if the bootloader or debug probe may have modified it during
connection.

> Why re-load SP here when the hardware already loaded it at reset?
> The hardware reads it from the vector table at `SCB->VTOR`, which defaults to
> `0x08000000`. If the firmware is loaded by a bootloader at a different Flash
> offset and VTOR was not yet updated, SP would be wrong. Re-loading from the
> actual symbol avoids this.

### 1.2 SystemInit — FPU and VTOR

```c
SystemInit();
```

**File:** [drivers/hal/stm32/hal_rcc_stm32.c:60](../drivers/hal/stm32/hal_rcc_stm32.c#L60)

`SystemInit` runs **before `.data` and `.bss` are initialised**. It must only
access stack-local variables and symbols in the `.boot_text` / `.boot_data`
sections. The `SystemCoreClock` global is placed in `.boot_data` (`__SECTION_BOOT_DATA`)
so it is accessible here.

**What `SystemInit` does:**

```c
void SystemInit(void)
{
    /* Enable FPU coprocessors CP10 and CP11 (full access) */
    SCB->CPACR |= ((3UL << 10U*2U) | (3UL << 11U*2U));

    /* Optional VTOR relocation (controlled by USER_VECT_TAB_ADDRESS define) */
#if defined(USER_VECT_TAB_ADDRESS)
    SCB->VTOR = FLASH_BASE | 0x00000000U;
#endif
}
```

**What `SystemInit` does NOT do:**
- Does not configure PLL or change SYSCLK — the MCU is still running on HSI at
  16 MHz at this point.
- Does not call any HAL function — HAL is not yet initialised.
- Does not touch any peripheral clock gate.

> Clock configuration is deferred to `drv_rcc_clock_init()` in `main()` because
> it requires the STM32 HAL (specifically `HAL_RCC_OscConfig` and
> `HAL_RCC_ClockConfig`), and HAL requires `HAL_Init()` to be called first.

**CPU state after SystemInit:**
- FPU enabled (CP10/CP11 full access via `SCB->CPACR`)
- SYSCLK = HSI = 16 MHz
- Flash latency = reset default (0 wait-states — insufficient for 100 MHz, will
  be corrected by `_stm32_clock_init()` later)
- Interrupts disabled

### 1.3 .data Section Copy (Flash → RAM)

C global and static variables with non-zero initialisers live in Flash at link
time (LMA = Load Memory Address) and must be copied to RAM before `main()`.
The linker script defines multiple `.data`-like sections for different firmware
layers. `Reset_Handler` copies each one individually:

```c
/* 1. Boot section initialised data */
for (dst = &_sdata_boot, src = &__boot_data_lma; dst < &_edata_boot; )
    *dst++ = *src++;

/* 2. OS section initialised data */
for (dst = &_sdata_os, src = &__os_data_lma; dst < &_edata_os; )
    *dst++ = *src++;

/* 3. Application section initialised data */
for (dst = &_sdata_app, src = &__app_data_lma; dst < &_edata_app; )
    *dst++ = *src++;

/* 4. Default .data section */
for (dst = &_sdata, src = &_sidata; dst < &_edata; )
    *dst++ = *src++;
```

| Section | Linker symbols | Contents |
|---------|---------------|----------|
| `.boot_data` | `_sdata_boot` … `_edata_boot` | `SystemCoreClock`, other pre-scheduler globals marked `__SECTION_BOOT_DATA` |
| `.os_data` | `_sdata_os` … `_edata_os` | OS-layer initialised globals |
| `.app_data` | `_sdata_app` … `_edata_app` | Application-layer initialised globals |
| `.data` | `_sdata` … `_edata` | All other initialised globals (default section) |

After this loop all C global variables have their compile-time initial values.

### 1.4 .bss Zero-Fill

Zero-initialised globals (declared without an explicit initialiser) occupy the
`.bss` section. The linker allocates RAM space for them but stores nothing in
Flash. `Reset_Handler` zeroes this region explicitly:

```c
for (dst = &_sbss; dst < &_ebss; )
    *dst++ = 0;
```

After this loop all uninitialised C globals are guaranteed to be `0`, as
required by the C standard.

### 1.5 C++ Static Constructors

```c
__libc_init_array();
```

This Newlib function calls the array of static constructor function pointers
(`.init_array` section) accumulated by the linker. For pure C projects this is
a no-op. For C++ projects it runs global object constructors in link order.

**After `__libc_init_array` returns**, the full C runtime environment is ready:
- Stack is valid
- All `.data` sections are initialised
- All `.bss` sections are zeroed
- All global objects are constructed

`Reset_Handler` then calls `main()`.

---

## Phase 2 — main()

**File:** [init/main.c:62](../init/main.c#L62)

`main()` is marked `__SECTION_BOOT __USED` so it is placed in `.boot_text` and
never optimised away by the linker.

At entry to `main()`:
- CPU clock: HSI 16 MHz (PLL not yet started)
- Interrupts: disabled
- FreeRTOS scheduler: not yet started
- No FreeRTOS task exists

### 2.1 HAL_Init

```c
HAL_Init();
```

**STM32 HAL source:** `arch/devices/STM/stm32f4xx-hal-driver/Src/stm32f4xx_hal.c`

`HAL_Init()` performs the following in order:

| Step | What happens | Why |
|------|-------------|-----|
| Set Flash prefetch / instruction cache | `FLASH->ACR` bits | Improves code fetch performance |
| Set NVIC priority grouping | `HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` | All 4 bits = preemption priority, 0 bits = sub-priority — required by FreeRTOS |
| Call `HAL_MspInit()` | Weak callback in `hal_msp_stm32.c` | Board-level low-level init (NVIC, SysTick config) |
| Call `HAL_InitTick(TICK_INT_PRIORITY)` | **Overridden** by `hal_timebase_stm32.c` | Normally configures SysTick — overridden here to configure TIM1 instead so SysTick is free for FreeRTOS |

**TIM1 timebase override** ([drivers/hal/stm32/hal_timebase_stm32.c](../drivers/hal/stm32/hal_timebase_stm32.c)):

The STM32 HAL normally uses SysTick for its 1 ms tick. FreeRTOS also needs
SysTick for its scheduler. The conflict is resolved by providing a strong
definition of `HAL_InitTick()` that configures **TIM1** to generate an update
interrupt at 1 kHz (1 ms period). From this point:

| Tick source | Consumer | Frequency |
|------------|---------|-----------|
| TIM1 (via `TIM1_UP_TIM10_IRQHandler`) | STM32 HAL — `HAL_GetTick()`, `HAL_Delay()`, timeouts | 1 kHz |
| SysTick | FreeRTOS — `xTaskGetTickCount()`, `vTaskDelay()` | Configured by `configTICK_RATE_HZ` |

**CPU state after `HAL_Init()`:**
- TIM1 running, generating 1 ms interrupts (but IRQs still masked by `PRIMASK`)
- NVIC priority grouping = Group 4
- HAL tick source active

### 2.2 drv_rcc_clock_init — PLL Bring-Up

```c
drv_rcc_clock_init();
```

**File:** [drivers/drv_rcc.c:35](../drivers/drv_rcc.c#L35)

`drv_rcc_clock_init()` is the generic RCC dispatch layer. It calls
`_hal_rcc_stm32_register()` to bind the STM32 ops table, then invokes
`ops->clock_init()`, which resolves to `_stm32_clock_init()` in
[drivers/hal/stm32/hal_rcc_stm32.c:126](../drivers/hal/stm32/hal_rcc_stm32.c#L126).

**Call chain:**

```
drv_rcc_clock_init()                        [drv_rcc.c]
  └─ _hal_rcc_stm32_register(&_ops)         [drv_rcc.c]
       └─ *ops_out = &_stm32_rcc_ops        [hal_rcc_stm32.c]
  └─ _ops->clock_init()
       └─ _stm32_clock_init()               [hal_rcc_stm32.c]
            ├─ __HAL_RCC_PWR_CLK_ENABLE()   — enable PWR peripheral clock
            ├─ __HAL_PWR_VOLTAGESCALING_CONFIG(SCALE1)
            │      — required for SYSCLK > 84 MHz on F411
            ├─ HAL_RCC_OscConfig(&osc)
            │      — enable HSI (16 MHz internal oscillator)
            │      — configure PLL: PLLM=16, PLLN=200, PLLP=2, PLLQ=4
            │      — start PLL, wait for PLLRDY lock
            └─ HAL_RCC_ClockConfig(&clk, RCC_FLASH_LATENCY)
                   — switch SYSCLK source to PLLCLK
                   — set AHB divider = /1  → HCLK  = 100 MHz
                   — set APB1 divider = /2 → PCLK1 =  50 MHz
                   — set APB2 divider = /1 → PCLK2 = 100 MHz
                   — set Flash latency = 3 wait-states
```

**PLL arithmetic:**

```
HSI = 16 MHz
VCO input  = HSI / PLLM = 16 / 16 = 1 MHz    (must be 1–2 MHz)
VCO output = VCO_in × PLLN = 1 × 200 = 200 MHz
SYSCLK     = VCO_out / PLLP = 200 / 2 = 100 MHz
USB/SDIO   = VCO_out / PLLQ = 200 / 4 = 50 MHz
```

**CPU state after `drv_rcc_clock_init()`:**
- SYSCLK = 100 MHz (from PLL)
- HCLK = 100 MHz
- PCLK1 = 50 MHz (APB1 — I2C, USART2, SPI2/3, TIM2–5)
- PCLK2 = 100 MHz (APB2 — USART1/6, SPI1/4/5, TIM1/9/10/11)
- Flash latency = 3 wait-states

### 2.3 drv_cpu_interrupt_prio_set — NVIC Configuration

```c
drv_cpu_interrupt_prio_set();
```

**File:** [drivers/drv_cpu.c:20](../drivers/drv_cpu.c#L20)

Sets the SVCall exception priority required by FreeRTOS:

```c
void drv_cpu_interrupt_prio_set(void)
{
    NVIC_SetPriority(SVCall_IRQn, 0U);
}
```

FreeRTOS uses `SVC` (software interrupt) to trigger the first context switch and
for privileged-mode system calls. Setting it to priority 0 (highest) ensures it
is never preempted by any peripheral interrupt.

> All peripheral interrupts that call FreeRTOS API (e.g. `xQueueSendFromISR`)
> must be configured at priorities `≥ configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`.
> The HAL MSP callbacks (`HAL_UART_MspInit` etc.) set individual NVIC entries
> when peripherals are registered by the management threads later.

### 2.4 ipc_queues_init — IPC Ring Buffers

```c
ipc_queues_init();
```

**File:** [init/main.c:34](../init/main.c#L34) (static function)

```c
static void ipc_queues_init(void)
{
    ipc_mqueue_init();              // reset the mqueue list sentinel

    for (uint8_t i = 0; i < NO_OF_UART; i++) {
        global_uart_tx_mqueue_list[i] = ipc_mqueue_register(
            IPC_MQUEUE_TYPE_UART_HW, i, sizeof(uint8_t), PIPE_UART_1_DRV_TX_SIZE);

        global_uart_rx_mqueue_list[i] = ipc_mqueue_register(
            IPC_MQUEUE_TYPE_UART_HW, i, sizeof(uint8_t), PIPE_UART_1_DRV_RX_SIZE);
    }
}
```

**What this creates:**

For every UART channel (`NO_OF_UART` comes from `app/board/mcu_config.h`,
generated from the board XML):

1. A **TX ring buffer** (`IPC_MQUEUE_TYPE_UART_HW`) — a `struct ringbuffer`
   backed by a heap-allocated `uint8_t[]` of `PIPE_UART_1_DRV_TX_SIZE` bytes.
   ID stored in `global_uart_tx_mqueue_list[i]`.

2. An **RX ring buffer** — same structure, `PIPE_UART_1_DRV_RX_SIZE` bytes.
   ID stored in `global_uart_rx_mqueue_list[i]`.

**File:** [ipc/mqueue.c:58](../ipc/mqueue.c#L58)

The `kmaloc()` calls inside `ipc_mqueue_register()` allocate from the FreeRTOS
heap (`heap_4.c` — `pvPortMalloc`). This is valid before the scheduler starts
because `heap_4.c` does not require the scheduler to be running for allocation.

These ring buffers are the **only** communication channel between the UART ISR
and the management thread / application:

```
UART ISR → ringbuffer_putchar(rx_rb, byte)
                           ↑
app reads ← uart_mgmt_read_byte() ← ringbuffer_getchar(rx_rb)
```

### 2.5 uart_mgmt_start — Management Thread Creation

```c
uart_mgmt_start();
```

**File:** [services/uart_mgmt.c](../services/uart_mgmt.c) (inside `#if INC_SERVICE_UART_MGMT == 1`)

`uart_mgmt_start()` does two things and returns immediately:

```c
int32_t uart_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(UART_MGMT_QUEUE_DEPTH, sizeof(uart_mgmt_msg_t));
    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(uart_mgmt_thread, "uart_mgmt",
                                   PROC_SERVICE_SERIAL_MGMT_STACK_SIZE,
                                   PROC_SERVICE_SERIAL_MGMT_PRIORITY, NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}
```

1. **`xQueueCreate`** — allocates the management message queue on the FreeRTOS
   heap. Messages of type `uart_mgmt_msg_t` can be queued `UART_MGMT_QUEUE_DEPTH`
   deep. The queue is empty and ready at this point.

2. **`os_thread_create`** — calls `xTaskCreate()`, which allocates the task stack
   and TCB on the FreeRTOS heap and adds `uart_mgmt_thread` to the ready list.
   The task does **not** run yet — `vTaskStartScheduler()` has not been called.

> `uart_mgmt_thread` begins with `os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT)`
> (4000 ms). Even after the scheduler starts the task sleeps and performs no
> hardware access until that timer expires. This is intentional — hardware init
> inside a task avoids the IRQ-disabled pre-scheduler window.

### 2.6 printk_init

```c
printk_init();
```

**File:** `kernel/kernel_syscall.c`

Binds the `printk()` function to the TX ring buffer of `COMM_PRINTK_HW_ID`
(the designated debug UART). After this call, `printk("msg")` places bytes into
the ring buffer. When `uart_mgmt` wakes at 4000 ms and HAL initialises the UART,
those bytes will be transmitted.

> `printk_init()` must be called **after** `ipc_queues_init()` because it looks
> up the TX queue ID from `global_uart_tx_mqueue_list[]`.

### 2.7 app_main — Application Task Creation

```c
app_main();
```

**File:** [app/app_main.c:94](../app/app_main.c#L94)

`app_main` is declared `__attribute__((weak))` in `main.c` so that projects
without an application can link without an error. The strong definition in
`app/app_main.c` overrides it.

`app_main()` creates the three demonstration tasks and returns immediately to
`main()`:

```c
int app_main(void)
{
    os_thread_create(heartbeat_task, "heartbeat",
                     APP_HEARTBEAT_STACK, APP_HEARTBEAT_PRIO, NULL);

    os_thread_create(hello_task,     "hello",
                     APP_HELLO_STACK,    APP_HELLO_PRIO,    NULL);

    os_thread_create(echo_task,      "echo",
                     APP_ECHO_STACK,     APP_ECHO_PRIO,     NULL);

    return 0;
}
```

Each `os_thread_create()` call allocates a stack and TCB on the FreeRTOS heap
and adds the task to the scheduler's ready list. None of these tasks run until
`vTaskStartScheduler()` is called. All three tasks are currently in the
**Ready** state.

**Tasks registered at this point:**

| Task | Function | Stack (words) | Priority | State |
|------|----------|--------------|----------|-------|
| `uart_mgmt` | `uart_mgmt_thread` | 512 | 1 | Ready (will immediately delay) |
| `heartbeat` | `heartbeat_task` | 256 | 2 | Ready |
| `hello` | `hello_task` | 512 | 1 | Ready |
| `echo` | `echo_task` | 512 | 1 | Ready |

---

## Phase 3 — vTaskStartScheduler

```c
vTaskStartScheduler();
```

**FreeRTOS source:** `kernel/FreeRTOS-Kernel/tasks.c`

`vTaskStartScheduler()` performs these steps internally:

| Step | Action |
|------|--------|
| 1 | Create the **Idle task** (`prvIdleTask`, priority 0) |
| 2 | Create the **Timer task** (if `configUSE_TIMERS == 1`) |
| 3 | Disable interrupts (`taskDISABLE_INTERRUPTS()`) |
| 4 | Call `xPortStartScheduler()` → Cortex-M4 port (`port.c`) |
| 5 | Configure SysTick: `SysTick->LOAD`, `SysTick->VAL`, `SysTick->CTRL` |
| 6 | Set PendSV to lowest priority (for context switches) |
| 7 | Call `prvStartFirstTask()` — inline assembly that triggers the first SVC |
| 8 | SVC_Handler runs `vPortSVCHandler()` → loads first task context, sets PSP, enables interrupts |

**From this point `vTaskStartScheduler()` does not return** (under normal
operation). Control passes to the highest-priority ready task, which is
`heartbeat_task` (priority 2 — the only task at priority 2).

---

## Phase 4 — Post-Scheduler: Management Thread Startup

After the scheduler starts, all tasks compete for CPU time. The management
threads begin with `os_thread_delay()` calls, which put them in the
**Blocked** state immediately. The application tasks run freely during the
delay window.

```
t = 0 ms       Scheduler starts
               heartbeat_task runs (priority 2, highest) → gpio_mgmt_post() →
                 posts to gpio_mgmt queue → gpio_mgmt_start() was NOT called
                 in main.c yet… (see note below)
               heartbeat_task delays 500 ms → Blocked

t = 0..4000 ms hello_task, echo_task, heartbeat_task cycle
               uart_mgmt_thread is Blocked (TIME_OFFSET_SERIAL_MANAGEMENT = 4000 ms)
               No UART hardware has been initialised yet

t = 3000 ms    gpio_mgmt_thread wakes (TIME_OFFSET_GPIO_MANAGEMENT)
               → board_get_config() iterates gpio_table
               → for each GPIO pin:
                    board_gpio_clk_enable(port)      ← __HAL_RCC_GPIOx_CLK_ENABLE()
                    hal_gpio_stm32_set_config(h, …)  ← store parameters in h->hw
                    drv_gpio_register(dev_id, ops)   ← ops->hw_init() → HAL_GPIO_Init()
                    if initial_state: ops->set(h)    ← apply boot output level
               → enters message loop

t = 4000 ms    uart_mgmt_thread wakes (TIME_OFFSET_SERIAL_MANAGEMENT)
               → board_get_config() iterates uart_table
               → for each UART:
                    hal_uart_stm32_set_config(h, instance, …)
                    drv_uart_register(dev_id, ops, baudrate, 10)
                      → ops->hw_init(h) → HAL_UART_Init()
                      → ops->start_rx_it(h)          ← arms UART RX interrupt
               → enters message loop
               UART is now live — printk output begins transmitting

t = 5500 ms    spi_mgmt_thread wakes → HAL_SPI_Init for all SPI buses

t = 6500 ms    iic_mgmt_thread wakes → HAL_I2C_Init for all I2C buses
```

> **Note:** `gpio_mgmt_start()`, `spi_mgmt_start()`, and `iic_mgmt_start()` are
> called from `app_main()` (not shown in the minimal example here) or from a
> separate OS init function. The ordering constraint is only that each
> `xxx_mgmt_start()` is called before `vTaskStartScheduler()` — the actual
> hardware init happens inside the task, after the delay expires.

---

## Memory Map and Section Placement

The boot sequence depends on the linker script placing sections correctly.

```
0x08000000  FLASH_BOOT (16 KB)
│
│  .isr_vector     g_pfnVectors[]            ← CPU fetches SP + PC from here
│  .boot_text      Reset_Handler             ← __SECTION_BOOT
│                  SystemInit
│                  main
│  .boot_data_lma  initialised .boot_data    ← copied to RAM by Reset_Handler
│
0x08004000  FLASH_OS (remaining Flash)
│  .text            all other .text
│  .os_data_lma     initialised .os_data     ← copied to RAM
│  .app_data_lma    initialised .app_data    ← copied to RAM
│  .data_lma        initialised .data        ← copied to RAM

0x20000000  SRAM (128 KB on F411)
│  .boot_data       SystemCoreClock, …       ← copied from .boot_data_lma
│  .os_data         OS-layer globals
│  .app_data        App-layer globals
│  .data            all other initialiseds
│  .bss             zero-filled globals
│  FreeRTOS heap    tasks, queues, stacks    ← pvPortMalloc from here
│          ↑ grows upward
│          ↓ grows downward
0x20020000  _estack (top of SRAM = initial SP)
```

---

## Interrupt State Through Boot

```
Power-on             PRIMASK = 1  (interrupts disabled by Cortex-M4 reset)
                      ↓
Reset_Handler        PRIMASK = 1  (disabled, never changed)
  SystemInit         PRIMASK = 1
  .data/.bss init    PRIMASK = 1
  __libc_init_array  PRIMASK = 1
                      ↓
main()               PRIMASK = 1
  HAL_Init()         PRIMASK = 1   (TIM1 configured but IRQ pending mask active)
  drv_rcc_clock_init PRIMASK = 1
  drv_cpu_*          PRIMASK = 1
  ipc_queues_init    PRIMASK = 1
  uart_mgmt_start    PRIMASK = 1   (task created, not running)
  printk_init        PRIMASK = 1
  app_main           PRIMASK = 1   (tasks created, not running)
                      ↓
vTaskStartScheduler  ↓
  xPortStartScheduler
    SysTick init
    PendSV priority
    prvStartFirstTask
      SVC #0 → vPortSVCHandler
        PRIMASK = 0  ← INTERRUPTS ENABLED HERE for the first time
        First task begins executing
```

Interrupts are first enabled by the FreeRTOS SVC handler at the moment the
very first task context is loaded. No peripheral interrupt can fire before this
point regardless of peripheral configuration.

---

## Call Stack Summary

```
[hardware reset]
  └─ Reset_Handler()                             startup_stm32f411vetx.c
       ├─ [asm] ldr sp, [g_pfnVectors]           — SP = _estack
       ├─ SystemInit()                            hal_rcc_stm32.c
       │    └─ SCB->CPACR |= FPU_enable
       ├─ [loop] copy .boot_data LMA → VMA
       ├─ [loop] copy .os_data   LMA → VMA
       ├─ [loop] copy .app_data  LMA → VMA
       ├─ [loop] copy .data      LMA → VMA
       ├─ [loop] zero .bss
       ├─ __libc_init_array()                     newlib (C++ ctors)
       └─ main()                                  init/main.c
            ├─ HAL_Init()                         stm32f4xx_hal.c
            │    ├─ NVIC priority grouping = 4
            │    ├─ HAL_MspInit()                 hal_msp_stm32.c
            │    └─ HAL_InitTick()  [overridden]  hal_timebase_stm32.c
            │         └─ TIM1 → 1 kHz update IRQ
            ├─ drv_rcc_clock_init()               drv_rcc.c
            │    ├─ _hal_rcc_stm32_register()     hal_rcc_stm32.c
            │    └─ _stm32_clock_init()           hal_rcc_stm32.c
            │         ├─ HAL_RCC_OscConfig()      — HSI → PLL lock
            │         └─ HAL_RCC_ClockConfig()    — SYSCLK = 100 MHz
            ├─ drv_cpu_interrupt_prio_set()        drv_cpu.c
            │    └─ NVIC_SetPriority(SVCall, 0)
            ├─ ipc_queues_init()                  init/main.c (static)
            │    ├─ ipc_mqueue_init()             ipc/mqueue.c
            │    └─ ipc_mqueue_register() ×2N     ipc/mqueue.c
            │         └─ kmaloc(ringbuffer)       → FreeRTOS heap
            ├─ uart_mgmt_start()                  services/uart_mgmt.c
            │    ├─ xQueueCreate()                → FreeRTOS heap
            │    └─ os_thread_create("uart_mgmt") → xTaskCreate → FreeRTOS heap
            ├─ printk_init()                      kernel/kernel_syscall.c
            │    └─ bind printk → TX ring buffer [global_uart_tx_mqueue_list]
            ├─ app_main()                         app/app_main.c
            │    ├─ os_thread_create("heartbeat") → xTaskCreate
            │    ├─ os_thread_create("hello")     → xTaskCreate
            │    └─ os_thread_create("echo")      → xTaskCreate
            └─ vTaskStartScheduler()              FreeRTOS-Kernel/tasks.c
                 ├─ xTaskCreate("IDLE")
                 ├─ xPortStartScheduler()         portable/GCC/ARM_CM4F/port.c
                 │    ├─ SysTick config
                 │    ├─ PendSV = lowest priority
                 │    └─ prvStartFirstTask()      [SVC #0]
                 └─ [does not return]
                      → vPortSVCHandler()
                           └─ PRIMASK = 0  ← interrupts enabled
                                → heartbeat_task() starts (priority 2)
```

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
