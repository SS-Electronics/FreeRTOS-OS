# DMA Engine — FreeRTOS-OS

A Linux `dmaengine`-style DMA framework for bare-metal ARM Cortex-M4.  
Provides a vendor-agnostic API for submitting DMA transfers asynchronously, with completion callbacks fired from ISR context.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Directory Layout](#directory-layout)
- [Core Concepts](#core-concepts)
  - [dma_device — Controller](#dma_device--controller)
  - [dma_chan — Channel / Stream](#dma_chan--channel--stream)
  - [dma_async_tx_descriptor — Transfer Descriptor](#dma_async_tx_descriptor--transfer-descriptor)
  - [dma_slave_config — Peripheral Config](#dma_slave_config--peripheral-config)
  - [Descriptor Pool](#descriptor-pool)
- [Public API](#public-api)
  - [Controller Registration](#controller-registration)
  - [Channel Allocation](#channel-allocation)
  - [Slave Configuration](#slave-configuration)
  - [Descriptor Preparation](#descriptor-preparation)
  - [Submission and Execution](#submission-and-execution)
  - [Control](#control)
- [STM32F4 HAL Backend](#stm32f4-hal-backend)
  - [Stream / Channel Mux Table](#stream--channel-mux-table)
  - [Initialisation](#initialisation)
  - [IRQ Dispatch Wiring](#irq-dispatch-wiring)
- [Usage Examples](#usage-examples)
  - [UART TX — Memory to Peripheral](#uart-tx--memory-to-peripheral)
  - [UART RX — Peripheral to Memory](#uart-rx--peripheral-to-memory)
  - [Memory-to-Memory Copy](#memory-to-memory-copy)
  - [Cyclic Transfer (Audio / ADC Ring)](#cyclic-transfer-audio--adc-ring)
- [Transfer Flow](#transfer-flow)
- [Configuration Reference](#configuration-reference)
- [Adding a New Vendor Backend](#adding-a-new-vendor-backend)

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│               Application / Driver code                  │
│   dma_request_chan()  dmaengine_prep_slave_single()      │
│   dmaengine_submit()  dma_async_issue_pending()          │
└───────────────────────┬──────────────────────────────────┘
                        │ vendor-agnostic API
┌───────────────────────▼──────────────────────────────────┐
│               DMA Engine Core  (drv_dma.c)               │
│   device registry · descriptor pool · pending queue      │
│   dma_complete_callback()  dma_error_callback()          │
└───────────────────────┬──────────────────────────────────┘
                        │ dma_chan_hal_ops_t vtable
┌───────────────────────▼──────────────────────────────────┐
│            STM32 HAL Backend  (hal_dma_stm32.c)          │
│   DMA_HandleTypeDef · HAL_DMA_Start_IT()                 │
│   hal_dma_stm32_irq_dispatch()                           │
└───────────────────────┬──────────────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────────────┐
│          DMA1 / DMA2  hardware  (STM32F411VET6)          │
│          16 streams × 8 channel-mux request lines        │
└──────────────────────────────────────────────────────────┘
```

The engine core is vendor-agnostic. Porting to a new MCU only requires a new HAL backend that implements `dma_chan_hal_ops_t`.

---

## Directory Layout

```
FreeRTOS-OS/
├── drivers/
│   ├── dma/
│   │   └── drv_dma.c                   Engine core
│   └── hal/
│       └── stm32/
│           └── hal_dma_stm32.c         STM32F4 HAL backend
└── include/
    └── drivers/
        └── dma/
            ├── drv_dma.h               Public API and all type definitions
            └── hal/
                └── stm32/
                    └── hal_dma_stm32.h STM32 HAL interface
```

---

## Core Concepts

### `dma_device` — Controller

Represents one physical DMA controller (DMA1 or DMA2).  
Each device holds an array of `dma_chan_t` (one per stream) and a pointer to the `dma_chan_hal_ops_t` vtable.

```c
struct dma_device {
    const char              *name;           // "DMA1" or "DMA2"
    uint8_t                  nr_channels;    // 8 for STM32F4
    const dma_chan_hal_ops_t *ops;           // HAL vtable
    dma_chan_t               channels[DMA_MAX_CHANNELS];
    struct list_node         list;           // global registry linkage
};
```

Controllers are registered at boot by calling `dma_register_device()`.  
For STM32 targets `hal_dma_stm32_init()` does this automatically.

---

### `dma_chan` — Channel / Stream

A logical channel corresponding to one DMA stream.  
Obtained by calling `dma_request_chan()` and released with `dma_release_chan()`.

```
State machine:
  IDLE ──request──► ALLOCATED ──issue_pending──► BUSY
   ▲                    ▲                           │
   └──release───────────┘        complete / error ──┘
```

Each channel owns a **static descriptor pool** (`desc_pool[DMA_DESC_POOL_SIZE]`) and a **pending FIFO queue**.  Descriptors submitted via `dmaengine_submit()` queue up; `dma_async_issue_pending()` starts the head of the queue. On completion the next queued descriptor starts automatically.

---

### `dma_async_tx_descriptor` — Transfer Descriptor

Describes one transfer.  Allocated from the per-channel pool by any `dmaengine_prep_*()` call.

```c
typedef struct dma_async_tx_descriptor {
    dma_chan_t               *chan;
    dma_cookie_t              cookie;         // unique transfer ID
    dma_desc_state_t          state;          // FREE → PREPARED → SUBMITTED → ...

    uint32_t                  src;            // source address
    uint32_t                  dst;            // destination address
    uint32_t                  len;            // total bytes
    uint32_t                  period_len;     // cyclic period (cyclic only)
    dma_transfer_direction_t  direction;
    dma_xfer_mode_t           mode;           // NORMAL or CYCLIC

    void (*callback)      (void *param);      // fires on completion (ISR context)
    void (*error_callback)(void *param);      // fires on DMA error  (ISR context)
    void  *callback_param;
} dma_async_tx_descriptor_t;
```

> **Important:** `callback` and `error_callback` are called from **ISR context**.  
> Keep them short — set a flag or post to a FreeRTOS queue; do not block.

---

### `dma_slave_config` — Peripheral Config

Configures the channel for a slave (peripheral ↔ memory) transfer.

```c
typedef struct {
    dma_transfer_direction_t  direction;
    uint32_t                  src_addr;       // peripheral register address
    uint32_t                  dst_addr;       // peripheral register address
    dma_slave_buswidth_t      src_addr_width; // 1 / 2 / 4 bytes
    dma_slave_buswidth_t      dst_addr_width;
    uint32_t                  src_maxburst;   // beats per burst (1 = single)
    uint32_t                  dst_maxburst;
    dma_addr_adj_t            src_addr_adj;   // INCREMENT or FIXED
    dma_addr_adj_t            dst_addr_adj;
    uint8_t                   request;        // HW channel mux (0–7, STM32F4)
} dma_slave_config_t;
```

**Common patterns:**

| Transfer | `src_addr_adj` | `dst_addr_adj` | `src_addr` | `dst_addr` |
|---|---|---|---|---|
| MEM → peripheral (TX) | `INCREMENT` | `FIXED` | — | `&PERIPHx->DR` |
| Peripheral → MEM (RX) | `FIXED` | `INCREMENT` | `&PERIPHx->DR` | — |
| MEM → MEM (memcpy) | `INCREMENT` | `INCREMENT` | src buffer | dst buffer |

---

### Descriptor Pool

Each channel holds `DMA_DESC_POOL_SIZE` (default **4**) descriptors in a statically allocated pool.  
`dmaengine_prep_*()` allocates from this pool; the descriptor is returned to the pool when the transfer completes or is terminated.

If all 4 descriptors are in use, `dmaengine_prep_*()` returns `NULL`.  
Increase `DMA_DESC_POOL_SIZE` in `drv_dma.h` if your use-case needs deeper pipelining.

---

## Public API

### Controller Registration

```c
int32_t dma_register_device(dma_device_t *dev);
```

Registers a DMA controller.  Initialises every channel's pending queue and calls `chan_init` on each stream.  For STM32 call `hal_dma_stm32_init()` instead — it enables clocks, wires stream pointers, and calls `dma_register_device()` for both controllers.

---

### Channel Allocation

```c
dma_chan_t *dma_request_chan(const char *dev_name, uint8_t chan_id);
void        dma_release_chan(dma_chan_t *chan);
```

- `dev_name` — `"DMA1"` or `"DMA2"`
- `chan_id`  — stream index `0`–`7`
- Returns `NULL` if the stream is already allocated or the device is not found.
- `dma_release_chan()` calls `dmaengine_terminate_all()` before marking the channel idle.

---

### Slave Configuration

```c
int32_t dmaengine_slave_config(dma_chan_t *chan, const dma_slave_config_t *cfg);
```

Stores the config in the channel and calls the HAL `slave_config` op, which programs the `DMA_HandleTypeDef` and calls `HAL_DMA_Init()`.  Must be called before any `dmaengine_prep_*()`.

---

### Descriptor Preparation

```c
// Peripheral ↔ memory single-shot
dma_async_tx_descriptor_t *dmaengine_prep_slave_single(
    dma_chan_t *chan, uint32_t buf, uint32_t len,
    dma_transfer_direction_t dir);

// Memory-to-memory copy
dma_async_tx_descriptor_t *dmaengine_prep_dma_memcpy(
    dma_chan_t *chan, uint32_t dst, uint32_t src, uint32_t len);

// Continuous cyclic (audio ring buffer, ADC streaming)
dma_async_tx_descriptor_t *dmaengine_prep_dma_cyclic(
    dma_chan_t *chan, uint32_t buf_addr, uint32_t buf_len,
    uint32_t period_len, dma_transfer_direction_t dir);
```

All three return a pointer to a descriptor from the pool, or `NULL` on pool exhaustion.  
After receiving the descriptor, set `callback` / `callback_param` before submitting.

---

### Submission and Execution

```c
dma_cookie_t dmaengine_submit        (dma_async_tx_descriptor_t *desc);
void         dma_async_issue_pending (dma_chan_t *chan);
```

`dmaengine_submit()` enqueues the descriptor and returns a monotonic **cookie** (positive integer, unique per channel).  
`dma_async_issue_pending()` starts the head of the queue if no transfer is currently active.  Multiple descriptors can be submitted before calling `dma_async_issue_pending()` — they execute in FIFO order, chained automatically on each completion.

---

### Control

```c
int32_t  dmaengine_terminate_all (dma_chan_t *chan);
int32_t  dmaengine_pause         (dma_chan_t *chan);
int32_t  dmaengine_resume        (dma_chan_t *chan);
uint32_t dmaengine_get_residue   (dma_chan_t *chan);  // bytes remaining
```

- `terminate_all` — aborts the active transfer and discards the pending queue.  
- `pause` / `resume` — disable / re-enable the stream EN bit (no HAL hardware pause exists on STM32F4).  
- `get_residue` — reads `NDTR × bus_width`, returning the number of bytes not yet transferred.

---

## STM32F4 HAL Backend

### Stream / Channel Mux Table

STM32F411 DMA request line assignments (abbreviated).  Full table: RM0383 §9.3.3.

| Stream | Channel | Peripheral |
|---|---|---|
| DMA1 Stream 0 | Ch 1 | I2C1_RX |
| DMA1 Stream 5 | Ch 4 | USART2_RX |
| DMA1 Stream 6 | Ch 1 | I2C1_TX |
| DMA1 Stream 6 | Ch 4 | USART2_TX |
| DMA2 Stream 2 | Ch 3 | SPI1_RX |
| DMA2 Stream 3 | Ch 3 | SPI1_TX |
| DMA2 Stream 3 | Ch 4 | USART1_TX |
| DMA2 Stream 5 | Ch 4 | USART1_RX |

Set `dma_slave_config_t.request` to the **channel number** (0–7); the backend shifts it into `DMA_SxCR_CHSEL`.

---

### Initialisation

Call once at boot, **after** the system clock is configured and **before** any driver that uses DMA:

```c
hal_dma_stm32_init();
```

Internally this:
1. Enables `DMA1` and `DMA2` AHB1 clocks via `__HAL_RCC_DMA1_CLK_ENABLE()`.
2. Wires `hdma.Instance` for each of the 16 streams.
3. Calls `dma_register_device()` for `hal_dma1_device` and `hal_dma2_device`.

Placement in `main.c` (before `os_kernel_thread_register()`):

```c
/* After SystemCoreClockUpdate() and peripheral clock setup */
hal_dma_stm32_init();
```

---

### IRQ Dispatch Wiring

The project's IRQ framework owns the `DMAx_StreamY_IRQHandler` symbols through the generated `irq_periph_dispatch_generated.c`.  Wire each stream you use by adding a `<dma>` entry in `app/board/irq_table.xml` and re-running `make irq_gen`:

```xml
<!-- DMA1 Stream 6 — USART2 TX -->
<periph name="DMA1_Stream6"
        irqn="DMA1_Stream6_IRQn"
        priority="5"
        action="enable"
        dispatch="hal_dma_stm32_irq_dispatch(1, 6)"
        dispatch_include="&lt;drivers/dma/hal/stm32/hal_dma_stm32.h&gt;"/>

<!-- DMA1 Stream 5 — USART2 RX -->
<periph name="DMA1_Stream5"
        irqn="DMA1_Stream5_IRQn"
        priority="5"
        action="enable"
        dispatch="hal_dma_stm32_irq_dispatch(1, 5)"
        dispatch_include="&lt;drivers/dma/hal/stm32/hal_dma_stm32.h&gt;"/>
```

`hal_dma_stm32_irq_dispatch(ctrl, stream)` routes into `HAL_DMA_IRQHandler()` for the correct stream context.

> **Priority rule:** DMA stream IRQs must be at NVIC priority ≥ `configLIBRARY_MAX_SYSCALL_IRQ_PRIORITY` (5 on this project) to safely use FreeRTOS API from callbacks.

---

## Usage Examples

### UART TX — Memory to Peripheral

```c
#include <drivers/dma/drv_dma.h>
#include <drivers/dma/hal/stm32/hal_dma_stm32.h>

static dma_chan_t *_uart_tx_chan;

static void _uart_tx_done(void *param)
{
    /* Called from ISR — signal the waiting task */
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR((TaskHandle_t)param, &woken);
    portYIELD_FROM_ISR(woken);
}

void uart_dma_init(void)
{
    _uart_tx_chan = dma_request_chan("DMA1", 6);   /* Stream 6, Ch 4 = USART2_TX */

    dma_slave_config_t cfg = {
        .direction      = DMA_MEM_TO_DEV,
        .dst_addr       = (uint32_t)&USART2->DR,
        .src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
        .dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
        .src_addr_adj   = DMA_ADDR_INCREMENT,
        .dst_addr_adj   = DMA_ADDR_FIXED,
        .request        = 4,
    };
    dmaengine_slave_config(_uart_tx_chan, &cfg);
}

void uart_dma_transmit(const uint8_t *buf, uint32_t len)
{
    dma_async_tx_descriptor_t *desc =
        dmaengine_prep_slave_single(_uart_tx_chan, (uint32_t)buf, len, DMA_MEM_TO_DEV);

    desc->callback       = _uart_tx_done;
    desc->callback_param = (void *)xTaskGetCurrentTaskHandle();

    dmaengine_submit(desc);
    dma_async_issue_pending(_uart_tx_chan);

    /* Wait for completion (task blocks until ISR gives notification) */
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
}
```

---

### UART RX — Peripheral to Memory

```c
static dma_chan_t *_uart_rx_chan;

void uart_dma_rx_init(void)
{
    _uart_rx_chan = dma_request_chan("DMA1", 5);   /* Stream 5, Ch 4 = USART2_RX */

    dma_slave_config_t cfg = {
        .direction      = DMA_DEV_TO_MEM,
        .src_addr       = (uint32_t)&USART2->DR,
        .src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
        .dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
        .src_addr_adj   = DMA_ADDR_FIXED,
        .dst_addr_adj   = DMA_ADDR_INCREMENT,
        .request        = 4,
    };
    dmaengine_slave_config(_uart_rx_chan, &cfg);
}

void uart_dma_receive(uint8_t *buf, uint32_t len,
                      void (*done_cb)(void *), void *cb_param)
{
    dma_async_tx_descriptor_t *desc =
        dmaengine_prep_slave_single(_uart_rx_chan, (uint32_t)buf, len, DMA_DEV_TO_MEM);

    desc->callback       = done_cb;
    desc->callback_param = cb_param;

    dmaengine_submit(desc);
    dma_async_issue_pending(_uart_rx_chan);
}
```

---

### Memory-to-Memory Copy

```c
static dma_chan_t *_memcpy_chan;

void dma_memcpy_init(void)
{
    /* DMA2 is required for MEM_TO_MEM on STM32F4 */
    _memcpy_chan = dma_request_chan("DMA2", 0);

    /* No slave_config needed for memcpy — the HAL backend defaults to
       byte-width, both addresses increment.                            */
}

void dma_memcpy_async(void *dst, const void *src, uint32_t len,
                      void (*done_cb)(void *), void *param)
{
    dma_async_tx_descriptor_t *desc =
        dmaengine_prep_dma_memcpy(_memcpy_chan,
                                  (uint32_t)dst, (uint32_t)src, len);

    desc->callback       = done_cb;
    desc->callback_param = param;

    dmaengine_submit(desc);
    dma_async_issue_pending(_memcpy_chan);
}
```

> **STM32F4 note:** Memory-to-memory DMA is only supported on **DMA2**, not DMA1.

---

### Cyclic Transfer (Audio / ADC Ring)

```c
static dma_chan_t *_adc_chan;
static uint16_t   _adc_buf[256];   /* double-buffer: first 128 / last 128 */

static void _adc_period_done(void *param)
{
    /* Called every 128 samples (period_len = 128 * sizeof(uint16_t)) */
    uint16_t *half = (uint16_t *)param;
    process_adc_samples(half, 128);
}

void adc_dma_cyclic_init(void)
{
    _adc_chan = dma_request_chan("DMA2", 4);   /* DMA2 Stream 4, Ch 0 = ADC1 */

    dma_slave_config_t cfg = {
        .direction      = DMA_DEV_TO_MEM,
        .src_addr       = (uint32_t)&ADC1->DR,
        .src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES,
        .dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES,
        .src_addr_adj   = DMA_ADDR_FIXED,
        .dst_addr_adj   = DMA_ADDR_INCREMENT,
        .request        = 0,
    };
    dmaengine_slave_config(_adc_chan, &cfg);

    uint32_t buf_len    = sizeof(_adc_buf);          /* 512 bytes total  */
    uint32_t period_len = sizeof(_adc_buf) / 2;      /* 256 bytes / half */

    dma_async_tx_descriptor_t *desc =
        dmaengine_prep_dma_cyclic(_adc_chan,
                                  (uint32_t)_adc_buf, buf_len, period_len,
                                  DMA_DEV_TO_MEM);

    desc->callback       = _adc_period_done;
    desc->callback_param = (void *)_adc_buf;

    dmaengine_submit(desc);
    dma_async_issue_pending(_adc_chan);
    /* Runs forever; HAL_DMA re-arms after each complete cycle. */
}
```

---

## Transfer Flow

```
dmaengine_prep_slave_single()
    │  allocates desc from pool
    ▼
dmaengine_submit()
    │  assigns cookie, pushes desc onto chan->pending FIFO
    ▼
dma_async_issue_pending()
    │  dequeues head, sets chan->active_desc, calls ops->start()
    ▼
HAL_DMA_Start_IT()          ← STM32 HAL starts the stream
    │
    │  (hardware DMA runs)
    │
DMAx_StreamY_IRQHandler()   ← IRQ fires on TC (Transfer Complete)
    │
hal_dma_stm32_irq_dispatch()
    │
HAL_DMA_IRQHandler()
    │  calls XferCpltCallback
    ▼
_hal_cplt_cb()
    │
dma_complete_callback()
    │  fires desc->callback (in ISR)
    │  frees desc back to pool
    │  auto-starts next queued descriptor (if any)
    ▼
  (next transfer or channel goes ALLOCATED)
```

---

## Configuration Reference

All limits are in `include/drivers/dma/drv_dma.h`:

| Macro | Default | Description |
|---|---|---|
| `DMA_MAX_DEVICES` | `2` | Maximum registered DMA controllers |
| `DMA_MAX_CHANNELS` | `8` | Streams per controller |
| `DMA_DESC_POOL_SIZE` | `4` | Descriptor pool depth per channel |

---

## Adding a New Vendor Backend

Implement the `dma_chan_hal_ops_t` vtable:

```c
typedef struct {
    int32_t  (*chan_init)    (dma_chan_t *chan);
    int32_t  (*chan_deinit)  (dma_chan_t *chan);
    int32_t  (*slave_config) (dma_chan_t *chan, const dma_slave_config_t *cfg);
    int32_t  (*start)        (dma_chan_t *chan, dma_async_tx_descriptor_t *desc);
    int32_t  (*terminate_all)(dma_chan_t *chan);
    int32_t  (*pause)        (dma_chan_t *chan);
    int32_t  (*resume)       (dma_chan_t *chan);
    uint32_t (*get_residue)  (dma_chan_t *chan);
} dma_chan_hal_ops_t;
```

Checklist:
1. Allocate a `dma_device_t` with `name`, `nr_channels`, and `ops` filled in.
2. For each channel, set `chan->hw_ctx` to point to your vendor context struct.
3. In `start()`, call `HAL_DMA_Start_IT()` (or vendor equivalent) and wire callbacks to call `dma_complete_callback(chan)` / `dma_error_callback(chan)`.
4. Call `dma_register_device(&your_device)` at boot.
5. Route ISRs through `dma_complete_callback()` / `dma_error_callback()`.
