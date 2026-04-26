/*
 * shell_task_mgmt.c — CLI commands: heap, tasks
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * heap
 * ────
 *   Queries FreeRTOS heap APIs directly for live accuracy and pulls
 *   malloc_fail_count from the task-manager health snapshot.
 *
 *   Example output:
 *     Heap usage:
 *       Total  :  49152 B
 *       Used   :  14320 B
 *       Free   :  34832 B
 *       MinFree:  33104 B  (min ever free)
 *       Errors :  0 malloc failure(s)
 *
 * tasks
 * ─────
 *   Reads from the task-manager health snapshot (refreshed every 5 s).
 *   Uses FreeRTOS+CLI multi-shot output (returns pdTRUE per line) so the
 *   output is not limited by SHELL_OUT_BUF_LEN.
 *
 *   Example output:
 *     Tasks  (5 registered)
 *       ID  Name             State      StackHWM
 *       ----------------------------------------
 *        1  task_mgr         blocked    128 w
 *        2  uart_mgmt        blocked    256 w
 *       [T]  Tmr Svc         blocked    256 w  (timer daemon)
 *       Stack overflow events: 0
 */

#include <shell/shell_task_mgmt.h>
#include <os/kernel_task_mgr.h>
#include <os/kernel_syscall.h>

#include <FreeRTOS.h>
#include <task.h>
#include <FreeRTOS_CLI.h>

#include <stdio.h>
#include <string.h>

/* ── Forward declarations ─────────────────────────────────────────────────── */

static BaseType_t _cmd_heap_fn (char *out, size_t len, const char *in);
static BaseType_t _cmd_tasks_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_debug_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_help_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_uptime_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_reboot_fn(char *out, size_t len, const char *in);

/* ── CLI command descriptors ──────────────────────────────────────────────── */

static const CLI_Command_Definition_t _cmd_heap = {
    "heap",
    "heap\r\n"
    "  Show FreeRTOS heap: total, used, free, min-ever-free, malloc failures.\r\n",
    _cmd_heap_fn, 0
};

static const CLI_Command_Definition_t _cmd_tasks = {
    "tasks",
    "tasks\r\n"
    "  List all OS tasks with state and stack high-watermark (5 s snapshot).\r\n",
    _cmd_tasks_fn, 0
};

static const CLI_Command_Definition_t _cmd_debug = {
    "debug",
    "debug <en|dis>\r\n"
    "  Enable or disable printk debug output.\r\n",
    _cmd_debug_fn, 1
};

static const CLI_Command_Definition_t _cmd_help = {
    "help",
    "help\r\n  List all registered commands.\r\n",
    _cmd_help_fn, 0
};

static const CLI_Command_Definition_t _cmd_version = {
    "version",
    "version\r\n  Print firmware version and build date.\r\n",
    _cmd_version_fn, 0
};

static const CLI_Command_Definition_t _cmd_uptime = {
    "uptime",
    "uptime\r\n  Print system uptime in milliseconds.\r\n",
    _cmd_uptime_fn, 0
};

static const CLI_Command_Definition_t _cmd_reboot = {
    "reboot",
    "reboot\r\n  Trigger a software reset (NVIC_SystemReset).\r\n",
    _cmd_reboot_fn, 0
};

/* ── Helpers ──────────────────────────────────────────────────────────────── */

static const char * const _state_str[] = {
    "running", "ready", "blocked", "suspended", "deleted", "invalid"
};

static const char *_state_to_str(eTaskState s)
{
    if ((unsigned)s < (sizeof(_state_str) / sizeof(_state_str[0])))
        return _state_str[(unsigned)s];
    return "?";
}

/* ── heap command ─────────────────────────────────────────────────────────── */

static BaseType_t _cmd_heap_fn(char *out, size_t len, const char *in)
{
    (void)in;

    const sys_health_t *h     = task_mgr_get_health();
    size_t              total = (size_t)configTOTAL_HEAP_SIZE;
    size_t              free  = (size_t)xPortGetFreeHeapSize();
    size_t              used  = total - free;
    size_t              minf  = (size_t)xPortGetMinimumEverFreeHeapSize();
    uint32_t            faults = h ? h->malloc_fail_count : 0U;

    snprintf(out, len,
             "Heap usage:\r\n"
             "  Total  : %6u B\r\n"
             "  Used   : %6u B\r\n"
             "  Free   : %6u B\r\n"
             "  MinFree: %6u B  (min ever free)\r\n"
             "  Errors : %lu malloc failure(s)\r\n",
             (unsigned)total,
             (unsigned)used,
             (unsigned)free,
             (unsigned)minf,
             (unsigned long)faults);

    return pdFALSE;
}

/* ── tasks command (multi-shot) ───────────────────────────────────────────── */

/*
 * Multi-shot state: -1 = header not sent yet, 0..N = next task line index,
 * TASK_MGR_MAX_TASKS + 1 = timer-daemon line, TASK_MGR_MAX_TASKS + 2 = summary.
 * Reset to -1 after the final pdFALSE return so a repeated 'tasks' command
 * starts fresh.
 */
static int16_t _tasks_line = -1;

static BaseType_t _cmd_tasks_fn(char *out, size_t len, const char *in)
{
    (void)in;

    const sys_health_t *h = task_mgr_get_health();

    if (h == NULL)
    {
        _tasks_line = -1;
        snprintf(out, len, "Task manager not available.\r\n");
        return pdFALSE;
    }

    /* ── Header ── */
    if (_tasks_line == -1)
    {
        snprintf(out, len,
                 "Tasks  (%u registered)\r\n"
                 "  ID  %-16s %-10s StackHWM\r\n"
                 "  ----------------------------------------\r\n",
                 (unsigned)h->task_count,
                 "Name", "State");
        _tasks_line = 0;
        return pdTRUE;
    }

    /* ── Per-task lines ── */
    if (_tasks_line < (int16_t)h->task_count)
    {
        const task_health_t *t = &h->tasks[_tasks_line];
        snprintf(out, len,
                 "  %2lu  %-16s %-10s %u w\r\n",
                 (unsigned long)t->thread_id,
                 t->name ? t->name : "?",
                 _state_to_str(t->state),
                 (unsigned)t->stack_hwm);
        _tasks_line++;
        return pdTRUE;
    }

    /* ── Timer-daemon line ── */
    if (_tasks_line == (int16_t)h->task_count && h->timer_task_valid)
    {
        const task_health_t *t = &h->timer_task;
        snprintf(out, len,
                 "  [T] %-16s %-10s %u w  (timer daemon)\r\n",
                 t->name ? t->name : "Tmr Svc",
                 _state_to_str(t->state),
                 (unsigned)t->stack_hwm);
        _tasks_line++;
        return pdTRUE;
    }

    /* ── Summary / final line ── */
    snprintf(out, len,
             "  Stack overflow events: %lu\r\n",
             (unsigned long)h->stack_overflow_count);
    _tasks_line = -1;
    return pdFALSE;
}

/* ── debug command ────────────────────────────────────────────────────────── */

static BaseType_t _cmd_debug_fn(char *out, size_t len, const char *in)
{
    BaseType_t plen;
    const char *param = FreeRTOS_CLIGetParameter(in, 1, &plen);

    if (param == NULL)
    {
        snprintf(out, len, "Usage: debug <en|dis>\r\n");
        return pdFALSE;
    }

    if (plen == 2 && strncmp(param, "en", 2) == 0)
    {
        printk_enable();
        snprintf(out, len, "Debug output enabled.\r\n");
    }
    else if (plen == 3 && strncmp(param, "dis", 3) == 0)
    {
        printk_disable();
        snprintf(out, len, "Debug output disabled.\r\n");
    }
    else
    {
        snprintf(out, len, "Unknown option '%.*s'. Usage: debug <en|dis>\r\n",
                 (int)plen, param);
    }

    return pdFALSE;
}

/* ── Built-in command implementations ───────────────────────────────────── */
__SECTION_OS __USED
static BaseType_t _cmd_help_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len,
             "Use FreeRTOS+CLI registered commands:\r\n"
             "Type any command and press Enter.\r\n");
    return pdFALSE;
}

__SECTION_OS __USED
static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len,
             "FreeRTOS-OS v1.0 built %s %s\r\n",
             __DATE__, __TIME__);
    return pdFALSE;
}

__SECTION_OS __USED
static BaseType_t _cmd_uptime_fn(char *out, size_t len, const char *in)
{
    (void)in;
    uint32_t ms = drv_time_get_ticks();
    snprintf(out, len,
             "Uptime: %lu ms (%lu s)\r\n",
             (unsigned long)ms, (unsigned long)(ms / 1000UL));
    return pdFALSE;
}

__SECTION_OS __USED
static BaseType_t _cmd_reboot_fn(char *out, size_t len, const char *in)
{
    (void)in;
    // snprintf(out, len, "Rebooting...\r\n");
    // _shell_write(out, (uint16_t)strlen(out));
    vTaskDelay(pdMS_TO_TICKS(50));
    NVIC_SystemReset();
    return pdFALSE;
}


/* ── Registration ─────────────────────────────────────────────────────────── */

void shell_task_mgmt_register_cmds(void)
{

    FreeRTOS_CLIRegisterCommand(&_cmd_help);
    FreeRTOS_CLIRegisterCommand(&_cmd_version);
    FreeRTOS_CLIRegisterCommand(&_cmd_uptime);
    FreeRTOS_CLIRegisterCommand(&_cmd_reboot);
    FreeRTOS_CLIRegisterCommand(&_cmd_heap);
    FreeRTOS_CLIRegisterCommand(&_cmd_tasks);
    FreeRTOS_CLIRegisterCommand(&_cmd_debug);
}
