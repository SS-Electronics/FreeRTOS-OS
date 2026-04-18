# IPC & Message Queues — FreeRTOS-OS

All inter-task communication is registered through a central queue registry (`ipc/mqueue.c`). The registry itself is an intrusive doubly-circular list of `type_message_queue_descriptor` nodes, so register/unregister are O(1).

---

## Table of Contents

- [Queue Types](#queue-types)
- [API](#api)
- [Usage Examples](#usage-examples)
  - [Inter-task typed queue](#inter-task-typed-queue)
  - [UART byte-stream ring buffer](#uart-byte-stream-ring-buffer)
  - [Iterate all registered queues](#iterate-all-registered-queues)
- [Management Service Queues](#management-service-queues)
- [Ring Buffer](#ring-buffer)

---

## Queue Types

| Type constant | Backend | Typical use |
|---|---|---|
| `IPC_MQUEUE_TYPE_PROC_TXN` | FreeRTOS `xQueueCreate` | Typed messages between tasks |
| `IPC_MQUEUE_TYPE_UART_HW` | RT-Thread `ringbuffer` | Byte stream from UART ISR |

---

## API

Include: `#include <ipc/mqueue.h>`

| Function | Description |
|---|---|
| `ipc_mqueue_init()` | Initialise the registry — call once at system start |
| `ipc_mqueue_register(type, dev_id, item_size, depth)` | Register a queue; returns `qid` (≥0) or negative `OS_ERR_*` |
| `ipc_mqueue_send_item(qid, data)` | Non-blocking send |
| `ipc_mqueue_receive_item(qid, data)` | Non-blocking receive |
| `ipc_mqueue_unregister(qid)` | Unregister and free resources |
| `ipc_get_mqueue_head()` | Return sentinel list head for iteration |

---

## Usage Examples

### Inter-task typed queue

```c
#include <ipc/mqueue.h>

/* System init */
ipc_mqueue_init();

/* Register a queue for 10 items of 4 bytes each */
int32_t qid = ipc_mqueue_register(IPC_MQUEUE_TYPE_PROC_TXN, -1, 4, 10);

/* Producer task */
uint32_t value = 42;
ipc_mqueue_send_item(qid, &value);

/* Consumer task */
uint32_t received;
ipc_mqueue_receive_item(qid, &received);

/* Cleanup */
ipc_mqueue_unregister(qid);
```

### UART byte-stream ring buffer

```c
/* Register a 512-byte ring buffer for UART 1 */
int32_t uart_qid = ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, UART_1, 1, 512);
```

The UART ISR writes bytes into this ring buffer via the board callback; the `uart_mgmt` service thread drains it.

### Iterate all registered queues

```c
type_message_queue_descriptor *pos;
list_for_each_entry(pos, ipc_get_mqueue_head(), list) {
    /* pos->mqueue.queue_id   — numeric ID              */
    /* pos->mqueue.queue_type — IPC_MQUEUE_TYPE_*       */
    /* pos->mqueue.item_size  — bytes per item          */
}
```

---

## Management Service Queues

Each peripheral management thread exposes its own FreeRTOS queue for command dispatch. These are higher-level than raw `ipc_mqueue` and are the recommended API for application code:

| Service | Header | Post function |
|---|---|---|
| UART | `<services/uart_mgmt.h>` | `uart_mgmt_async_transmit(dev_id, data, len)` |
| I2C | `<services/iic_mgmt.h>` | `iic_mgmt_sync_receive(...)` |
| SPI | `<services/spi_mgmt.h>` | — |
| GPIO | `<services/gpio_mgmt.h>` | `gpio_mgmt_post(gpio_id, cmd, state, delay_ms)` |

See [DEV_MGMT.md](DEV_MGMT.md) for full management service thread documentation.

---

## Ring Buffer

`ipc/ringbuffer.cpp` provides a byte-stream circular buffer used by UART ISRs:

- Based on the RT-Thread ring buffer implementation
- Thread-safe write from ISR context
- Drained by the `uart_mgmt` service thread
- Registered via `ipc_mqueue_register(IPC_MQUEUE_TYPE_UART_HW, ...)` — same registry as task queues

Include: `#include <ipc/ringbuffer.h>`
