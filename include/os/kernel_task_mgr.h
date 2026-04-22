/*
 * kernel_task_mgr.h — Public API for the kernel task health monitor
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Exposes the snapshot types and accessor used by
 * services/kernel_service_task_manager.c.  Consumers (e.g. the shell)
 * call task_mgr_get_health() to read the last periodic scan result.
 *
 * Thread safety note: the snapshot is refreshed every TASK_MGR_SCAN_PERIOD_MS
 * by the task-manager thread and is read without a lock by the shell.  On a
 * single-core Cortex-M4 under FreeRTOS this may produce a slightly torn read
 * if a scan occurs concurrently; for a debug/diagnostic interface this is
 * acceptable.
 */

#ifndef FREERTOS_OS_INCLUDE_OS_KERNEL_TASK_MGR_H_
#define FREERTOS_OS_INCLUDE_OS_KERNEL_TASK_MGR_H_

#include <stdint.h>
#include <stddef.h>
#include <FreeRTOS.h>
#include <task.h>

#define TASK_MGR_MAX_TASKS      32

/* Per-task snapshot */
typedef struct {
    TaskHandle_t    handle;
    const char     *name;
    eTaskState      state;
    UBaseType_t     stack_hwm;   /* words remaining — lower = closer to overflow */
    uint32_t        thread_id;
} task_health_t;

/* Full system health snapshot */
typedef struct {
    task_health_t   tasks[TASK_MGR_MAX_TASKS];
    uint8_t         task_count;

    task_health_t   timer_task;
    uint8_t         timer_task_valid;

    size_t          heap_free;
    size_t          heap_min_ever_free;

    uint32_t        stack_overflow_count;
    uint32_t        malloc_fail_count;
} sys_health_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns a pointer to the most recent health snapshot collected by the
 * task-manager background thread.  Never returns NULL.
 */
const sys_health_t *task_mgr_get_health(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_OS_KERNEL_TASK_MGR_H_ */
