# OS_INSIDE — FreeRTOS-OS Internals & Post-Mortems

Deep-dive notes on how FreeRTOS-OS works under the hood, and documented post-mortems for non-obvious bugs that were traced to root cause and fixed.

---

## Post-Mortem: USART2 Silent — ISR Priority + Non-ISR-Safe ringbuffer_getchar

### Symptom

After bringing up the scheduler, USART1 (UART_DEBUG) produced output but USART2 (UART_APP, `/dev/ttyUSB0`) remained completely silent. `uart_mgmt_thread` iterates `bc->uart_count` UARTs in order — UART_DEBUG first, UART_APP second. A live memory dump via OpenOCD confirmed that `drv_uart_handle_t` for UART_APP (`_uart_handles[1]`) was entirely zero: `initialized == 0`, `ops == NULL`. The thread froze during iteration 0 before ever reaching iteration 1.

---

### Investigation: Locating the Freeze

OpenOCD was used to halt the running MCU and read the program counter:

```
> halt
> reg pc
pc: 0x080281f6
```

Disassembly of `vPortEnterCritical` at `0x080281ac`:

```asm
80281b2:  mov.w  r3, #80         ; BASEPRI = 0x50 (configMAX_SYSCALL_INTERRUPT_PRIORITY)
80281b6:  msr    BASEPRI, r3
80281c6:  ldr    r3, [uxCriticalNesting]
80281ca:  adds   r3, #1
80281cc:  str    r3, [uxCriticalNesting]
80281d4:  cmp    r3, #1           ; if (uxCriticalNesting == 1) — first entry
80281d6:  bne.n  80281fa
80281d8:  ldr    r3, [0xE000ED04] ; SCB->ICSR
80281dc:  uxtb   r3, r3           ; bits[7:0] = VECTACTIVE
80281de:  cmp    r3, #0
80281e0:  beq.n  80281fa          ; skip assert if NOT in interrupt
; configASSERT((VECTACTIVE == 0)) failed:
80281e2:  mov.w  r3, #80         ; taskDISABLE_INTERRUPTS
80281e6:  msr    BASEPRI, r3
80281f6:  nop                    ; ← PC STUCK HERE
80281f8:  b.n    80281f6          ; for(;;)
```

The MCU was frozen in the `configASSERT` infinite loop inside `vPortEnterCritical`. The assert fires when `taskENTER_CRITICAL()` is called from inside an interrupt handler (`VECTACTIVE != 0`).

---

### Investigation: Tracing the ISR Call Chain

Reading `ICSR` while halted confirmed `VECTACTIVE = 37` — exception 37 on STM32F4 is `USART1_IRQn`. The ISR had preempted the scheduler and was stuck in the assert.

The full call chain was traced by disassembling each function:

```
USART1_IRQHandler                    (irq_periph_dispatch_generated.c)
  └─ hal_uart_stm32_irq_handler()    (drivers/hal/stm32/hal_uart_stm32.c)
       ├─ drv_irq_dispatch_from_isr()  → RX path (ISR-safe)
       └─ drv_uart_tx_get_next_byte()  ← TX empty path
            └─ ringbuffer_getchar()    (ipc/ringbuffer.c)
                 └─ taskENTER_CRITICAL()  ← calls vPortEnterCritical
                                           VECTACTIVE ≠ 0 → configASSERT → for(;;)
```

**`drv_uart_tx_get_next_byte`** is called from the USART TX-empty ISR to pull the next byte from the TX ring buffer. It delegates directly to `ringbuffer_getchar`, which uses the task-context-only `taskENTER_CRITICAL` / `taskEXIT_CRITICAL` pair. These increment `uxCriticalNesting` and, on first entry, assert that `VECTACTIVE == 0`. When called from an ISR, the assert fires immediately.

The RX path (`_uart_rx_cb` → `ringbuffer_putchar`) was already ISR-safe and was not involved in the freeze.

---

### Root Cause 1 — Non-ISR-Safe `ringbuffer_getchar` in TX ISR Path

`ringbuffer_getchar` was documented as "task context only" and used `taskENTER_CRITICAL`. Its caller `drv_uart_tx_get_next_byte` was documented as "called from the UART TXE ISR" but used the non-ISR-safe getchar. The two were inconsistent.

**Fix:** Added `ringbuffer_getchar_from_isr` using `taskENTER_CRITICAL_FROM_ISR` / `taskEXIT_CRITICAL_FROM_ISR`, matching the pattern already established by `ringbuffer_putchar`:

```c
/* ipc/ringbuffer.c */
uint32_t ringbuffer_getchar_from_isr(struct ringbuffer *rb, uint8_t *ch)
{
    UBaseType_t ux = taskENTER_CRITICAL_FROM_ISR();
    uint32_t ret = 0;
    if (_data_len(rb) > 0) {
        *ch = rb->buffer_ptr[rb->read_index];
        if (rb->read_index == (uint16_t)(rb->buffer_size - 1)) {
            rb->read_mirror = (uint16_t)(~rb->read_mirror) & 1U;
            rb->read_index  = 0;
        } else {
            rb->read_index++;
        }
        ret = 1;
    }
    taskEXIT_CRITICAL_FROM_ISR(ux);
    return ret;
}
```

`drv_uart_tx_get_next_byte` was updated to call `ringbuffer_getchar_from_isr`.

---

### Root Cause 2 — UART IRQ Priorities Above `configMAX_SYSCALL_INTERRUPT_PRIORITY`

FreeRTOS on Cortex-M enforces a strict priority boundary. The kernel uses `BASEPRI` to create critical sections. Any ISR with a **numerically lower** NVIC priority value than `configMAX_SYSCALL_INTERRUPT_PRIORITY` runs above this BASEPRI mask — it can preempt FreeRTOS critical sections in task context, and calling **any** FreeRTOS API from it (even `FROM_ISR` variants) is undefined behavior.

The rule:

```
configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY = 5   (from autoconf.h)
configMAX_SYSCALL_INTERRUPT_PRIORITY   = 0x50 (= 5 << (8 - 4 prio bits))

ISRs that call FreeRTOS API must have NVIC priority >= 5 (numerically)
i.e., they must NOT be more urgent than priority 5
```

The UART ISRs were configured at NVIC priority `2U` — numerically lower than 5, meaning **higher hardware priority** than the FreeRTOS mask. This meant:

- `taskENTER_CRITICAL()` in task context sets `BASEPRI = 0x50`
- USART1 ISR at NVIC priority 0x20 (register value for library priority 2) is **not blocked** by `BASEPRI = 0x50`
- The ISR fires inside a task critical section, calls `ringbuffer_getchar` → `taskENTER_CRITICAL` → configASSERT fires

I2C and SPI were similarly configured at priority `3U` (also above the FreeRTOS-safe boundary).

**Fix:** All peripheral ISRs that call FreeRTOS API were moved to priority `5U` in both `irq_hw_init_generated.c` and the source `irq_table.xml`:

| Peripheral | Old priority | New priority |
|---|---|---|
| USART1 (UART_DEBUG) | 2 | 5 |
| USART2 (UART_APP)   | 2 | 5 |
| I2C1 Event/Error    | 3 | 5 |
| SPI1                | 3 | 5 |
| EXTI0 (BTN_USER)    | 6 | 6 (already OK) |
| TIM1_UP (HAL tick)  | 1 | 1 (does not call FreeRTOS API) |

---

### FreeRTOS Priority Rule Summary

On a Cortex-M4 with 4-bit NVIC priority field (STM32F4):

```
Priority 0–4  : "Fast" ISRs — MUST NOT call any FreeRTOS API
                (they run above the BASEPRI mask; calling API is undefined behavior)

Priority 5–14 : "Safe" ISRs — MAY call FreeRTOS FROM_ISR API
                (configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY = 5)

Priority 15   : Lowest (configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
                Used for SysTick / PendSV
```

The register values written to the NVIC `IPR` byte are shifted left by `(8 - configPRIO_BITS)`:

| Library priority | IPR register value | Preempts BASEPRI=0x50? |
|---|---|---|
| 1 | 0x10 | YES — unsafe |
| 2 | 0x20 | YES — unsafe |
| 5 | 0x50 | NO — safe boundary |
| 6 | 0x60 | NO — safe |
| 15 | 0xF0 | NO — safe |

---

### Diagnosis Tools Used

**OpenOCD + Tcl script to dump live memory:**

```tcl
# dump_handle.tcl — read drv_uart_handle_t[1] at runtime
proc dump_word {addr} {
    set v [mrw $addr]
    format "0x%08x: 0x%08x" $addr $v
}
set base 0x2000xxxx   ;# &_uart_handles[1] from nm
for {set i 0} {$i < 23} {incr i} {
    set addr [expr {$base + $i * 4}]
    puts [dump_word $addr]
}
```

Run with:
```bash
openocd -f arch/debug_cfg/stm32_f411xx_debug.cfg \
        -c "init; halt" \
        -f dump_handle.tcl \
        -c "shutdown"
```

**Disassembly check for `vPortEnterCritical` call sites:**

```bash
arm-none-eabi-objdump -d build/app.elf | grep -B2 "bl.*vPortEnterCritical"
```

**Verify the fix — confirm `drv_uart_tx_get_next_byte` calls the ISR-safe variant:**

```bash
arm-none-eabi-objdump -d build/app.elf \
  | awk '/^.*<drv_uart_tx_get_next_byte>:/,/^[0-9a-f]{8} </' \
  | grep "bl "
# Expected output:
#  801d6de:  f00b fd3f  bl  8029160 <ringbuffer_getchar_from_isr>
```

---

### Files Changed

| File | Change |
|---|---|
| `ipc/ringbuffer.c` | Added `ringbuffer_getchar_from_isr` (ISR-safe, uses `taskENTER_CRITICAL_FROM_ISR`) |
| `include/ipc/ringbuffer.h` | Declared `ringbuffer_getchar_from_isr` |
| `drivers/drv_uart.c` | `drv_uart_tx_get_next_byte` now calls `ringbuffer_getchar_from_isr` |
| `app/board/irq_hw_init_generated.c` | UART/I2C/SPI bind priorities `2U`/`3U` → `5U` |
| `app/board/irq_table.xml` | Same priority fix in source XML (prevents regression on `make irq_gen`) |

---

### Prevention Guidelines

1. **Any function documented "called from ISR" must only use `FROM_ISR` FreeRTOS API.** Check call sites when writing a new ISR or callback.

2. **All peripheral IRQ priorities in `irq_table.xml` must be `>= configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY`.** The only exceptions are bare-metal ISRs that call zero FreeRTOS API (e.g. HAL timebase timer).

3. **Run this after every `irq_table.xml` regeneration:**
   ```bash
   arm-none-eabi-objdump -d build/app.elf | grep "bl.*vPortEnterCritical" | grep -v "taskENTER_CRITICAL\b"
   ```
   Any hit inside an ISR handler is a bug.

4. **The `BASEPRI` / `VECTACTIVE` assert in `vPortEnterCritical` is intentional.** If the MCU freezes there, it always means `taskENTER_CRITICAL` was called from interrupt context. Find the ISR in the call stack and replace with the `FROM_ISR` variant.

---

## ISR Dispatch Architecture

FreeRTOS-OS uses a Linux-inspired two-level IRQ dispatch to decouple hardware vectors from application handlers.

### Layer 1 — Hardware Vector (generated)

`app/board/irq_periph_dispatch_generated.c` provides the actual `USART1_IRQHandler` etc. These are thin wrappers that call the HAL peripheral driver (to clear the hardware flag) and then call `drv_irq_dispatch_from_isr(irq_id, &data, &hpt)`.

### Layer 2 — Software irq_desc Chain

`drv_irq_dispatch_from_isr` → `__do_IRQ_from_isr` → `irq_to_desc(irq_id)` → `desc->handle_irq()` → `handle_irq_event()` → walks the `irqaction` chain, calling each registered handler.

```
Hardware IRQ fires
      │
      ▼  CPU vector table → irq_periph_dispatch_generated.c
drv_irq_dispatch_from_isr(IRQ_ID_xxx, &data, &hpt)
      │
      ▼  __do_IRQ_from_isr()
irq_to_desc(irq_id) → desc->handle_irq() → handle_irq_event()
      │
      ▼  walk irqaction chain
irq_handler_t  (registered via request_irq)
  — or —
irq_notify_cb_t trampoline  (registered via irq_register)
      │
      ▼  vTaskNotifyGiveFromISR / ringbuffer_putchar / etc.
FreeRTOS task unblocks
```

### Registering a Handler

**Linux-style `request_irq`** (direct `irqaction`, returns `irqreturn_t`):
```c
request_irq(IRQ_ID_EXTI(0), _btn_irq_handler, "btn_user", task_handle);
```

**Notify-callback `irq_register`** (wraps `irq_notify_cb_t` in a trampoline):
```c
irq_register(IRQ_ID_UART_RX(UART_APP), _echo_uart_rx_cb, ringbuffer_ptr);
```

Both must follow the ISR-safe API rules above.

---

## Post-Mortem: Shell Silent — Four Independent Defects

### Symptom

After wiring the interactive shell service (`os_shell_management.c`) to `app_main()` and connecting PuTTY to `/dev/ttyUSB0` (USART2, 115200 8N1), no banner appeared and keystrokes produced no response. The MCU was running (heartbeat LED toggling), USART2 was initialized (previous `[APP] Hello` messages had worked), but the shell was completely silent.

---

### Root Cause 1 — `os_shell_management.c` Never Compiled

`services/Makefile` controlled which `.c` files were included in the build. `os_shell_management.c` was present in `services/` but had no entry in the Makefile. The linker silently discarded the compilation unit, so `os_shell_mgmt_start()` resolved to its `__attribute__((weak))` stub in `main.c` (which does nothing).

**Evidence:** `arm-none-eabi-nm build/app.elf | grep os_shell_mgmt_start` returned `os_shell_mgmt_start` at a suspiciously low address with size 0 — the weak stub.

**Fix:** Added the guarded entry to `services/Makefile`:

```makefile
ifeq ($(CONFIG_INC_SERVICE_OS_SHELL_MGMT),1)
obj-y += services/os_shell_management.o
endif
```

`CONFIG_INC_SERVICE_OS_SHELL_MGMT = 1` was already set in `autoconf.h` from Kconfig, so the file compiled in immediately.

---

### Root Cause 2 — `FreeRTOS_CLI.h` Missing `FreeRTOS.h` Include

`FreeRTOS_CLI.h` uses `BaseType_t` and `UBaseType_t` (FreeRTOS-defined types) without including `FreeRTOS.h` itself. The header relies on the caller to pull in FreeRTOS types first. The `os_shell_management.h` public header included `FreeRTOS_CLI.h` directly after `def_std.h` — FreeRTOS types were not yet in scope, causing compile errors:

```
FreeRTOS_CLI.h:41: error: expected declaration specifiers or '...' before '*' token
FreeRTOS_CLI.h:51: error: unknown type name 'pdCOMMAND_LINE_CALLBACK'
FreeRTOS_CLI.h:95: error: unknown type name 'BaseType_t'
```

**Fix:** Added `#include <FreeRTOS.h>` to `os_shell_management.h` before `FreeRTOS_CLI.h`:

```c
#include <def_std.h>
#include <FreeRTOS.h>        /* ← added: BaseType_t, UBaseType_t required by CLI */
#include <FreeRTOS_CLI.h>
```

---

### Root Cause 3 — `echo_task` Consuming Shell RX Bytes

`app_main.c` ran an `echo_task` that polled the UART_APP RX ring buffer via `uart_mgmt_read_byte(UART_APP, &rx_byte)`. The shell task polls the **same** ring buffer via `ringbuffer_getchar(rx_rb, &byte)`. Both are task-context readers of a single-consumer ring buffer:

```
USART2 RXNE ISR
  └─ ringbuffer_putchar(rx_rb, byte)   ← one producer

echo_task:  ringbuffer_getchar(rx_rb, &b)  ┐
shell_task: ringbuffer_getchar(rx_rb, &b)  ┘ two consumers — race
```

Each byte in the ring buffer could be consumed by either task. In practice `echo_task` ran at the same priority, woke on `ulTaskNotifyTake`, and drained the buffer first. The shell received no bytes, so no commands were processed and no output was sent.

`echo_task` also called `irq_register(IRQ_ID_UART_RX(UART_APP), _echo_uart_rx_cb, NULL)` which added a notification subscriber to the same IRQ chain — waking echo before the shell task could even check.

**Fix:** Removed `echo_task` and `hello_task` from `app_main.c`. The shell is the sole UART_APP RX consumer. `hello_task` was also removed because it wrote `[APP] Hello from FreeRTOS-OS!\r\n` to UART_APP every 2 seconds, which would appear mid-line in any active shell session.

---

### Root Cause 4 — `printk` and Shell on the Same UART

`conf_os.h` had both `COMM_PRINTK_HW_ID` and `UART_SHELL_HW_ID` set to `1` (UART_APP / USART2). `printk()` writes asynchronously to the TX ring buffer from any task. With the shell also writing to the same TX ring buffer, `printk` output would appear interspersed with shell prompts and command output:

```
> heap
[BTN] Button pressed — count 1     ← printk fired mid-command
Heap usage:
  Total  : ...
```

Both writers are individually correct (both use `ringbuffer_putchar` which is ISR-safe), but the interleaving is unacceptable for interactive use.

**Fix:** Moved `printk` to UART_DEBUG (USART1, PA9/PA10 — accessible via the ST-Link virtual COM port):

```c
/* config/conf_os.h */
#define COMM_PRINTK_HW_ID   (0)   /* UART_DEBUG — USART1 — PA9/PA10 — ST-Link VCP */
#define UART_SHELL_HW_ID    (1)   /* UART_APP   — USART2 — PA2/PA3  — USB-Serial  */
```

`btn_task` was also updated to use `printk()` instead of `uart_mgmt_async_transmit(UART_APP, ...)` so button events go to the debug UART, not the shell terminal.

---

### Files Changed

| File | Change |
|---|---|
| `services/Makefile` | Added `os_shell_management.o` guarded by `CONFIG_INC_SERVICE_OS_SHELL_MGMT` |
| `include/services/os_shell_management.h` | Added `#include <FreeRTOS.h>` before `FreeRTOS_CLI.h` |
| `app/app_main.c` | Removed `echo_task` and `hello_task`; added `os_shell_mgmt_start()`; `btn_task` uses `printk()` |
| `config/conf_os.h` | `COMM_PRINTK_HW_ID` changed from `1` → `0` |

---

### Boot Timing After the Fix

```
T + 0 s    MCU reset
T + 4 s    uart_mgmt_thread: USART1 + USART2 initialised (TIME_OFFSET_SERIAL_MANAGEMENT)
T + 5 s    os_shell_task: banner printed, read loop started (TIME_OFFSET_OS_SHELL_MGMT)
```

PuTTY on `/dev/ttyUSB0` at 115200 8N1 (no flow control) shows:

```
=== FreeRTOS-OS Shell ===
Type 'help' for available commands.

>
```

---

### Prevention Guidelines

1. **Every `.c` file in `services/` needs a Makefile entry.** After adding a new service source, verify with `arm-none-eabi-nm build/app.elf | grep <symbol>` that the expected symbols have real addresses and non-zero size.

2. **FreeRTOS+CLI headers require `FreeRTOS.h` to be included first.** Any header that includes `FreeRTOS_CLI.h` must pull in `FreeRTOS.h` immediately before it.

3. **A ring buffer has one logical consumer.** Never have two tasks calling `ringbuffer_getchar` on the same buffer. If multiple consumers are needed, use the IRQ subscriber chain (`irq_register` / `request_irq`) to fan-out notifications, each subscriber writing to its own buffer.

4. **`printk` and the shell must be on different UARTs.** `COMM_PRINTK_HW_ID != UART_SHELL_HW_ID` is a hard requirement for usable interactive sessions. Verify this after any board bring-up where only one physical UART is wired.

---

*Last updated: 2026-04-22 — Post-mortem: USART2 silent / ISR priority + ringbuffer_getchar; Shell silent / Makefile + FreeRTOS.h + ring buffer consumer + UART assignment*
