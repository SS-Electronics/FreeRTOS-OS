# OS Thread API — FreeRTOS-OS

Threads are the primary execution unit. The OS wraps FreeRTOS tasks behind a Linux-inspired API and tracks all live threads in a global intrusive doubly-circular sentinel list.

---

## Table of Contents

- [Thread API](#thread-api)
- [thread_handle_t](#thread_handle_t)
- [Intrusive Linked List](#intrusive-linked-list)
  - [Key Operations](#key-operations)
- [Example — Creating Tasks](#example--creating-tasks)
- [Error Codes](#error-codes)

---

## Thread API

Include: `#include <os/kernel.h>`

| Function | Description |
|---|---|
| `os_thread_create(func, name, stack, prio, arg)` | Create a thread; returns positive `thread_id` or negative `OS_ERR_*` |
| `os_thread_delete(thread_id)` | Destroy thread and release resources |
| `os_thread_delay(ms)` | Block calling thread for `ms` milliseconds |
| `os_suspend_this_thread()` | Suspend calling thread indefinitely |
| `os_suspend_thread(thread_id)` | Suspend an arbitrary thread by ID |
| `os_resume_thread(thread_id)` | Resume a previously suspended thread |

```c
#include <os/kernel.h>

static void sensor_task(void *arg)
{
    for (;;) {
        /* do work */
        os_thread_delay(100);
    }
}

int app_main(void)
{
    /* returns positive thread_id on success, negative OS_ERR_* on failure */
    int32_t id = os_thread_create(sensor_task, "sensor", 512, 2, NULL);
    //                             ^entry       ^name    ^words ^prio ^arg

    os_suspend_thread(id);
    os_resume_thread(id);
    os_thread_delete(id);
    return 0;
}
```

---

## thread_handle_t

Every created thread is represented by a `thread_handle_t`. It embeds a `struct list_node` so it can live in the global `thread_list` sentinel without any extra allocation.

```c
typedef struct {
    struct list_node  list;           /* intrusive sentinel-list node       */
    uint32_t          thread_id;      /* monotonic unique ID                */
    uint32_t          priority;       /* FreeRTOS UBaseType_t priority      */
    TaskHandle_t      thread_handle;  /* underlying FreeRTOS task handle    */
    void             *init_parameter; /* opaque arg passed to entry         */
} thread_handle_t;
```

---

## Intrusive Linked List

`mm/list.h` provides a doubly-circular sentinel list — same design as Linux `include/linux/list.h`. Any struct becomes a list member by embedding `struct list_node`:

```c
struct list_node {
    struct list_node *next;
    struct list_node *prev;
};
```

The `container_of(ptr, type, member)` macro recovers the containing struct from the embedded member pointer — no wrapper pointers, no extra heap allocation per node.

### Key Operations

```c
LIST_NODE_HEAD(thread_list);                    /* declare sentinel         */

list_add_tail(&handle->list, &thread_list);     /* O(1) append              */
list_delete(&handle->list);                     /* O(1) unlink              */

/* type-safe iteration */
thread_handle_t *pos;
list_for_each_entry(pos, &thread_list, list) {
    /* pos is a valid thread_handle_t* */
}
```

---

## Example — Creating Tasks

```c
#include <os/kernel.h>
#include <services/gpio_mgmt.h>
#include <services/uart_mgmt.h>

#define HEARTBEAT_LED_ID  0
#define DEBUG_UART_ID     0

static void heartbeat_task(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_mgmt_post(HEARTBEAT_LED_ID, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        os_thread_delay(500);
    }
}

static void log_task(void *arg)
{
    (void)arg;
    static const uint8_t msg[] = "[OS] running\r\n";
    for (;;) {
        uart_mgmt_async_transmit(DEBUG_UART_ID, msg, sizeof(msg) - 1);
        os_thread_delay(2000);
    }
}

int app_main(void)
{
    os_thread_create(heartbeat_task, "heartbeat", 256, 2, NULL);
    os_thread_create(log_task,       "log",       512, 1, NULL);
    return 0;
}
```

---

## Error Codes

Defined in `include/def_err.h`:

| Code | Meaning |
|---|---|
| `OS_ERR_NONE` (0) | Success |
| `OS_ERR_OP` (negative) | Operation failed |
| `THEREAD_CREATE_FAILED` (-1) | Thread creation failed (stack or scheduler not started) |
