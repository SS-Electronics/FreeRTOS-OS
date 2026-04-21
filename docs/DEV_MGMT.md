# Driver Management Services — FreeRTOS-OS

This document describes in detail how the four peripheral management service threads work: their startup sequence, internal state, message dispatch, synchronisation patterns, error recovery, and public APIs.

For the driver layer these threads sit above, see [DRIVERS.md](DRIVERS.md).
For the board configuration system that drives peripheral registration, see [BOARD.md](BOARD.md).

---

## Table of Contents

- [Design Principles](#design-principles)
- [Overall Architecture](#overall-architecture)
- [Startup Sequence](#startup-sequence)
- [UART Management — `uart_mgmt`](#uart-management--uart_mgmt)
  - [State](#uart-state)
  - [Thread Body](#uart-thread-body)
  - [Message Protocol](#uart-message-protocol)
  - [Async Transmit Flow](#async-transmit-flow)
  - [Error Recovery](#uart-error-recovery)
  - [Public API](#uart-public-api)
  - [Configuration Knobs](#uart-configuration-knobs)
- [I2C Management — `iic_mgmt`](#i2c-management--iic_mgmt)
  - [State](#i2c-state)
  - [Thread Body](#i2c-thread-body)
  - [Message Protocol](#i2c-message-protocol)
  - [Async Write Flow](#async-write-flow)
  - [Sync Read Flow — Task-Notification Pattern](#sync-read-flow--task-notification-pattern)
  - [Device Probe Flow](#device-probe-flow)
  - [Public API](#i2c-public-api)
  - [Configuration Knobs](#i2c-configuration-knobs)
- [SPI Management — `spi_mgmt`](#spi-management--spi_mgmt)
  - [State](#spi-state)
  - [Thread Body](#spi-thread-body)
  - [Message Protocol](#spi-message-protocol)
  - [Sync Full-Duplex Transfer Flow](#sync-full-duplex-transfer-flow)
  - [CS Pin Responsibility](#cs-pin-responsibility)
  - [Public API](#spi-public-api)
  - [Configuration Knobs](#spi-configuration-knobs)
- [GPIO Management — `gpio_mgmt`](#gpio-management--gpio_mgmt)
  - [State](#gpio-state)
  - [Thread Body](#gpio-thread-body)
  - [GPIO Registration Sequence](#gpio-registration-sequence)
  - [Message Protocol](#gpio-message-protocol)
  - [Delayed Command Flow](#delayed-command-flow)
  - [Public API](#gpio-public-api)
  - [Configuration Knobs](#gpio-configuration-knobs)
- [OS Shell Management — `os_shell_mgmt`](#os-shell-management--os_shell_mgmt)
- [Application Driver Layer — `drv_app`](#application-driver-layer--drv_app)
- [Compile-Time Activation](#compile-time-activation)
- [Thread Timing Summary](#thread-timing-summary)
- [Adding a New Management Thread](#adding-a-new-management-thread)

---

## Design Principles

**1. One thread owns one bus type.**
Each management thread holds exclusive ownership of its driver handles. No application task ever calls `drv_uart_transmit()` directly — all hardware access goes through the management queue. This eliminates the need for per-handle mutexes and makes bus-level serialisation implicit.

**2. Self-registration from board configuration.**
At startup, each thread reads `board_get_config()` and iterates the board peripheral table. It calls `hal_xxx_set_config()` (stores parameters in the handle's `hw` context) then `drv_xxx_register()` (calls `hw_init()`). No application code is needed to bring up peripherals.

**3. Vendor selection at compile time, not runtime.**
`CONFIG_DEVICE_VARIANT` (set in `app/board/mcu_config.h`) selects which HAL backend's ops table is bound. The management threads contain `#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)` guards — unused vendor branches are dead-code eliminated.

**4. Callers never block on the bus.**
- **Async** path: post a message and return immediately (`xQueueSend` with timeout 0).
- **Sync** path (I2C read, SPI transfer): post a message that carries `result_notify = xTaskGetCurrentTaskHandle()`, then call `ulTaskNotifyTake()`. The management thread calls `xTaskNotifyGive()` when done, waking the caller.

**5. Use `drv_app/` as the recommended application interface.**
The `drv_app` layer (`include/drv_app/`) wraps the management service APIs into simple `uart_sync_transmit()` / `spi_async_transfer()` style calls. Application tasks should include `<drv_app/uart_app.h>` etc. rather than calling `uart_mgmt_*` directly. This keeps application code independent of FreeRTOS queue internals.

**5. Startup delays prevent race conditions.**
Each management thread begins with `os_thread_delay(TIME_OFFSET_xxx)` so that the OS scheduler, IPC queues, and clocks are fully running before any peripheral hardware initialisation occurs. These delays are staggered so threads initialise sequentially.

---

## Overall Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                   Application Tasks                            │
│   uart_sync_transmit()  iic_sync_receive()  spi_sync_transfer()│
│   gpio_async_toggle()   gpio_read()                            │
└──────┬───────────────┬──────────────┬───────────────┬──────────┘
       │               │              │               │
       ▼               ▼              ▼               ▼
┌─────────────────────────────────────────────────────────────────┐
│               drv_app layer  (include/drv_app/)                │
│   uart_app.c    iic_app.c    spi_app.c    gpio_app.c           │
└──────┬───────────────┬──────────────┬───────────────┬──────────┘
       │ xQueueSend    │              │               │
       ▼               ▼              ▼               ▼
  ┌─────────┐    ┌─────────┐   ┌─────────┐   ┌─────────┐
  │  uart_  │    │  iic_   │   │  spi_   │   │  gpio_  │
  │  mgmt   │    │  mgmt   │   │  mgmt   │   │  mgmt   │
  │ thread  │    │ thread  │   │ thread  │   │ thread  │
  └────┬────┘    └────┬────┘   └────┬────┘   └────┬────┘
       │              │              │               │
       │  ops->xxx()  │              │               │
       ▼              ▼              ▼               ▼
  drv_uart.c     drv_iic.c    drv_spi.c      drv_gpio.c
  (generic)      (generic)    (generic)      (generic)
       │              │              │               │
       ▼              ▼              ▼               ▼
  hal_uart_      hal_iic_     hal_spi_       hal_gpio_
  stm32.c        stm32.c      stm32.c        stm32.c
  (STM32 HAL)
```

Each management thread has:
- A **private static `QueueHandle_t`** — the only way in from the outside world
- A **thread function** with startup registration block + infinite message loop
- A **`_start()` function** that creates the queue and spawns the thread
- **Public helper functions** that package messages and call `xQueueSend`

---

## Startup Sequence

```
main() / app_main()
  ├─ uart_mgmt_start()    → xQueueCreate + os_thread_create("uart_mgmt")
  ├─ iic_mgmt_start()     → xQueueCreate + os_thread_create("iic_mgmt")
  ├─ spi_mgmt_start()     → xQueueCreate + os_thread_create("spi_mgmt")
  ├─ gpio_mgmt_start()    → xQueueCreate + os_thread_create("gpio_mgmt")
  └─ vTaskStartScheduler()

--- scheduler running ---

  t=0       gpio_mgmt  wakes (TIME_OFFSET_GPIO_MANAGEMENT = 3000 ms)
              └─ board_get_config() → iterates gpio_table
                   for each GPIO:
                     board_gpio_clk_enable(port)
                     hal_gpio_stm32_set_config(h, ...)   ← store only
                     drv_gpio_register(id, ops)           ← hw_init runs here
                     if initial_state: ops->set(h)
              └─ enters message loop

  t=1000 ms uart_mgmt  wakes (TIME_OFFSET_SERIAL_MANAGEMENT = 4000 ms)
              └─ board_get_config() → iterates uart_table
                   for each UART:
                     hal_uart_stm32_set_config(h, ...)   ← store only
                     drv_uart_register(id, ops, baud, 10) ← hw_init + start_rx_it
              └─ enters message loop

  t=1500 ms spi_mgmt   wakes (TIME_OFFSET_SPI_MANAGEMENT = 5500 ms)
              └─ board_get_config() → iterates spi_table
                   for each SPI:
                     hal_spi_stm32_set_config(h, ...)    ← store only
                     drv_spi_register(id, ops, 0, 10)    ← hw_init runs here
              └─ enters message loop

  t=3500 ms iic_mgmt   wakes (TIME_OFFSET_IIC_MANAGEMENT = 6500 ms)
              └─ board_get_config() → iterates iic_table
                   for each I2C:
                     hal_iic_stm32_set_config(h, ...)    ← store only
                     drv_iic_register(id, ops, hz, 100)  ← hw_init runs here
              └─ enters message loop
```

> Offsets are measured from `vTaskStartScheduler()`. GPIO starts first at 3 000 ms because GPIO clocks must be enabled before any UART/SPI/I2C HAL MSP init functions run (they call `HAL_GPIO_Init` internally). The `set_config` → `register` → `hw_init` ordering within each thread ensures the vendor hardware context is fully populated before `hw_init` reads it.

---

## UART Management — `uart_mgmt`

**Files:**
- [services/uart_mgmt.c](services/uart_mgmt.c)
- [include/services/uart_mgmt.h](include/services/uart_mgmt.h)

**Compile guard:** `INC_SERVICE_UART_MGMT == 1` (set in `config/conf_os.h` via `make menuconfig`)

### UART State

```c
/* uart_mgmt.c — translation unit private */
static QueueHandle_t _mgmt_queue = NULL;
```

The single private queue is the only way into the thread. The driver handles themselves live in `drv_uart.c` (static array indexed by `dev_id`).

### UART Thread Body

```
uart_mgmt_thread()
│
├─ os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT)     // 4000 ms
│
├─ ops = hal_uart_stm32_get_ops()     // or infineon variant
│
├─ bc = board_get_config()
│   for i in 0 .. bc->uart_count - 1:
│       d = &bc->uart_table[i]
│       h = drv_uart_get_handle(d->dev_id)
│       hal_uart_stm32_set_config(h,              // populate hw context
│           d->instance, d->word_len,
│           d->stop_bits, d->parity, d->mode)
│       drv_uart_register(d->dev_id, ops,         // binds ops, calls hw_init()
│           d->baudrate, timeout=10)              //  → HAL_UART_Init()
│                                                 //  → start_rx_it() → ISR armed
│
└─ for (;;):
       msg ← xQueueReceive(_mgmt_queue, portMAX_DELAY)
       h   = drv_uart_get_handle(msg.dev_id)

       switch msg.cmd:

         UART_MGMT_CMD_TRANSMIT:
           err = h->ops->transmit(h, msg.data, msg.len, h->timeout_ms)
           if err != OS_ERR_NONE:
             h->ops->hw_deinit(h)      // recovery reinit
             h->ops->hw_init(h)

         UART_MGMT_CMD_REINIT:
           h->ops->hw_deinit(h)
           h->ops->hw_init(h)

         UART_MGMT_CMD_DEINIT:
           h->ops->hw_deinit(h)

       if msg.result_code != NULL:
           *msg.result_code = err           // write result back to caller
       if msg.result_notify != NULL:
           xTaskNotifyGive(msg.result_notify)  // wake sync caller
```

### UART Message Protocol

```c
typedef enum {
    UART_MGMT_CMD_TRANSMIT = 0,  // post data for TX
    UART_MGMT_CMD_REINIT,        // force deinit + init
    UART_MGMT_CMD_DEINIT,        // shut down a UART
} uart_mgmt_cmd_t;

typedef struct {
    uart_mgmt_cmd_t  cmd;
    uint8_t          dev_id;        // which UART (matches board_device_ids.h)
    const uint8_t   *data;          // TX buffer pointer — must remain valid!
    uint16_t         len;           // number of bytes to transmit
    TaskHandle_t     result_notify; // non-NULL → blocking (sync) transmit
    int32_t         *result_code;   // non-NULL → write result here
} uart_mgmt_msg_t;
```

**Buffer lifetime:** The caller must keep `data` valid until the management thread drains the queue item and completes `ops->transmit()`. Use a static buffer, a heap buffer that is freed by the callback, or a buffer copied before returning (the transmit is synchronous inside the management thread).

### Async Transmit Flow

```
Application task
    │
    │  uart_mgmt_async_transmit(UART_DEBUG, buf, len)
    │    └─ xQueueSend(_mgmt_queue, &msg, timeout=0)
    │         returns OS_ERR_OP if queue full (16 items deep)
    │
    │  returns immediately — caller not blocked
    │
    ▼
uart_mgmt thread (running independently)
    │
    │  xQueueReceive → msg (TRANSMIT)
    │  h->ops->transmit(h, msg.data, msg.len, timeout)
    │    └─ hal_uart_stm32.c → HAL_UART_Transmit()  (blocking within mgmt thread)
    │
    ▼  HAL_UART_TxCpltCallback()
         └─ board_get_uart_cbs(dev_id)->on_tx_done(dev_id)   // app callback if registered
```

### UART Error Recovery

On `transmit` failure the management thread attempts one automatic recovery cycle:

```c
err = h->ops->transmit(h, msg.data, msg.len, h->timeout_ms);
if (err != OS_ERR_NONE) {
    h->ops->hw_deinit(h);   // HAL_UART_DeInit()
    h->ops->hw_init(h);     // HAL_UART_Init() + start_rx_it()
}
```

For explicit recovery from application code, post `UART_MGMT_CMD_REINIT`.

### UART Public API

```c
// Start the thread — call before vTaskStartScheduler()
int32_t uart_mgmt_start(void);

// Get the raw queue handle (for manual message posting)
QueueHandle_t uart_mgmt_get_queue(void);

// Non-blocking transmit — returns OS_ERR_OP if queue full
int32_t uart_mgmt_async_transmit(uint8_t dev_id,
                                  const uint8_t *data, uint16_t len);

// Non-blocking RX byte — reads one byte from the ISR ring buffer.
// Returns OS_ERR_NONE if a byte was available, OS_ERR_OP if empty.
int32_t uart_mgmt_read_byte(uint8_t dev_id, uint8_t *byte);
```

**RX path** — receive is not handled through the queue at all. The UART ISR dispatches directly:

```
HAL_UART_RxCpltCallback(huart)
  └─ drv_uart_rx_isr_dispatch(dev_id, byte)   [in drv_uart.c]
       ├─ ringbuffer_putchar(rb, byte)          // IPC ring buffer for os_read()
       └─ board_get_uart_cbs(dev_id)->on_rx_byte(dev_id, byte)   // app callback
```

### UART Configuration Knobs

All in `config/conf_os.h` (editable via `make menuconfig`):

| Macro | Default | Effect |
|-------|---------|--------|
| `INC_SERVICE_UART_MGMT` | `1` | Compile and start the thread |
| `PROC_SERVICE_SERIAL_MGMT_STACK_SIZE` | `512` words | FreeRTOS task stack |
| `PROC_SERVICE_SERIAL_MGMT_PRIORITY` | `1` | FreeRTOS task priority |
| `TIME_OFFSET_SERIAL_MANAGEMENT` | `4000` ms | Startup delay before hw init |
| `UART_MGMT_QUEUE_DEPTH` | `16` items | Max pending TX requests |

---

## I2C Management — `iic_mgmt`

**Files:**
- [services/iic_mgmt.c](services/iic_mgmt.c)
- [include/services/iic_mgmt.h](include/services/iic_mgmt.h)

**Compile guard:** `INC_SERVICE_IIC_MGMT == 1`

### I2C State

```c
static QueueHandle_t _mgmt_queue = NULL;
```

### I2C Thread Body

```
iic_mgmt_thread()
│
├─ os_thread_delay(TIME_OFFSET_IIC_MANAGEMENT)        // 6500 ms
│
├─ ops = hal_iic_stm32_get_ops()
│
├─ bc = board_get_config()
│   for i in 0 .. bc->iic_count - 1:
│       d = &bc->iic_table[i]
│       h = drv_iic_get_handle(d->dev_id)
│       hal_iic_stm32_set_config(h,
│           d->instance, d->addr_mode, d->dual_addr)
│       drv_iic_register(d->dev_id, ops,
│           d->clock_hz, IIC_ACK_TIMEOUT_MS=100)    // hw_init → HAL_I2C_Init()
│
└─ for (;;):
       msg ← xQueueReceive(_mgmt_queue, portMAX_DELAY)
       h   = drv_iic_get_handle(msg.bus_id)
       result = OS_ERR_OP

       switch msg.cmd:

         IIC_MGMT_CMD_TRANSMIT:
           result = h->ops->transmit(h,
               msg.dev_addr, msg.reg_addr, msg.use_reg,
               msg.data, msg.len, h->timeout_ms)

         IIC_MGMT_CMD_RECEIVE:
           result = h->ops->receive(h,
               msg.dev_addr, msg.reg_addr, msg.use_reg,
               msg.data, msg.len, h->timeout_ms)

         IIC_MGMT_CMD_PROBE:
           result = h->ops->is_device_ready(h, msg.dev_addr, h->timeout_ms)

         IIC_MGMT_CMD_REINIT:
           h->ops->hw_deinit(h)
           result = h->ops->hw_init(h)

       if msg.result_code != NULL:
           *msg.result_code = result           // write result back to caller

notify:
       if msg.result_notify != NULL:
           xTaskNotifyGive(msg.result_notify)  // wake waiting caller
```

### I2C Message Protocol

```c
typedef enum {
    IIC_MGMT_CMD_TRANSMIT = 0,  // write bytes to a device register
    IIC_MGMT_CMD_RECEIVE,       // read bytes from a device register
    IIC_MGMT_CMD_PROBE,         // check if a device ACKs its address
    IIC_MGMT_CMD_REINIT,        // reinitialise the bus
} iic_mgmt_cmd_t;

typedef struct {
    iic_mgmt_cmd_t   cmd;
    uint8_t          bus_id;        // I2C bus index
    uint16_t         dev_addr;      // 7-bit I2C device address (not shifted)
    uint8_t          reg_addr;      // register / sub-address
    uint8_t          use_reg;       // 1 = send reg_addr prefix, 0 = raw write
    uint8_t         *data;          // TX data (TRANSMIT) / RX buffer (RECEIVE)
    uint16_t         len;           // byte count
    TaskHandle_t     result_notify; // task to notify on completion (NULL = fire-forget)
    int32_t         *result_code;   // where to store the return code (NULL = don't care)
} iic_mgmt_msg_t;
```

**`use_reg`:** When `1`, the management thread prefixes the transfer with the 8-bit `reg_addr` byte (memory-write or memory-read pattern). When `0`, a raw I2C write is performed with the data buffer only.

### Async Write Flow

```
Task A
  │  iic_mgmt_async_transmit(I2C_SENSOR_BUS, 0x40, 0x01, 1, buf, 2)
  │    msg.result_notify = NULL   ← no notification
  │    msg.result_code   = NULL   ← don't care about result
  │    xQueueSend(_mgmt_queue, &msg, timeout=0)
  │
  └─ returns immediately

iic_mgmt thread
  │  xQueueReceive → msg (TRANSMIT)
  │  h->ops->transmit(…) → HAL_I2C_Mem_Write()
  │  result_code = NULL  → skip write
  └─ result_notify = NULL → skip notify
```

### Sync Read Flow — Task-Notification Pattern

```
Task A (sensor_task)
  │
  │  int32_t result = OS_ERR_OP
  │  iic_mgmt_sync_receive(I2C_SENSOR_BUS, 0x40, 0x02, 1, buf, 2, 10)
  │    ├─ msg.result_notify = xTaskGetCurrentTaskHandle()   ← this task
  │    ├─ msg.result_code   = &result                       ← local variable
  │    ├─ xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(10))
  │    └─ ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10))       ← BLOCKS HERE
  │
  ▼ (task suspended by FreeRTOS scheduler)

iic_mgmt thread
  │  xQueueReceive → msg (RECEIVE)
  │  h->ops->receive(…) → HAL_I2C_Mem_Read() → fills buf[]
  │  *msg.result_code = result                ← writes result to Task A's stack
  └─ xTaskNotifyGive(msg.result_notify)       ← unblocks Task A

Task A resumes
  │  ulTaskNotifyTake() returns
  │  result == OS_ERR_NONE → process buf[]
  └─ …
```

**Key point:** `result` lives on Task A's stack. The management thread writes to it through the pointer stored in the message. This is safe because Task A is blocked for the duration — it cannot touch `result` or `buf` until the notify arrives.

### Device Probe Flow

```
iic_mgmt_msg_t msg = {
    .cmd           = IIC_MGMT_CMD_PROBE,
    .bus_id        = I2C_SENSOR_BUS,
    .dev_addr      = 0x40,
    .result_notify = xTaskGetCurrentTaskHandle(),
    .result_code   = &result,
};
xQueueSend(iic_mgmt_get_queue(), &msg, pdMS_TO_TICKS(10));
ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
// result == OS_ERR_NONE → device present
```

### I2C Public API

```c
int32_t       iic_mgmt_start(void);
QueueHandle_t iic_mgmt_get_queue(void);

// Non-blocking write (fire-and-forget)
int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len);

// Non-blocking read — result ignored; use sync variant if result needed
int32_t iic_mgmt_async_receive(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                uint8_t *data, uint16_t len);

// Blocking write — suspends caller until management thread completes
int32_t iic_mgmt_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);

// Blocking read — suspends caller until management thread completes
int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);

// Blocking probe — returns OS_ERR_NONE if device ACKs its address
int32_t iic_mgmt_sync_probe(uint8_t bus_id, uint16_t dev_addr,
                             uint32_t timeout_ms);
```

### I2C Configuration Knobs

| Macro | Default | Effect |
|-------|---------|--------|
| `INC_SERVICE_IIC_MGMT` | `1` | Enable/disable the thread |
| `PROC_SERVICE_IIC_MGMT_STACK_SIZE` | `1024` words | Stack depth |
| `PROC_SERVICE_IIC_MGMT_PRIORITY` | `1` | Task priority |
| `TIME_OFFSET_IIC_MANAGEMENT` | `6500` ms | Startup delay |
| `IIC_MGMT_QUEUE_DEPTH` | `8` items | Max pending requests |
| `IIC_ACK_TIMEOUT_MS` | `100` ms | HAL device-ready timeout |

---

## SPI Management — `spi_mgmt`

**Files:**
- [services/spi_mgmt.c](services/spi_mgmt.c)
- [include/services/spi_mgmt.h](include/services/spi_mgmt.h)

**Compile guard:** `BOARD_SPI_COUNT > 0` (no Kconfig flag — always compiled, C guard eliminates dead code when no SPI buses are defined in the board XML)

### SPI State

```c
static QueueHandle_t _mgmt_queue = NULL;
```

### SPI Thread Body

```
spi_mgmt_thread()
│
├─ os_thread_delay(TIME_OFFSET_SPI_MANAGEMENT)        // 5500 ms
│
├─ ops = hal_spi_stm32_get_ops()
│
├─ bc = board_get_config()
│   for i in 0 .. bc->spi_count - 1:
│       d = &bc->spi_table[i]
│       h = drv_spi_get_handle(d->dev_id)
│       hal_spi_stm32_set_config(h,
│           d->instance, d->mode, d->direction,
│           d->data_size, d->clk_polarity, d->clk_phase,
│           d->nss, d->baud_prescaler, d->bit_order)
│       drv_spi_register(d->dev_id, ops, 0, timeout=10)  // hw_init → HAL_SPI_Init()
│
└─ for (;;):
       msg ← xQueueReceive(_mgmt_queue, portMAX_DELAY)
       h   = drv_spi_get_handle(msg.bus_id)
       result = OS_ERR_OP

       switch msg.cmd:

         SPI_MGMT_CMD_TRANSMIT:
           result = h->ops->transmit(h, msg.tx_data, msg.len, h->timeout_ms)

         SPI_MGMT_CMD_RECEIVE:
           result = h->ops->receive(h, msg.rx_data, msg.len, h->timeout_ms)

         SPI_MGMT_CMD_TRANSFER:                           // full-duplex
           result = h->ops->transfer(h,
               msg.tx_data, msg.rx_data, msg.len, h->timeout_ms)

         SPI_MGMT_CMD_REINIT:
           h->ops->hw_deinit(h)
           result = h->ops->hw_init(h)

       if msg.result_code != NULL: *msg.result_code = result

notify:
       if msg.result_notify != NULL: xTaskNotifyGive(msg.result_notify)
```

### SPI Message Protocol

```c
typedef enum {
    SPI_MGMT_CMD_TRANSMIT = 0,
    SPI_MGMT_CMD_RECEIVE,
    SPI_MGMT_CMD_TRANSFER,   // full-duplex TX and RX simultaneously
    SPI_MGMT_CMD_REINIT,
} spi_mgmt_cmd_t;

typedef struct {
    spi_mgmt_cmd_t   cmd;
    uint8_t          bus_id;
    const uint8_t   *tx_data;       // TX buffer (TRANSMIT / TRANSFER)
    uint8_t         *rx_data;       // RX buffer (RECEIVE / TRANSFER)
    uint16_t         len;
    TaskHandle_t     result_notify; // non-NULL → blocking operation
    int32_t         *result_code;   // non-NULL → write result here
} spi_mgmt_msg_t;
```

### Sync Full-Duplex Transfer Flow

```
Task A
  │  spi_mgmt_sync_transfer(SPI_FLASH_BUS, tx, rx, len, timeout=10)
  │    ├─ msg.cmd           = SPI_MGMT_CMD_TRANSFER
  │    ├─ msg.result_notify = xTaskGetCurrentTaskHandle()
  │    ├─ msg.result_code   = &result
  │    ├─ xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(10))
  │    └─ ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10))  ← BLOCKS
  │
  ▼ (suspended)

spi_mgmt thread
  │  xQueueReceive → msg (TRANSFER)
  │  h->ops->transfer(h, tx, rx, len, timeout) → HAL_SPI_TransmitReceive()
  │  *msg.result_code = result
  └─ xTaskNotifyGive(msg.result_notify)

Task A resumes
  └─ rx buffer filled, result available
```

### CS Pin Responsibility

The SPI management thread does **not** control chip-select (CS) GPIO pins. The caller is responsible for asserting CS before posting the transfer message and deasserting it after the blocking call returns (or after the async transmit completes):

```c
// Assert CS (active-low example)
drv_gpio_clear(drv_gpio_get_handle(GPIO_FLASH_CS));

// Blocking transfer
int32_t ret = spi_mgmt_sync_transfer(SPI_FLASH_BUS, tx_buf, rx_buf, 4, 10);

// Deassert CS
drv_gpio_set(drv_gpio_get_handle(GPIO_FLASH_CS));
```

Or use `gpio_mgmt_post()` with `delay_ms = 0` for deferred deassert.

### SPI Public API

```c
int32_t       spi_mgmt_start(void);
QueueHandle_t spi_mgmt_get_queue(void);

// Non-blocking transmit (fire-and-forget)
int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len);

// Non-blocking full-duplex transfer (fire-and-forget)
int32_t spi_mgmt_async_transfer(uint8_t bus_id,
                                 const uint8_t *tx, uint8_t *rx,
                                 uint16_t len);

// Blocking TX-only — suspends caller until complete
int32_t spi_mgmt_sync_transmit(uint8_t bus_id,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms);

// Blocking RX-only — suspends caller until complete
int32_t spi_mgmt_sync_receive(uint8_t bus_id,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);

// Blocking full-duplex transfer — suspends caller until complete
int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms);
```

### SPI Configuration Knobs

| Macro | Default | Effect |
|-------|---------|--------|
| `PROC_SERVICE_SPI_MGMT_STACK_SIZE` | `512` words | Stack depth |
| `PROC_SERVICE_SPI_MGMT_PRIORITY` | `1` | Task priority |
| `TIME_OFFSET_SPI_MANAGEMENT` | `5500` ms | Startup delay |
| `SPI_MGMT_QUEUE_DEPTH` | `8` items | Max pending requests |

---

## GPIO Management — `gpio_mgmt`

**Files:**
- [services/gpio_mgmt.c](services/gpio_mgmt.c)
- [include/services/gpio_mgmt.h](include/services/gpio_mgmt.h)

**Compile guard:** always compiled (no Kconfig or count guard). GPIO registration always runs at startup regardless of what other peripherals are defined.

### GPIO State

```c
static QueueHandle_t _mgmt_queue = NULL;
```

### GPIO Thread Body

```
gpio_mgmt_thread()
│
├─ os_thread_delay(TIME_OFFSET_GPIO_MANAGEMENT)       // 3000 ms — runs first
│
├─ ops = hal_gpio_stm32_get_ops()
│
├─ bc = board_get_config()
│   for i in 0 .. bc->gpio_count - 1:
│       d = &bc->gpio_table[i]
│       h = drv_gpio_get_handle(d->dev_id)
│
│       board_gpio_clk_enable(d->port)             // __HAL_RCC_GPIOx_CLK_ENABLE()
│                                                  // MUST happen before HAL_GPIO_Init
│
│       hal_gpio_stm32_set_config(h,               // stores in h->hw: port, pin,
│           d->port, d->pin,                       // mode, pull, speed, active_state
│           d->mode, d->pull, d->speed,            // does NOT call HAL_GPIO_Init
│           d->active_state)
│
│       drv_gpio_register(d->dev_id, ops)          // calls hw_init()
│           └─ ops->hw_init(h)                     //   → HAL_GPIO_Init() runs here
│
│       if h->initialized && d->initial_state      // apply boot state for outputs
│           h->ops->set(h)                         //   → HAL_GPIO_WritePin(SET)
│
└─ for (;;):
       msg ← xQueueReceive(_mgmt_queue, portMAX_DELAY)

       if msg.delay_ms > 0:
           os_thread_delay(msg.delay_ms)           // ← deferred execution

       h = drv_gpio_get_handle(msg.gpio_id)
       if h == NULL || !h->initialized: continue

       switch msg.cmd:
         GPIO_MGMT_CMD_SET:    h->ops->set(h)
         GPIO_MGMT_CMD_CLEAR:  h->ops->clear(h)
         GPIO_MGMT_CMD_TOGGLE: h->ops->toggle(h)
         GPIO_MGMT_CMD_WRITE:  h->ops->write(h, msg.state)
         GPIO_MGMT_CMD_REINIT:
           h->ops->hw_deinit(h)
           h->ops->hw_init(h)
```

### GPIO Registration Sequence

The three-step ordering for each GPIO pin is critical:

```
Step 1 — board_gpio_clk_enable(port)
          Calls __HAL_RCC_GPIOx_CLK_ENABLE()
          Without this, HAL_GPIO_Init() writes to an un-clocked peripheral
          and produces no effect or a hard fault.

Step 2 — hal_gpio_stm32_set_config(h, port, pin, mode, pull, speed, active_state)
          Stores all parameters into h->hw (drv_hw_gpio_ctx_t).
          Does NOT call HAL_GPIO_Init() — it is store-only.
          This prevents a double-init: drv_gpio_register will call hw_init().

Step 3 — drv_gpio_register(dev_id, ops)
          Binds the ops vtable to the handle.
          Calls ops->hw_init(h) which reads h->hw and calls HAL_GPIO_Init().
          Sets h->initialized = 1 on success.
```

If `set_config` called `HAL_GPIO_Init` directly, then `drv_gpio_register` would call it again through `hw_init`, producing a double-init. The store-only design avoids this.

### GPIO Message Protocol

```c
typedef enum {
    GPIO_MGMT_CMD_SET = 0,    // drive pin to active level
    GPIO_MGMT_CMD_CLEAR,      // drive pin to inactive level
    GPIO_MGMT_CMD_TOGGLE,     // toggle current output level
    GPIO_MGMT_CMD_WRITE,      // write explicit 0/1 (uses msg.state)
    GPIO_MGMT_CMD_REINIT,     // deinit + init (e.g. after mode change)
} gpio_mgmt_cmd_t;

typedef struct {
    gpio_mgmt_cmd_t cmd;
    uint8_t         gpio_id;    // logical GPIO ID from board_device_ids.h
    uint8_t         state;      // for WRITE command: 0 or 1
    uint32_t        delay_ms;   // 0 = execute immediately, >0 = sleep first
} gpio_mgmt_msg_t;
```

**`active_state` abstraction:** `GPIO_MGMT_CMD_SET` drives the pin to its logical active level (could be `GPIO_PIN_SET` or `GPIO_PIN_RESET` depending on what was configured in the board XML). The driver layer handles the polarity inversion — callers always think in terms of "active" / "inactive".

### Delayed Command Flow

The `delay_ms` field allows time-sequenced GPIO operations without blocking the calling task:

```c
// Assert active-low nRESET immediately, release after 50 ms
gpio_mgmt_post(GPIO_RESET, GPIO_MGMT_CMD_CLEAR, 0, 0);   // immediate
gpio_mgmt_post(GPIO_RESET, GPIO_MGMT_CMD_SET,   0, 50);  // 50 ms later

// Execution timeline inside gpio_mgmt thread:
//   msg1: delay_ms=0 → execute SET immediately
//   msg2: delay_ms=50 → os_thread_delay(50) → execute CLEAR
//
// WARNING: while gpio_mgmt is sleeping for 50 ms it cannot process
// other queue messages. Do not post delay_ms > 0 if other GPIO
// messages need to be serviced promptly.
```

### GPIO Public API

```c
int32_t       gpio_mgmt_start(void);
QueueHandle_t gpio_mgmt_get_queue(void);

// Post any GPIO command — non-blocking (fire-and-forget)
int32_t gpio_mgmt_post(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                        uint8_t state, uint32_t delay_ms);
```

**Bypassing the management thread:** For latency-critical GPIO operations (e.g. toggling a debug pin inside an ISR), call the generic driver directly:

```c
drv_gpio_handle_t *h = drv_gpio_get_handle(LED_BOARD);
h->ops->toggle(h);   // no queue, no FreeRTOS involvement
```

This is safe as long as only one execution context accesses the same pin at a time.

### GPIO Configuration Knobs

| Macro | Default | Effect |
|-------|---------|--------|
| `PROC_SERVICE_GPIO_MGMT_STACK_SIZE` | `256` words | Stack depth |
| `PROC_SERVICE_GPIO_MGMT_PRIORITY` | `1` | Task priority |
| `TIME_OFFSET_GPIO_MANAGEMENT` | `3000` ms | Startup delay (runs first) |
| `GPIO_MGMT_QUEUE_DEPTH` | `16` items | Max pending commands |

---

## OS Shell Management — `os_shell_mgmt`

**Files:**
- [services/os_shell_management.c](services/os_shell_management.c)
- [include/services/os_shell_management.h](include/services/os_shell_management.h)

**Compile guard:** `INC_SERVICE_OS_SHELL_MGMT == 1` (set in `config/conf_os.h`)

The OS shell is a FreeRTOS+CLI-backed interactive console accessed over the designated shell UART (`UART_SHELL_HW_ID`). It is a consumer of the UART ring-buffer infrastructure rather than a management owner — it reads from `global_uart_rx_mqueue_list[UART_SHELL_HW_ID]` (filled by the UART ISR via the IRQ desc chain) and writes into `global_uart_tx_mqueue_list[UART_SHELL_HW_ID]` (drained by the TXEIE interrupt path).

### Shell I/O Path

```
USART2_IRQHandler()
  └─ hal_uart_stm32_irq_handler()
       ├─ RXNE: ringbuffer_putchar(global_uart_rx_mqueue_list[1], byte)
       │         drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(1), ...)
       └─ TXE:  drv_uart_tx_get_next_byte(1, &b)
                 WRITE_REG(USART2->DR, b)
                 (TXEIE disabled when ring buffer empties)

_os_shell_task()
  ├─ reads bytes from global_uart_rx_mqueue_list[UART_SHELL_HW_ID]
  ├─ assembles lines (backspace/DEL, Ctrl-C clear, CR/LF submit)
  └─ FreeRTOS_CLIProcessCommand() → _shell_write() → ring buffer + drv_uart_tx_kick()
```

The shell task never calls `uart_mgmt_async_transmit()`. It writes directly to the TX ring buffer and calls `drv_uart_tx_kick()` to enable TXEIE, so the shell works independently of the `uart_mgmt` queue depth.

### Thread Body

```
_os_shell_task()
│
├─ os_thread_delay(TIME_OFFSET_OS_SHELL_MGMT)   // starts after uart_mgmt is ready
│
├─ FreeRTOS_CLIRegisterCommand(&help_cmd)
├─ FreeRTOS_CLIRegisterCommand(&version_cmd)
├─ FreeRTOS_CLIRegisterCommand(&uptime_cmd)
├─ FreeRTOS_CLIRegisterCommand(&reboot_cmd)
│
└─ for (;;):
       byte = ringbuffer_getchar(rx_rb)
       if no byte:
           vTaskDelay(pdMS_TO_TICKS(5))    // yield — no busy-poll
           continue
       accumulate into _line_buf[]
       on CR or LF:
           _shell_process_line()
               do {
                   more = FreeRTOS_CLIProcessCommand(
                               _line_buf, _out_buf, SHELL_OUT_BUF_LEN)
                   _shell_write(_out_buf, strlen(_out_buf))
               } while (more == pdTRUE)
           emit prompt "> "
```

### Built-in Commands

| Command   | Description |
|-----------|-------------|
| `help`    | Lists all registered commands with their help strings |
| `version` | Prints the firmware version string |
| `uptime`  | Prints scheduler tick count and elapsed time |
| `reboot`  | Calls `NVIC_SystemReset()` — immediate MCU reset |

### Registering Application Commands

```c
#include <services/os_shell_management.h>

static BaseType_t my_cmd_handler(char *out, size_t out_len,
                                  const char *args)
{
    snprintf(out, out_len, "hello from my_cmd\r\n");
    return pdFALSE;   /* pdTRUE = more output pending, pdFALSE = done */
}

static const CLI_Command_Definition_t my_cmd = {
    "my_cmd",
    "my_cmd: prints a greeting\r\n",
    my_cmd_handler,
    0   /* expected parameter count */
};

/* Call before os_shell_mgmt_start(), or from any task before the shell task runs */
os_shell_register_command(&my_cmd);
```

### Public API

```c
/* Start the shell task — call before vTaskStartScheduler() */
int32_t os_shell_mgmt_start(void);

/* Register a FreeRTOS+CLI command definition */
static inline BaseType_t os_shell_register_command(
        const CLI_Command_Definition_t *cmd);
```

### Configuration Knobs

All in `config/conf_os.h`:

| Macro | Default | Effect |
|-------|---------|--------|
| `INC_SERVICE_OS_SHELL_MGMT` | `1` | Compile and start the shell task |
| `UART_SHELL_HW_ID` | `1` | `dev_id` of the shell UART (must match `BOARD_UART_SHELL_ID`) |
| `SHELL_LINE_BUF_LEN` | `128` | Max input line length in bytes |
| `SHELL_OUT_BUF_LEN` | `512` | Output scratch buffer per CLI call |
| `TIME_OFFSET_OS_SHELL_MGMT` | `4500` ms | Startup delay — must be > `TIME_OFFSET_SERIAL_MANAGEMENT` |

See [SHELL_CLI.md](SHELL_CLI.md) for the full shell system architecture and guide.

---

## Application Driver Layer — `drv_app`

**Files:**
- `drv_app/uart_app.c` / `include/drv_app/uart_app.h`
- `drv_app/spi_app.c`  / `include/drv_app/spi_app.h`
- `drv_app/iic_app.c`  / `include/drv_app/iic_app.h`
- `drv_app/gpio_app.c` / `include/drv_app/gpio_app.h`

The `drv_app` layer provides the **recommended application-facing API**. It wraps the management service queues and the task-notification sync pattern into simple function calls so application tasks do not need to know about `QueueHandle_t`, `TaskHandle_t`, or `xTaskNotifyTake`.

### UART App API

```c
// Blocking TX — posts to uart_mgmt queue with result_notify set
int32_t uart_sync_transmit(uint8_t dev_id, const uint8_t *data,
                            uint16_t len, uint32_t timeout_ms);

// Blocking RX — polls uart_mgmt_read_byte() in 1 ms ticks
int32_t uart_sync_receive(uint8_t dev_id, uint8_t *buf,
                           uint16_t len, uint32_t timeout_ms);

// Non-blocking TX — wraps uart_mgmt_async_transmit()
int32_t uart_async_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);

// Non-blocking single-byte RX from ISR ring buffer
int32_t uart_async_read_byte(uint8_t dev_id, uint8_t *byte);
```

### SPI App API

```c
int32_t spi_sync_transmit(uint8_t bus_id, const uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);
int32_t spi_sync_receive(uint8_t bus_id, uint8_t *data,
                          uint16_t len, uint32_t timeout_ms);
int32_t spi_sync_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                           uint16_t len, uint32_t timeout_ms);
int32_t spi_async_transmit(uint8_t bus_id, const uint8_t *data, uint16_t len);
int32_t spi_async_transfer(uint8_t bus_id, const uint8_t *tx, uint8_t *rx,
                            uint16_t len);
```

### I2C App API

```c
int32_t iic_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           const uint8_t *data, uint16_t len,
                           uint32_t timeout_ms);
int32_t iic_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                          uint8_t reg_addr, uint8_t use_reg,
                          uint8_t *data, uint16_t len,
                          uint32_t timeout_ms);
int32_t iic_sync_probe(uint8_t bus_id, uint16_t dev_addr, uint32_t timeout_ms);
int32_t iic_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                            uint8_t reg_addr, uint8_t use_reg,
                            const uint8_t *data, uint16_t len);
int32_t iic_async_receive(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           uint8_t *data, uint16_t len);
```

### GPIO App API

```c
// Direct driver read — bypasses management queue for zero-latency reads
uint8_t gpio_read(uint8_t gpio_id);

// Async commands — post via gpio_mgmt_post()
int32_t gpio_async_set(uint8_t gpio_id);
int32_t gpio_async_clear(uint8_t gpio_id);
int32_t gpio_async_toggle(uint8_t gpio_id);
int32_t gpio_async_write(uint8_t gpio_id, uint8_t state);
int32_t gpio_async_delayed(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                            uint8_t state, uint32_t delay_ms);
```

> **`gpio_read` note:** GPIO reads are instantaneous and non-serialisation-critical. `gpio_read()` calls `drv_gpio_get_handle(gpio_id)->ops->read(h)` directly without going through the management queue, giving zero-latency reads safe from any task (but not from an ISR — use the HAL directly from ISR context).

---

## Compile-Time Activation

Management threads are enabled/disabled in `config/conf_os.h` (set via `make menuconfig`):

| Service | Guard macro | Default |
|---------|-------------|---------|
| UART management | `INC_SERVICE_UART_MGMT == 1` | `1` (enabled) |
| I2C management | `INC_SERVICE_IIC_MGMT == 1` | `1` (enabled) |
| SPI management | `BOARD_SPI_COUNT > 0` | depends on board XML |
| GPIO management | always compiled | always active |

UART and I2C are wrapped in `#if (INC_SERVICE_xxx_MGMT == 1) … #endif` blocks that cover the entire translation unit body. Setting the flag to `0` produces an empty object file.

SPI uses a C preprocessor count from the generated board header:

```c
#if (BOARD_SPI_COUNT > 0)
// entire spi_mgmt_thread body + API
#endif
```

GPIO has no guard — it always registers all GPIOs from the board config.

---

## Thread Timing Summary

| Thread | Startup delay | Board config iterated | First message after |
|--------|--------------|----------------------|---------------------|
| `gpio_mgmt` | 3 000 ms | `gpio_table` | 3 000 ms |
| `uart_mgmt` | 4 000 ms | `uart_table` | 4 000 ms |
| `spi_mgmt` | 5 500 ms | `spi_table` | 5 500 ms |
| `iic_mgmt` | 6 500 ms | `iic_table` | 6 500 ms |

GPIO runs first because UART, SPI, and I2C MSP init functions (`stm32f4xx_hal_msp.c`) call `HAL_GPIO_Init()` for their alternate-function pins. Those GPIO clocks must already be enabled by `gpio_mgmt` before the HAL MSP runs.

---

## Adding a New Management Thread

Follow this pattern to add a new peripheral class (e.g. `can_mgmt`):

**1. Define the message type in `include/services/can_mgmt.h`:**

```c
typedef enum {
    CAN_MGMT_CMD_TRANSMIT = 0,
    CAN_MGMT_CMD_REINIT,
} can_mgmt_cmd_t;

typedef struct {
    can_mgmt_cmd_t   cmd;
    uint8_t          bus_id;
    const uint8_t   *data;
    uint8_t          dlc;
    uint32_t         can_id;
    TaskHandle_t     result_notify;
    int32_t         *result_code;
} can_mgmt_msg_t;
```

**2. Implement `services/can_mgmt.c`:**

```c
static QueueHandle_t _mgmt_queue = NULL;

static void can_mgmt_thread(void *arg)
{
    os_thread_delay(TIME_OFFSET_CAN_MANAGEMENT);  // e.g. 10000 ms

    const drv_can_hal_ops_t *ops = hal_can_stm32_get_ops();
    const board_config_t    *bc  = board_get_config();
    for (uint8_t i = 0; i < bc->can_count; i++) {
        const board_can_desc_t *d = &bc->can_table[i];
        drv_can_handle_t       *h = drv_can_get_handle(d->dev_id);
        hal_can_stm32_set_config(h, d->instance, d->baud_prescaler, …);
        drv_can_register(d->dev_id, ops, d->baudrate, 10);
    }

    can_mgmt_msg_t msg;
    for (;;) {
        xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY);
        drv_can_handle_t *h = drv_can_get_handle(msg.bus_id);
        int32_t result = OS_ERR_OP;
        switch (msg.cmd) {
            case CAN_MGMT_CMD_TRANSMIT:
                result = h->ops->transmit(h, msg.can_id, msg.data, msg.dlc);
                break;
            case CAN_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h); result = h->ops->hw_init(h); break;
        }
        if (msg.result_code)   *msg.result_code = result;
        if (msg.result_notify)  xTaskNotifyGive(msg.result_notify);
    }
}

int32_t can_mgmt_start(void) {
    _mgmt_queue = xQueueCreate(CAN_MGMT_QUEUE_DEPTH, sizeof(can_mgmt_msg_t));
    if (!_mgmt_queue) return OS_ERR_MEM_OF;
    int32_t tid = os_thread_create(can_mgmt_thread, "can_mgmt",
                                   PROC_SERVICE_CAN_MGMT_STACK_SIZE,
                                   PROC_SERVICE_CAN_MGMT_PRIORITY, NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}
```

**3. Add activation flag in `config/conf_os.h` (or add a Kconfig entry in `kernel/Kconfig`):**

```c
#define INC_SERVICE_CAN_MGMT   (1)
#define TIME_OFFSET_CAN_MANAGEMENT   10000
#define PROC_SERVICE_CAN_MGMT_STACK_SIZE   1024
#define PROC_SERVICE_CAN_MGMT_PRIORITY     1
```

**4. Call `can_mgmt_start()` from `app_main()` or `main.c` before `vTaskStartScheduler()`.**

**5. Add `<can>` elements to the board XML and extend the code generator's `_emit_can_table()` function — see [BOARD.md](BOARD.md).**

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
