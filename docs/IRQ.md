# IRQ System

Linux-style interrupt descriptor chain for FreeRTOS / bare-metal Cortex-M4.

---

## Table of Contents

- [Overview](#overview)
- [Common Dispatch Spine](#common-dispatch-spine)
- [UART ISR Chain](#uart-isr-chain)
- [I2C ISR Chain](#i2c-isr-chain)
- [SPI ISR Chain](#spi-isr-chain)
- [Task Notification Mechanism](#task-notification-mechanism)
- [IRQ ID Space](#irq-id-space)
- [Key Data Structures](#key-data-structures)
- [Flow Handlers](#flow-handlers)
- [Registration APIs](#registration-apis)
- [Boot Sequence](#boot-sequence)
- [EXTI Example (app_main.c)](#exti-example)
- [UART Echo Example](#uart-echo-example)
- [IRQ Table Generator](#irq-table-generator)
- [File Map](#file-map)

---

## Overview

The IRQ system adapts the Linux `irq_desc` chain to a single-core embedded
target. Each software IRQ ID maps to a descriptor (`struct irq_desc`) that
holds a chip object (`struct irq_chip`), a flow handler, and a singly-linked
chain of `struct irqaction` nodes — one per registered driver callback.

All peripheral HAL backends (UART, I2C, SPI) dispatch events exclusively
through `drv_irq_dispatch_from_isr()` → `__do_IRQ_from_isr()` → subscriber
chain. No HAL weak callbacks (`HAL_UART_RxCpltCallback`, etc.) are used; the
MSP stubs are no-ops and all completion/error callbacks are no-op overrides.

Two registration paths are available:

| API | Signature | Use case |
|-----|-----------|----------|
| `request_irq()` | `irq_handler_t` — returns `irqreturn_t` | Direct, Linux-faithful; preferred for new code |
| `irq_register()` | `irq_notify_cb_t` — returns `void` | Compatibility wrapper; wraps into an irqaction via trampoline |

Both ultimately insert an `irqaction` into the same `irq_desc` chain.

---

## Common Dispatch Spine

Every peripheral feeds into the same spine once it calls
`drv_irq_dispatch_from_isr`:

```
drv_irq_dispatch_from_isr(irq_id, data, &hpt)     drivers/drv_irq.c
  │
  ▼
__do_IRQ_from_isr(irq_id, data, pxHPT)            irq/irq_desc.c
  ├─ irq_to_desc(irq_id)            O(1) array lookup
  ├─ depth check  (skips if irq_disable() depth > 0)
  └─ desc->handle_irq(desc, data, pxHPT)          flow handler
       │
       ▼  handle_irq_event()
       irqaction->handler(irq, data, dev_id, pxHPT)   ← subscriber #1
       irqaction->next->handler(...)                   ← subscriber #2 (chained)
       ...
       │
       ▼
  portYIELD_FROM_ISR(*pxHPT)   (called by the HAL handler after dispatch returns)
```

---

## UART ISR Chain

UART uses direct register-level handling — no HAL state machine is invoked.
The RXNE bit is enabled inside `stm32_uart_hw_init()`; no separate
`start_rx_it` step is needed.

```
CPU vector table
  │
  ▼  stm32_exceptions.c
USART1_IRQHandler()
  └─ hal_uart_stm32_irq_handler(USART1)        hal_uart_stm32.c
       reads SR / CR1 / CR3 directly
       │
       ├─ RXNE set (no errors)
       │    byte = READ_REG(DR)                clears RXNE
       │    drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(0), &byte, &hpt)
       │      └─► _uart_rx_cb()               uart_mgmt.c
       │               ringbuffer_putchar(rx_rb, byte)
       │
       ├─ RXNE set (with error flags)
       │    byte = READ_REG(DR)                clears SR error flags
       │    drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(0), &byte, &hpt)
       │    drv_irq_dispatch_from_isr(IRQ_ID_UART_ERROR(0), &errorflags, &hpt)
       │      └─► _uart_err_cb()              uart_mgmt.c
       │
       ├─ TXE set — TX ring buffer not empty
       │    byte = ringbuffer_getchar(tx_rb)
       │    WRITE_REG(DR, byte)
       │
       ├─ TXE set — TX ring buffer empty
       │    CLEAR_BIT(CR1, TXEIE)
       │    drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(0), NULL, &hpt)
       │
       └─ TC set
            CLEAR_BIT(CR1, TCIE)
            drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(0), NULL, &hpt)
       │
       ▼
  portYIELD_FROM_ISR(hpt)
```

**RX notification:** `_uart_rx_cb` writes the byte directly into the IPC ring
buffer. Tasks drain it with `uart_mgmt_read_byte()` (blocking) or
`uart_mgmt_read_line()`.  No task wake-up occurs per byte; the ring buffer
decouples ISR rate from task scheduling.

---

## I2C ISR Chain

I2C uses two NVIC lines (EV + ER). The HAL state machine is kept for the
complex I2C protocol handling; completion is detected by comparing
`HAL_I2C_GetState()` before and after the HAL handler call, so no HAL weak
callbacks are needed.

```
CPU vector table
  │
  ├─ I2C1_EV_IRQHandler()
  │    └─ hal_iic_stm32_ev_irq_handler(I2C1)   hal_iic_stm32.c
  │         prev = HAL_I2C_GetState()
  │         HAL_I2C_EV_IRQHandler(&hi2c)        HAL state machine step
  │         next = HAL_I2C_GetState()
  │         │
  │         └─ if next == READY && prev != READY  (transfer complete)
  │              ├─ prev == BUSY_TX  →  drv_irq_dispatch_from_isr(IRQ_ID_I2C_TX_DONE(0), ...)
  │              │    └─► _iic_done_cb()          iic_mgmt.c
  │              │              vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
  │              └─ prev == BUSY_RX  →  drv_irq_dispatch_from_isr(IRQ_ID_I2C_RX_DONE(0), ...)
  │                   └─► _iic_done_cb()          iic_mgmt.c
  │                             vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
  │         portYIELD_FROM_ISR(hpt)
  │
  └─ I2C1_ER_IRQHandler()
       └─ hal_iic_stm32_er_irq_handler(I2C1)   hal_iic_stm32.c
            HAL_I2C_ER_IRQHandler(&hi2c)        resets state to READY
            err = HAL_I2C_GetError()
            drv_irq_dispatch_from_isr(IRQ_ID_I2C_ERROR(0), &err, &hpt)
              └─► _iic_err_cb()                 iic_mgmt.c
                        vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
            portYIELD_FROM_ISR(hpt)
```

`BUSY_TX` covers both `HAL_I2C_Master_Transmit_IT` and `HAL_I2C_Mem_Write_IT`
(both set that state); `BUSY_RX` covers both receive variants.

---

## SPI ISR Chain

SPI has a single NVIC line. The HAL state machine is kept for multi-interrupt
operations; completion/error is detected by checking state and error code after
the HAL handler returns.

```
CPU vector table
  │
  ▼
SPI1_IRQHandler()
  └─ hal_spi_stm32_irq_handler(SPI1)          hal_spi_stm32.c
       prev = HAL_SPI_GetState()
       HAL_SPI_IRQHandler(&hspi)               HAL state machine step
       │
       ├─ if GetState() != READY → return      multi-interrupt op, not done yet
       │
       └─ transfer complete:
            err = HAL_SPI_GetError()
            │
            ├─ err != NONE
            │    drv_irq_dispatch_from_isr(IRQ_ID_SPI_ERROR(0), &err, &hpt)
            │      └─► _spi_err_cb()           spi_mgmt.c
            │                vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
            │
            └─ err == NONE
                 ├─ prev == BUSY_TX    →  IRQ_ID_SPI_TX_DONE(0)
                 ├─ prev == BUSY_RX    →  IRQ_ID_SPI_RX_DONE(0)
                 └─ prev == BUSY_TX_RX →  IRQ_ID_SPI_TXRX_DONE(0)
                      └─► _spi_done_cb()       spi_mgmt.c
                                vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
       portYIELD_FROM_ISR(hpt)
```

---

## Task Notification Mechanism

Two distinct notification patterns are used depending on the peripheral:

### UART RX — ring buffer (decoupled)

```
ISR context                         Task context
───────────                         ────────────
_uart_rx_cb()                       uart_mgmt_read_byte() / read_line()
  ringbuffer_putchar(rx_rb, byte)     ringbuffer_getchar(rx_rb, &byte)
                                      (blocks on semaphore if empty)
```

The ring buffer decouples ISR throughput from task scheduling. The producer
(ISR) never blocks; the consumer (task) sleeps until data arrives.

### I2C / SPI — direct task notification (coupled)

```
Management thread (task context)         ISR context
────────────────────────────────         ───────────
h->notify_task = xTaskGetCurrentTaskHandle()
ops->transfer_it(...)          ────────► _spi_done_cb() / _iic_done_cb()
ulTaskNotifyTake(pdTRUE, tmo)  ◄────────   vTaskNotifyGiveFromISR(h->notify_task, pxHPT)
result = h->last_err                     portYIELD_FROM_ISR(hpt)
h->notify_task = NULL
```

The management thread suspends on `ulTaskNotifyTake` after kicking the IT
transfer. The ISR's subscriber callback wakes exactly that thread via
`vTaskNotifyGiveFromISR`. One transfer completes before the next begins.

### Subscribers registered at startup

| IRQ ID | Subscriber | Registered in |
|--------|-----------|---------------|
| `IRQ_ID_UART_RX(n)` | `_uart_rx_cb` → `ringbuffer_putchar` | `uart_mgmt.c` |
| `IRQ_ID_UART_ERROR(n)` | `_uart_err_cb` | `uart_mgmt.c` |
| `IRQ_ID_I2C_TX_DONE(n)` | `_iic_done_cb` → `vTaskNotifyGiveFromISR` | `iic_mgmt.c` |
| `IRQ_ID_I2C_RX_DONE(n)` | `_iic_done_cb` → `vTaskNotifyGiveFromISR` | `iic_mgmt.c` |
| `IRQ_ID_I2C_ERROR(n)` | `_iic_err_cb`  → `vTaskNotifyGiveFromISR` | `iic_mgmt.c` |
| `IRQ_ID_SPI_TX_DONE(n)` | `_spi_done_cb` → `vTaskNotifyGiveFromISR` | `spi_mgmt.c` |
| `IRQ_ID_SPI_RX_DONE(n)` | `_spi_done_cb` → `vTaskNotifyGiveFromISR` | `spi_mgmt.c` |
| `IRQ_ID_SPI_TXRX_DONE(n)` | `_spi_done_cb` → `vTaskNotifyGiveFromISR` | `spi_mgmt.c` |
| `IRQ_ID_SPI_ERROR(n)` | `_spi_err_cb`  → `vTaskNotifyGiveFromISR` | `spi_mgmt.c` |

---

## IRQ ID Space

Software IRQ IDs are consecutive integers defined in `include/irq/irq_notify.h`:

```
ID   Macro                    Description
─────────────────────────────────────────────────────
 0   IRQ_ID_UART_RX(0)        UART bus 0 — byte received
 1   IRQ_ID_UART_TX_DONE(0)   UART bus 0 — transmit complete
 2   IRQ_ID_UART_ERROR(0)     UART bus 0 — error
 3   IRQ_ID_UART_RX(1)        UART bus 1 — …
 …   (4 buses × 3 events = 12 UART IDs)

12   IRQ_ID_I2C_TX_DONE(0)    I2C bus 0 — master TX complete
13   IRQ_ID_I2C_RX_DONE(0)    I2C bus 0 — master RX complete
14   IRQ_ID_I2C_ERROR(0)      I2C bus 0 — error
 …   (4 buses × 3 events = 12 I2C IDs)

24   IRQ_ID_SPI_TX_DONE(0)    SPI bus 0 — TX complete
25   IRQ_ID_SPI_RX_DONE(0)    SPI bus 0 — RX complete
26   IRQ_ID_SPI_TXRX_DONE(0)  SPI bus 0 — full-duplex complete
27   IRQ_ID_SPI_ERROR(0)      SPI bus 0 — error
 …   (4 buses × 4 events = 16 SPI IDs)

40   IRQ_ID_EXTI(0)           GPIO EXTI line 0 (PA0 / BTN_USER)
41   IRQ_ID_EXTI(1)           GPIO EXTI line 1
 …   (16 EXTI lines)

56   IRQ_ID_TOTAL
```

**All IDs are compile-time constants.** No dynamic allocation.  
`irq_to_desc(id)` is an O(1) array dereference into `_irq_desc_table[IRQ_ID_TOTAL]`.

---

## Key Data Structures

### `struct irq_desc`  (`include/irq/irq_desc.h`)

Per-IRQ descriptor. One instance per software IRQ ID in a static table.

```c
struct irq_desc {
    struct irq_common_data  irq_common_data;  // state flags
    struct irq_data         irq_data;         // chip + hwirq binding

    irq_flow_handler_t      handle_irq;       // flow handler (simple/edge/level)
    struct irqaction       *action;           // head of driver ISR chain

    unsigned int            depth;            // nested-disable depth
    unsigned int            tot_count;        // total fires ever
    unsigned int            irq_count;        // fires in current window
    unsigned int            irqs_unhandled;   // fires with IRQ_NONE returned
    const char             *name;             // from irq_table.c (generated)
};
```

### `struct irqaction`  (`include/irq/irq_desc.h`)

One node in the per-IRQ handler chain. Allocated from a static pool.

```c
struct irqaction {
    irq_handler_t      handler;   // irqreturn_t (*)(irq, data, dev_id, pxHPT)
    void              *dev_id;    // caller context passed back to handler
    const char        *name;      // for debugging
    struct irqaction  *next;      // next in chain
};
```

### `struct irq_chip`  (`include/irq/irq_desc.h`)

Interrupt controller operations. The NVIC chip is in
`drivers/hal/stm32/irq_chip_nvic.c`.

```c
struct irq_chip {
    const char *name;                                        // "NVIC"
    void (*irq_enable)(struct irq_data *);                   // HAL_NVIC_EnableIRQ
    void (*irq_disable)(struct irq_data *);                  // HAL_NVIC_DisableIRQ
    void (*irq_mask)(struct irq_data *);                     // same as disable on CM4
    void (*irq_unmask)(struct irq_data *);                   // same as enable on CM4
    void (*irq_ack)(struct irq_data *);                      // no-op (periph clears SR)
    void (*irq_set_affinity)(struct irq_data *, uint32_t);   // HAL_NVIC_SetPriority
    // …
};
```

---

## Flow Handlers

Three flow handlers are provided in `irq/irq_desc.c`:

| Handler | Chip ops called | When to use |
|---------|----------------|-------------|
| `handle_simple_irq` | none | Software IRQ IDs (UART, I2C, SPI, EXTI dispatched by HAL) |
| `handle_edge_irq` | `irq_ack` | GPIO EXTI if going through chip — clears pending before walking chain |
| `handle_level_irq` | `mask_ack` → chain → `unmask` | Level-sensitive lines that must be re-enabled after service |

Default at `irq_desc_init_all()` is `handle_simple_irq` for all IDs.

---

## Registration APIs

### `request_irq()` — direct irqaction registration

```c
int request_irq(irq_id_t irq, irq_handler_t handler,
                const char *name, void *dev_id);
```

- Allocates an `irqaction` from the static pool.
- Appends it to the tail of `desc->action` (preserves registration order).
- Returns `OS_ERR_NONE` on success, `OS_ERR_MEM_OF` if the pool is exhausted.
- Thread-safe to call before the scheduler starts; **not** ISR-safe.

```c
// Example — register a GPIO EXTI handler:
static irqreturn_t my_handler(irq_id_t irq, void *data,
                               void *dev_id, BaseType_t *pxHPT)
{
    vTaskNotifyGiveFromISR((TaskHandle_t)dev_id, pxHPT);
    return IRQ_HANDLED;
}

request_irq(IRQ_ID_EXTI(0), my_handler, "my_btn", xTaskGetCurrentTaskHandle());
```

### `free_irq()` — remove a handler

```c
void free_irq(irq_id_t irq, void *dev_id);
```

Walks the chain and unlinks the `irqaction` whose `dev_id` matches.

### `irq_register()` — notify-style wrapper

```c
int32_t irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg);
```

Allocates a trampoline (`_notify_trampoline_t`) that adapts the void-return
`irq_notify_cb_t` to `irq_handler_t`, then calls `request_irq()`.  
For new code, prefer `request_irq()` directly.

---

## Boot Sequence

```
main()
  └─ HAL_Init()
  └─ drv_rcc_clock_init()
  └─ board_clk_enable()          enables all RCC clocks (sys bus + peripherals + GPIO)
  └─ irq_hw_init_all()           generated from app/board/irq_table.xml
       ├─ irq_desc_init_all()    zero-fills table, sets handle_simple_irq + names
       ├─ irq_set_chip_and_handler(IRQ_ID_UART_RX(0), irq_chip_nvic_get(), handle_simple_irq)
       ├─ irq_chip_nvic_bind_hwirq(IRQ_ID_UART_RX(0), USART1_IRQn, 2)
       │   └─ HAL_NVIC_SetPriority(USART1_IRQn, 2, 0)
       ├─ … (all peripherals from irq_table.xml)
       └─ HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0)
  └─ xTaskCreate(uart_mgmt_thread, ...)   registers _uart_rx_cb / _uart_err_cb
  └─ xTaskCreate(iic_mgmt_thread,  ...)   registers _iic_done_cb / _iic_err_cb
  └─ xTaskCreate(spi_mgmt_thread,  ...)   registers _spi_done_cb / _spi_err_cb
  └─ vTaskStartScheduler()
```

**`irq_hw_init_all()` must be called after `board_clk_enable()` and before any `request_irq()`.**

---

## EXTI Example

`app/app_main.c` demonstrates a GPIO EXTI interrupt (BTN_USER, PA0):

```
EXTI0_IRQHandler()                   stm32_exceptions.c
  _EXTI_DISPATCH(GPIO_PIN_0, 0)
    __HAL_GPIO_EXTI_CLEAR_IT()
    drv_irq_dispatch_from_isr(IRQ_ID_EXTI(0), &pin, &hpt)
      __do_IRQ_from_isr()
        handle_simple_irq()
          handle_irq_event()
            _btn_irq_handler()       app_main.c
              vTaskNotifyGiveFromISR(btn_task_handle, pxHPT)
  portYIELD_FROM_ISR(hpt)

btn_task wakes:
  gpio_mgmt_post(LED_STATUS, GPIO_MGMT_CMD_TOGGLE, ...)
  uart_mgmt_async_transmit(UART_DEBUG, "[BTN] pressed\r\n", ...)
```

Key points:
- `_btn_irq_handler` uses the native `irq_handler_t` signature — no trampoline overhead.
- `dev_id` carries the task handle; no global variable needed.
- The NVIC line (`EXTI0_IRQn`) and its priority (`6`) were bound at boot by `irq_hw_init_all()`.

---

## UART Echo Example

`echo_task` in `app/app_main.c` uses the `irq_register()` compatibility path:

```c
irq_register(IRQ_ID_UART_RX(UART_DEBUG), _echo_uart_rx_cb, NULL);
```

`_echo_uart_rx_cb` is an `irq_notify_cb_t` (void return). `irq_register()` wraps
it in a trampoline and appends it to the same descriptor chain as
`_uart_rx_cb` (uart_mgmt). Both subscribers fire on every RXNE interrupt:
`_uart_rx_cb` writes into the ring buffer; `_echo_uart_rx_cb` sends the echo.

---

## IRQ Table Generator

`app/board/irq_table.xml` is the single source of truth for IRQ names and NVIC priorities.

```bash
make irq_gen    # regenerates:
                #   FreeRTOS-OS/irq/irq_table.c           — IRQ name table
                #   app/board/irq_hw_init_generated.c/.h  — irq_hw_init_all()
```

XML schema at a glance:

```xml
<irq_table board="stm32f411_devboard">
  <uart dev_id="0" label="UART_DEBUG" irqn="USART1_IRQn" priority="2"/>
  <i2c  dev_id="0" label="I2C_SENSOR"
        irqn_ev="I2C1_EV_IRQn" priority_ev="3"
        irqn_er="I2C1_ER_IRQn" priority_er="3"/>
  <spi  dev_id="0" label="SPI_FLASH"  irqn="SPI1_IRQn"  priority="3"/>
  <exti line="0"   label="BTN_USER"   irqn="EXTI0_IRQn" priority="6"/>
  <sys  name="SysTick" irqn="SysTick_IRQn" priority="15" action="set_priority"/>
</irq_table>
```

---

## File Map

| File | Role |
|------|------|
| `include/irq/irq_notify.h` | IRQ ID macros, `IRQ_ID_TOTAL`, `irq_notify_cb_t` |
| `include/irq/irq_desc.h` | `struct irq_desc/chip/data/irqaction`, full API |
| `include/irq/irq_table.h` | `irq_table_get_name()` declaration |
| `irq/irq_desc.c` | Core implementation: descriptor table, pool, flow handlers, dispatch |
| `irq/irq_notify.c` | Adapter: `irq_register` → trampoline → `request_irq` |
| `irq/irq_table.c` | **Generated** — static name table indexed by `irq_id_t` |
| `include/drivers/drv_irq.h` | Public driver API: `drv_irq_dispatch_from_isr`, `drv_irq_register` |
| `drivers/drv_irq.c` | Thin wrappers: `drv_irq_dispatch_from_isr` → `__do_IRQ_from_isr` |
| `include/drivers/hal/stm32/irq_chip_nvic.h` | NVIC chip declarations |
| `drivers/hal/stm32/irq_chip_nvic.c` | NVIC `irq_chip` ops (`HAL_NVIC_*`) |
| `drivers/hal/stm32/hal_uart_stm32.c` | UART ISR — direct SR/DR register reads, no HAL state machine |
| `drivers/hal/stm32/hal_iic_stm32.c` | I2C EV/ER ISRs — HAL state machine + pre/post state compare |
| `drivers/hal/stm32/hal_spi_stm32.c` | SPI ISR — HAL state machine + pre/post state compare |
| `drivers/hal/stm32/stm32_exceptions.c` | Hardware vector bodies — route to peripheral handlers |
| `services/uart_mgmt.c` | Registers `_uart_rx_cb` / `_uart_err_cb`; ring-buffer consumer |
| `services/iic_mgmt.c` | Registers `_iic_done_cb` / `_iic_err_cb`; `ulTaskNotifyTake` waiter |
| `services/spi_mgmt.c` | Registers `_spi_done_cb` / `_spi_err_cb`; `ulTaskNotifyTake` waiter |
| `app/board/irq_table.xml` | **Source of truth** — peripheral labels, NVIC priorities |
| `scripts/gen_irq_table.py` | Generator: XML → `irq_table.c` + `irq_hw_init_generated.c` |
| `app/board/irq_hw_init_generated.c` | **Generated** — `irq_hw_init_all()` |

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
