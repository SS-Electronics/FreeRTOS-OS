# OS Thread API — FreeRTOS-OS {#os-thread-api--freertos-os}

Threads are the primary execution unit. The OS wraps FreeRTOS tasks behind a Linux-inspired API and tracks all live threads in a global intrusive doubly-circular sentinel list.

---

## Table of Contents {#table-of-contents}

- [Thread Lifecycle](#thread-lifecycle)
- [Thread Map at Steady State](#thread-map-at-steady-state)
- [Thread API](#thread-api)
- [thread_handle_t](#thread_handle_t)
- [Intrusive Linked List](#intrusive-linked-list)
  - [Key Operations](#key-operations)
- [Example — Creating Tasks](#example--creating-tasks)
- [Error Codes](#error-codes)

---

## Thread Lifecycle {#thread-lifecycle}

Every task created by `os_thread_create()` follows the standard
FreeRTOS task lifecycle.  Two project-specific touchpoints are layered
on top:

1. The thread is added to the intrusive list (`thread_list`) so
   `task_mgr_start`'s monitor can enumerate it without any extra
   allocation.
2. If the application opted into the software watchdog, the thread
   calls `wdog_task_register(self)` once and then
   `wdog_task_checkin()` on every loop — failure to do so within the
   per-task budget will trigger `safety_enter_safe_state()`.

\dot
digraph thread_states {
    rankdir=LR;
    node [shape=ellipse, style=filled, fontname="Helvetica", fontsize=10];

    NX  [label="non-existent",                          fillcolor="#ECEFF1"];
    RDY [label="Ready",                                 fillcolor="#B3E5FC"];
    RUN [label="Running",                               fillcolor="#A5D6A7"];
    BLK [label="Blocked\nqueue / notify / delay",       fillcolor="#FFE082"];
    SUS [label="Suspended\nvTaskSuspend",               fillcolor="#CFD8DC"];
    DEL [label="Deleted",                               fillcolor="#FFCDD2"];

    NX  -> RDY [label="os_thread_create"];
    RDY -> RUN [label="scheduler picks\n(by priority)"];
    RUN -> RDY [label="preempted / tick"];
    RUN -> BLK [label="xQueueReceive\nulTaskNotifyTake\nos_thread_delay"];
    BLK -> RDY [label="event /\ntimeout"];
    RUN -> SUS [label="os_suspend_thread"];
    SUS -> RDY [label="os_resume_thread"];
    RUN -> DEL [label="os_thread_delete"];
}
\enddot

A version of this diagram with the full thread roster (kernel + OS
service tasks + application tasks, their priorities, and the queues
they consume) is in
[**DIAGRAMS.md § Thread Map at Steady State**](DIAGRAMS.md#thread-map-at-steady-state).

---

## Thread Map at Steady State {#thread-map-at-steady-state}

A typical build creates the following tasks once `vTaskStartScheduler`
has started:

| Task | Priority | Stack | Source | Purpose |
|---|---|---|---|---|
| `IDLE` | 0 | configMINIMAL_STACK_SIZE | FreeRTOS kernel | runs when nothing else is ready; reclaims deleted-task memory |
| `Tmr Svc` | `configMAX_PRIORITIES-1` | timer task stack | FreeRTOS kernel | software-timer callbacks |
| `WDOG` | 4 | `PROC_SERVICE_WDOG_STACK_SIZE` | [`safety/wdog.c`](../safety/wdog.c) | software watchdog; refreshes IWDG |
| `GPIO_MGMT` | 1 | `PROC_SERVICE_GPIO_MGMT_STACK_SIZE` | [`services/gpio_mgmt.c`](../services/gpio_mgmt.c) | GPIO commands via `_mgmt_queue` |
| `uart_mgmt` | 1 | `PROC_SERVICE_SERIAL_MGMT_STACK_SIZE` | [`services/uart_mgmt.c`](../services/uart_mgmt.c) | UART TX/RX serialisation |
| `iic_mgmt` | 1 | `PROC_SERVICE_IIC_MGMT_STACK_SIZE` | [`services/iic_mgmt.c`](../services/iic_mgmt.c) | I²C bus serialisation |
| `spi_mgmt` | 1 | `PROC_SERVICE_SPI_MGMT_STACK_SIZE` | [`services/spi_mgmt.c`](../services/spi_mgmt.c) | SPI bus serialisation |
| `ADC_MGMT` | 1 | `ADC_MGMT_STACK` | [`services/adc_mgmt.c`](../services/adc_mgmt.c) | per-channel ADC sample queue |
| `MGMT_TASK` | 1 | `MGMT_TASK_STACK` | [`services/kernel_service_task_manager.c`](../services/kernel_service_task_manager.c) | task health monitor + WDOG check-in |
| `OS_SHELL` | 1 | `OS_SHELL_MGMT_STACK` | [`services/os_shell_management.c`](../services/os_shell_management.c) | FreeRTOS-Plus-CLI command loop |
| user tasks | user-defined | user-defined | `examples/<target>/app_main.c` or user `app/app_main.c` | application logic |

Priority constants live in [`config/conf_os.h`](../config/conf_os.h)
(`PROC_SERVICE_*_PRIORITY`).  Staggered startup offsets
(`TIME_OFFSET_*_MANAGEMENT`) ensure peripherals come up in clock-tree
order — GPIO first, then UART, then SPI, then I²C — so a manager that
needs another manager (e.g. shell needs UART) finds it ready.

---

## Thread API {#thread-api}

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

## thread_handle_t {#thread_handle_t}

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

## Intrusive Linked List {#intrusive-linked-list}

`mm/list.h` provides a doubly-circular sentinel list — same design as Linux `include/linux/list.h`. Any struct becomes a list member by embedding `struct list_node`:

```c
struct list_node {
    struct list_node *next;
    struct list_node *prev;
};
```

The `container_of(ptr, type, member)` macro recovers the containing struct from the embedded member pointer — no wrapper pointers, no extra heap allocation per node.

### Key Operations {#key-operations}

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

## Example — Creating Tasks {#example--creating-tasks}

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

## Error Codes {#error-codes}

Defined in `include/def_err.h`:

| Code | Meaning |
|---|---|
| `OS_ERR_NONE` (0) | Success |
| `OS_ERR_OP` (negative) | Operation failed |
| `THEREAD_CREATE_FAILED` (-1) | Thread creation failed (stack or scheduler not started) |
