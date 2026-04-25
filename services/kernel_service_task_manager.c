/**
 * @file    kernel_service_task_manager.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  kernel
 * @brief   Background task health monitor and system diagnostics service
 *
 * @details
 * This module implements a periodic system health monitoring service for the
 * FreeRTOS-OS project. It runs as a dedicated kernel thread responsible for
 * collecting runtime diagnostics from the RTOS scheduler, application tasks,
 * and heap allocator.
 *
 * The service provides a centralized view of system health, including task
 * states, stack usage, timer daemon status, and memory statistics.
 *
 * ---------------------------------------------------------------------------
 * Responsibilities
 * ---------------------------------------------------------------------------
 *
 * 1. Task health snapshot
 *    - Iterates over system thread list (thread_list)
 *    - Collects per-task metadata using FreeRTOS APIs:
 *        • Task state (running, blocked, ready, etc.)
 *        • Stack high-water mark
 *        • Task handle and ID mapping
 *
 * 2. Timer daemon monitoring
 *    - Queries FreeRTOS timer service task handle
 *    - Records validity and stack usage
 *
 * 3. Heap monitoring
 *    - Tracks current free heap size
 *    - Tracks minimum-ever recorded heap usage
 *
 * 4. FreeRTOS application hooks integration
 *    - Stack overflow detection
 *    - malloc failure tracking
 *    - Idle task execution tracking
 *    - Static memory provision for idle/timer tasks
 *
 * ---------------------------------------------------------------------------
 * System Scan Flow
 * ---------------------------------------------------------------------------
 *
 * thread_task_mgmt()
 *      ↓
 * Startup delay (system stabilization)
 *      ↓
 * Periodic scan loop (TASK_MGR_SCAN_PERIOD_MS)
 *      ↓
 * ┌──────────────────────────────────────────────┐
 * │ _scan_tasks()                               │
 * │  - Iterate thread_list                      │
 * │  - Collect task state + stack watermark     │
 * └──────────────────────────────────────────────┘
 *      ↓
 * ┌──────────────────────────────────────────────┐
 * │ _scan_timer_task()                          │
 * │  - Query timer daemon task                  │
 * └──────────────────────────────────────────────┘
 *      ↓
 * ┌──────────────────────────────────────────────┐
 * │ _scan_heap()                                │
 * │  - xPortGetFreeHeapSize()                   │
 * │  - xPortGetMinimumEverFreeHeapSize()        │
 * └──────────────────────────────────────────────┘
 *      ↓
 * Update sys_health_t snapshot
 *
 * ---------------------------------------------------------------------------
 * FreeRTOS Configuration Requirements
 * ---------------------------------------------------------------------------
 *
 * configUSE_IDLE_HOOK                 = 1
 * configCHECK_FOR_STACK_OVERFLOW      = 2
 * configUSE_MALLOC_FAILED_HOOK        = 1
 * configUSE_TRACE_FACILITY            = 1
 * INCLUDE_uxTaskGetStackHighWaterMark = 1
 * INCLUDE_eTaskGetState               = 1
 *
 * ---------------------------------------------------------------------------
 * Hooks Behavior
 * ---------------------------------------------------------------------------
 *
 * vApplicationIdleHook()
 *  - Increment idle tick counter
 *  - Must never block or call blocking APIs
 *
 * vApplicationStackOverflowHook()
 *  - Triggered on stack overflow
 *  - System enters safe infinite loop after logging event
 *
 * vApplicationMallocFailedHook()
 *  - Triggered on heap allocation failure
 *  - Only increments failure counter (non-blocking)
 *
 * ---------------------------------------------------------------------------
 * Dependencies
 * ---------------------------------------------------------------------------
 * os/kernel_services.h
 * os/kernel_task_mgr.h
 * os/kernel.h
 * FreeRTOS.h
 * task.h
 * timers.h
 *
 * ---------------------------------------------------------------------------
 * @note
 * - This module runs as a background system service task
 * - Designed for diagnostic and observability purposes
 * - Not intended for real-time critical execution paths
 *
 * @warning
 * - Hook functions execute in restricted RTOS contexts
 * - Stack overflow hook halts system execution permanently
 * - Heap stats reflect allocator state at scan time only
 *
 * ---------------------------------------------------------------------------
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <os/kernel_services.h>
#include <os/kernel_task_mgr.h>
#include <os/kernel.h>


/* ── Module state ─────────────────────────────────────────────────────────── */
__SECTION_OS_DATA __USED
static sys_health_t _health;

/* ── Hook counters (written from hook context) ────────────────────────────── */
__SECTION_OS_DATA __USED
static volatile uint32_t _stack_overflow_count = 0;

__SECTION_OS_DATA __USED
static volatile uint32_t _malloc_fail_count    = 0;

__SECTION_OS_DATA __USED
static volatile uint32_t _idle_tick_count      = 0;

/* ── Internal: collect one task's metrics ─────────────────────────────────── */
__SECTION_OS __USED
static void _collect_task(task_health_t *out, TaskHandle_t h, uint32_t tid)
{
    out->handle    = h;
    out->name      = pcTaskGetName(h);
    out->state     = eTaskGetState(h);
    out->stack_hwm = uxTaskGetStackHighWaterMark(h);
    out->thread_id = tid;
}

/* ── Internal: scan the entire thread_list ────────────────────────────────── */
__SECTION_OS __USED
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
__SECTION_OS __USED
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
__SECTION_OS __USED
static void _scan_heap(void)
{
    _health.heap_free          = xPortGetFreeHeapSize();
    _health.heap_min_ever_free = xPortGetMinimumEverFreeHeapSize();
}

/* ── Public: query API ────────────────────────────────────────────────────── */
__SECTION_OS __USED
const sys_health_t *task_mgr_get_health(void)
{
    return &_health;
}

/* ── FreeRTOS application hooks ───────────────────────────────────────────── */
__SECTION_OS __USED
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    static StaticTask_t _idle_tcb;
    static StackType_t  _idle_stack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &_idle_tcb;
    *ppxIdleTaskStackBuffer = _idle_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

__SECTION_OS __USED
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    static StaticTask_t _timer_tcb;
    static StackType_t  _timer_stack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer   = &_timer_tcb;
    *ppxTimerTaskStackBuffer = _timer_stack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

__SECTION_OS __USED
void vApplicationIdleHook(void)
{
    _idle_tick_count++;
}

__SECTION_OS __USED
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    _stack_overflow_count++;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

__SECTION_OS __USED
void vApplicationMallocFailedHook(void)
{
    _malloc_fail_count++;
}

/* ── Service start ────────────────────────────────────────────────────────── */
__SECTION_OS __USED
int32_t task_mgr_start(void)
{
    if (os_thread_create(thread_task_mgmt, "MGMT_TASK",
                         PROC_SERVICE_TASK_MGMT_STACK_SIZE,
                         PROC_SERVICE_TASK_MGMT_PRIORITY, NULL) < 0)
        return OS_ERR_OP;

    return OS_ERR_NONE;
}

/* ── Background monitor task ──────────────────────────────────────────────── */
__SECTION_OS __USED
void thread_task_mgmt(void *arg)
{
    (void)arg;

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