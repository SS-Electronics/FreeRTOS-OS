/*
 * kernel_service_task_manager.c — Background task health monitor
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Responsibilities
 * ────────────────
 *  1. Periodic health snapshot: walks thread_list and collects per-task
 *     state + stack high-watermark via FreeRTOS trace APIs.
 *  2. Timer-daemon monitoring: queries the FreeRTOS timer task handle.
 *  3. Heap health: free / minimum-ever heap tracked each scan cycle.
 *  4. FreeRTOS application hooks:
 *       vApplicationStackOverflowHook  — configCHECK_FOR_STACK_OVERFLOW 2
 *       vApplicationIdleHook           — configUSE_IDLE_HOOK 1
 *       vApplicationMallocFailedHook   — configUSE_MALLOC_FAILED_HOOK 1
 *
 * Configuration requirements in FreeRTOSConfig.h
 * ───────────────────────────────────────────────
 *   configUSE_IDLE_HOOK                 1
 *   configCHECK_FOR_STACK_OVERFLOW      2
 *   configUSE_MALLOC_FAILED_HOOK        1
 *   configUSE_TRACE_FACILITY            1   (already set)
 *   INCLUDE_uxTaskGetStackHighWaterMark 1   (already set)
 *   INCLUDE_eTaskGetState               1   (already set)
 */

#include <os/kernel_services.h>
#include <os/kernel_task_mgr.h>
#include <os/kernel.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#define TASK_MGR_SCAN_PERIOD_MS 5000

/* ── Module state ─────────────────────────────────────────────────────────── */

static sys_health_t _health;

/* ── Hook counters (written from hook context) ────────────────────────────── */

static volatile uint32_t _stack_overflow_count = 0;
static volatile uint32_t _malloc_fail_count    = 0;
static volatile uint32_t _idle_tick_count      = 0;

/* ── Internal: collect one task's metrics ─────────────────────────────────── */

static void _collect_task(task_health_t *out, TaskHandle_t h, uint32_t tid)
{
    out->handle    = h;
    out->name      = pcTaskGetName(h);
    out->state     = eTaskGetState(h);
    out->stack_hwm = uxTaskGetStackHighWaterMark(h);
    out->thread_id = tid;
}

/* ── Internal: scan the entire thread_list ────────────────────────────────── */

static void _scan_tasks(void)
{
    _health.task_count = 0;

    thread_handle_t *pos;
    list_for_each_entry(pos, os_thread_list_get(), list)
    {
        if (_health.task_count >= TASK_MGR_MAX_TASKS)
            break;

        _collect_task(&_health.tasks[_health.task_count],
                      pos->thread_handle,
                      pos->thread_id);
        _health.task_count++;
    }
}

/* ── Internal: collect timer-daemon metrics ───────────────────────────────── */

static void _scan_timer_task(void)
{
#if (configUSE_TIMERS == 1)
    TaskHandle_t h = xTimerGetTimerDaemonTaskHandle();
    if (h != NULL)
    {
        _collect_task(&_health.timer_task, h, 0);
        _health.timer_task_valid = 1;
    }
    else
#endif
    {
        _health.timer_task_valid = 0;
    }
}

/* ── Internal: heap snapshot ──────────────────────────────────────────────── */

static void _scan_heap(void)
{
    _health.heap_free          = xPortGetFreeHeapSize();
    _health.heap_min_ever_free = xPortGetMinimumEverFreeHeapSize();
}

/* ── Public: query API ────────────────────────────────────────────────────── */

const sys_health_t *task_mgr_get_health(void)
{
    return &_health;
}

/* ── FreeRTOS application hooks ───────────────────────────────────────────── */

/*
 * Called by the idle task each time it runs (configUSE_IDLE_HOOK 1).
 * Must never attempt to block or call any API that might block.
 */
void vApplicationIdleHook(void)
{
    _idle_tick_count++;
}

/*
 * Called when a task overflows its stack (configCHECK_FOR_STACK_OVERFLOW 2).
 * Spin forever — the application is in an unrecoverable state.
 * pxCurrentTCB / pcTaskName identify the offending task for a debugger.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    _stack_overflow_count++;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

/*
 * Called when pvPortMalloc() fails (configUSE_MALLOC_FAILED_HOOK 1).
 * Increment counter so the health task can report it; do not block.
 */
void vApplicationMallocFailedHook(void)
{
    _malloc_fail_count++;
}

/* ── Background monitor task ──────────────────────────────────────────────── */

void thread_task_mgmt(void *arg)
{
    (void)arg;

    /* One-shot startup delay so all service threads have registered first */
    vTaskDelay(pdMS_TO_TICKS(TASK_MGR_SCAN_PERIOD_MS));

    while (1)
    {
        _scan_tasks();
        _scan_timer_task();
        _scan_heap();

        _health.stack_overflow_count = _stack_overflow_count;
        _health.malloc_fail_count    = _malloc_fail_count;

        vTaskDelay(pdMS_TO_TICKS(TASK_MGR_SCAN_PERIOD_MS));
    }
}
