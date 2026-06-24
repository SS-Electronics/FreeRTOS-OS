/**
 * @file        shell_task_mgmt.c
 * @brief       shell_task_mgmt.c — OS + Linux-style CLI commands
 * @ingroup     shell
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Shell
 * @info        Interactive shell over UART_APP using FreeRTOS-Plus-CLI; registers OS introspection commands.
 * @dependency  FreeRTOS-Plus-CLI, UART mgmt service
 *
 * @details
 * shell_task_mgmt.c — CLI commands: heap, tasks, iic-scan plus Linux-style
 * commands (echo, clear, uname, ps, free, whoami, hostname).
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
 * iic-scan <bus_id>
 * ────────────────
 *   Probes the full I2C address range (0x00–0xFF) on the given bus.
 *   Uses iic_sync_probe() with a 10 ms per-address timeout.
 *
 *   Example output:
 *     I2C bus 0 scan (0x00-0xFF):
 *       0x19  ACK  LSM303DLHC ACC
 *       0x1E  ACK  LSM303DLHC MAG
 *     Found: 2 device(s) of 112 probed.
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
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <shell/shell_task_mgmt.h>
#include <os/kernel_task_mgr.h>
#include <os/kernel_syscall.h>
#if defined(CONFIG_INC_SERVICE_IIC_MGMT) && (CONFIG_INC_SERVICE_IIC_MGMT == 1)
#include <drv_app/iic_app.h>
#endif
#include <board/board_config.h>
#include <autoconf.h>
#if defined(CONFIG_NET)
#include <net/net_ping.h>
#endif

#include <FreeRTOS.h>
#include <task.h>
#include <FreeRTOS_CLI.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* ── uname / hostname identity ────────────────────────────────────────────── */

#define UNAME_SYSNAME   "FreeRTOS-OS"
#define UNAME_NODENAME  "freertos-os"
#define UNAME_RELEASE   "1.0"
#define UNAME_MACHINE   "STM32H723ZG"
#define UNAME_PROC      "Cortex-M7"

/* ── Forward declarations ─────────────────────────────────────────────────── */

static BaseType_t _cmd_heap_fn    (char *out, size_t len, const char *in);
static BaseType_t _cmd_tasks_fn   (char *out, size_t len, const char *in);
static BaseType_t _cmd_debug_fn   (char *out, size_t len, const char *in);
static BaseType_t _cmd_help_fn    (char *out, size_t len, const char *in);
static BaseType_t _cmd_version_fn (char *out, size_t len, const char *in);
static BaseType_t _cmd_uptime_fn  (char *out, size_t len, const char *in);
static BaseType_t _cmd_reboot_fn  (char *out, size_t len, const char *in);
#if defined(CONFIG_INC_SERVICE_IIC_MGMT) && (CONFIG_INC_SERVICE_IIC_MGMT == 1)
static BaseType_t _cmd_iic_scan_fn(char *out, size_t len, const char *in);
#endif

/* Linux-style commands. */
static BaseType_t _cmd_echo_fn    (char *out, size_t len, const char *in);
static BaseType_t _cmd_clear_fn   (char *out, size_t len, const char *in);
static BaseType_t _cmd_uname_fn   (char *out, size_t len, const char *in);
static BaseType_t _cmd_ps_fn      (char *out, size_t len, const char *in);
static BaseType_t _cmd_free_fn    (char *out, size_t len, const char *in);
static BaseType_t _cmd_whoami_fn  (char *out, size_t len, const char *in);
static BaseType_t _cmd_hostname_fn(char *out, size_t len, const char *in);
#if defined(CONFIG_NET)
static BaseType_t _cmd_ping_fn    (char *out, size_t len, const char *in);
#endif

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

#if defined(CONFIG_INC_SERVICE_IIC_MGMT) && (CONFIG_INC_SERVICE_IIC_MGMT == 1)
static const CLI_Command_Definition_t _cmd_iic_scan = {
    "iic-scan",
    "iic-scan <bus_id>\r\n"
    "  Probe full I2C address range (0x00-0xFF) on the given bus.\r\n",
    _cmd_iic_scan_fn, 1
};
#endif

/* ── Linux-style command descriptors ──────────────────────────────────────── */

static const CLI_Command_Definition_t _cmd_echo = {
    "echo",
    "echo [-n] [text...]\r\n"
    "  Write the arguments to the console. -n omits the trailing newline.\r\n",
    _cmd_echo_fn, -1
};

static const CLI_Command_Definition_t _cmd_clear = {
    "clear",
    "clear\r\n  Clear the terminal screen.\r\n",
    _cmd_clear_fn, 0
};

static const CLI_Command_Definition_t _cmd_uname = {
    "uname",
    "uname [-a|-s|-n|-r|-m]\r\n"
    "  Print system information (sysname/nodename/release/machine).\r\n",
    _cmd_uname_fn, -1
};

static const CLI_Command_Definition_t _cmd_ps = {
    "ps",
    "ps\r\n  Report task status, Linux ps style (PID STAT STACK COMMAND).\r\n",
    _cmd_ps_fn, 0
};

static const CLI_Command_Definition_t _cmd_free = {
    "free",
    "free\r\n  Display heap memory usage (total/used/free/min-free) in bytes.\r\n",
    _cmd_free_fn, 0
};

static const CLI_Command_Definition_t _cmd_whoami = {
    "whoami",
    "whoami\r\n  Print the effective user name.\r\n",
    _cmd_whoami_fn, 0
};

static const CLI_Command_Definition_t _cmd_hostname = {
    "hostname",
    "hostname\r\n  Print the system host name.\r\n",
    _cmd_hostname_fn, 0
};

#if defined(CONFIG_NET)
static const CLI_Command_Definition_t _cmd_ping = {
    "ping",
    "ping <ip> [count]\r\n"
    "  Send ICMP echo requests to an IPv4 host (default 4 packets).\r\n",
    _cmd_ping_fn, -1
};
#endif

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

/* Map a FreeRTOS task state to a Linux ps STAT letter. */
static char _state_to_linux(eTaskState s)
{
    switch (s)
    {
        case eRunning:   return 'R';   /* running                */
        case eReady:     return 'R';   /* runnable               */
        case eBlocked:   return 'S';   /* interruptible sleep    */
        case eSuspended: return 'T';   /* stopped                */
        case eDeleted:   return 'Z';   /* zombie / being deleted */
        default:         return '?';
    }
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

        vTaskDelay(pdMS_TO_TICKS(10));

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
__SECTION_OS
static BaseType_t _cmd_help_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len,
             "Use FreeRTOS+CLI registered commands:\r\n"
             "Type any command and press Enter.\r\n");
    return pdFALSE;
}

__SECTION_OS
static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len,
             "FreeRTOS-OS v1.0 built %s %s\r\n",
             __DATE__, __TIME__);
    return pdFALSE;
}

__SECTION_OS
static BaseType_t _cmd_uptime_fn(char *out, size_t len, const char *in)
{
    (void)in;
    uint32_t ms = drv_time_get_ticks();
    snprintf(out, len,
             "Uptime: %lu ms (%lu s)\r\n",
             (unsigned long)ms, (unsigned long)(ms / 1000UL));
    return pdFALSE;
}

__SECTION_OS
static BaseType_t _cmd_reboot_fn(char *out, size_t len, const char *in)
{
    (void)in;
    // snprintf(out, len, "Rebooting...\r\n");
    // _shell_write(out, (uint16_t)strlen(out));
    vTaskDelay(pdMS_TO_TICKS(50));
    NVIC_SystemReset();
    return pdFALSE;
}


/* ── iic-scan command ─────────────────────────────────────────────────────── */

#if defined(CONFIG_INC_SERVICE_IIC_MGMT) && (CONFIG_INC_SERVICE_IIC_MGMT == 1)

/*
 * Multi-shot state:
 *   -1           first call — parse arg, run scan, print header
 *   0..count-1   print one result line per shot
 *   count        print summary line, reset to -1
 */
#define _IIC_SCAN_MAX  128U

static int16_t _scan_state  = -1;
static uint8_t _scan_bus_id =  0;
static uint8_t _scan_found[_IIC_SCAN_MAX];
static uint8_t _scan_count  =  0;

static BaseType_t _cmd_iic_scan_fn(char *out, size_t len, const char *in)
{
    /* ── First call: parse argument, run full scan, emit header ── */
    if (_scan_state == -1)
    {
        BaseType_t  plen;
        const char *arg = FreeRTOS_CLIGetParameter(in, 1, &plen);

        if (arg == NULL || plen == 0)
        {
            snprintf(out, len, "Usage: iic-scan <bus_id>\r\n");
            return pdFALSE;
        }

        /* Parse single-digit bus ID */
        uint8_t bus = (uint8_t)(arg[0] - '0');

        if (arg[0] < '0' || arg[0] > '9' || bus >= BOARD_IIC_COUNT)
        {
            snprintf(out, len,
                     "Error: bus %u not available (BOARD_IIC_COUNT=%u)\r\n",
                     (unsigned)bus, (unsigned)BOARD_IIC_COUNT);
            return pdFALSE;
        }

        _scan_bus_id = bus;
        _scan_count  = 0;

        /* Probe full byte address range — 10 ms timeout per address */
        for (uint16_t addr = 0x00U; addr <= 0xFFU; addr++)
        {
            if (iic_sync_probe(_scan_bus_id, addr, 10U) == OS_ERR_NONE)
            {
                if (_scan_count < _IIC_SCAN_MAX)
                    _scan_found[_scan_count++] = (uint8_t)addr;
            }
        }

        snprintf(out, len, "I2C bus %u scan (0x00-0xFF):\r\n",
                 (unsigned)_scan_bus_id);

        if (_scan_count == 0)
        {
            strncat(out, "  No devices found.\r\n", len - strlen(out) - 1U);
            return pdFALSE;
        }

        _scan_state = 0;
        return pdTRUE;
    }

    /* ── One result line per shot ── */
    if (_scan_state < (int16_t)_scan_count)
    {
        snprintf(out, len, "  0x%02X  ACK\r\n",
                 (unsigned)_scan_found[_scan_state]);
        _scan_state++;
        return pdTRUE;
    }

    /* ── Summary (final shot) ── */
    snprintf(out, len, "Found: %u device(s) of 256 probed.\r\n",
             (unsigned)_scan_count);
    _scan_state = -1;
    return pdFALSE;
}

#endif /* CONFIG_INC_SERVICE_IIC_MGMT */

/* ── echo command ─────────────────────────────────────────────────────────── */

__SECTION_OS
static BaseType_t _cmd_echo_fn(char *out, size_t len, const char *in)
{
    BaseType_t  plen;
    UBaseType_t idx        = 1;
    bool        no_newline = false;
    size_t      pos        = 0;

    out[0] = '\0';

    const char *p = FreeRTOS_CLIGetParameter(in, idx, &plen);

    /* A leading -n suppresses the trailing newline, like /bin/echo. */
    if (p != NULL && plen == 2 && strncmp(p, "-n", 2) == 0)
    {
        no_newline = true;
        idx++;
        p = FreeRTOS_CLIGetParameter(in, idx, &plen);
    }

    /* Join the remaining parameters with single spaces. */
    while (p != NULL && pos < (len - 1U))
    {
        if (pos != 0U)
        {
            out[pos++] = ' ';
        }

        int n = snprintf(out + pos, len - pos, "%.*s", (int)plen, p);
        if (n < 0)
        {
            break;
        }
        pos += ((size_t)n < (len - pos)) ? (size_t)n : (len - pos - 1U);

        idx++;
        p = FreeRTOS_CLIGetParameter(in, idx, &plen);
    }

    if (!no_newline && pos < (len - 2U))
    {
        out[pos++] = '\r';
        out[pos++] = '\n';
    }
    out[pos] = '\0';

    return pdFALSE;
}

/* ── clear command ────────────────────────────────────────────────────────── */

__SECTION_OS
static BaseType_t _cmd_clear_fn(char *out, size_t len, const char *in)
{
    (void)in;
    /* ANSI: erase screen + home cursor. */
    snprintf(out, len, "\033[2J\033[H");
    return pdFALSE;
}

/* ── uname command ────────────────────────────────────────────────────────── */

__SECTION_OS
static BaseType_t _cmd_uname_fn(char *out, size_t len, const char *in)
{
    BaseType_t  plen;
    const char *p = FreeRTOS_CLIGetParameter(in, 1, &plen);

    if (p == NULL)
    {
        snprintf(out, len, "%s\r\n", UNAME_SYSNAME);
        return pdFALSE;
    }

    if (plen == 2 && strncmp(p, "-a", 2) == 0)
    {
        snprintf(out, len, "%s %s %s %s %s\r\n",
                 UNAME_SYSNAME, UNAME_NODENAME, UNAME_RELEASE,
                 UNAME_MACHINE, UNAME_PROC);
    }
    else if (plen == 2 && strncmp(p, "-s", 2) == 0)
    {
        snprintf(out, len, "%s\r\n", UNAME_SYSNAME);
    }
    else if (plen == 2 && strncmp(p, "-n", 2) == 0)
    {
        snprintf(out, len, "%s\r\n", UNAME_NODENAME);
    }
    else if (plen == 2 && strncmp(p, "-r", 2) == 0)
    {
        snprintf(out, len, "%s\r\n", UNAME_RELEASE);
    }
    else if (plen == 2 && strncmp(p, "-m", 2) == 0)
    {
        snprintf(out, len, "%s\r\n", UNAME_MACHINE);
    }
    else
    {
        snprintf(out, len, "uname: unknown option '%.*s'\r\n", (int)plen, p);
    }

    return pdFALSE;
}

/* ── ps command (multi-shot, Linux style) ─────────────────────────────────── */

/* Same multi-shot convention as 'tasks': -1 header, 0..N rows, then timer. */
static int16_t _ps_line = -1;

__SECTION_OS
static BaseType_t _cmd_ps_fn(char *out, size_t len, const char *in)
{
    (void)in;

    const sys_health_t *h = task_mgr_get_health();

    if (h == NULL)
    {
        _ps_line = -1;
        snprintf(out, len, "ps: task manager unavailable\r\n");
        return pdFALSE;
    }

    /* ── Header ── */
    if (_ps_line == -1)
    {
        snprintf(out, len, "  PID S  STACK  COMMAND\r\n");
        _ps_line = 0;
        return pdTRUE;
    }

    /* ── Per-task rows ── */
    if (_ps_line < (int16_t)h->task_count)
    {
        const task_health_t *t = &h->tasks[_ps_line];
        snprintf(out, len, "  %3lu %c %6u  %s\r\n",
                 (unsigned long)t->thread_id,
                 _state_to_linux(t->state),
                 (unsigned)t->stack_hwm,
                 t->name ? t->name : "?");
        _ps_line++;

        vTaskDelay(pdMS_TO_TICKS(10));

        return pdTRUE;
    }

    /* ── Timer-daemon row ── */
    if (_ps_line == (int16_t)h->task_count && h->timer_task_valid)
    {
        const task_health_t *t = &h->timer_task;
        snprintf(out, len, "    T %c %6u  %s\r\n",
                 _state_to_linux(t->state),
                 (unsigned)t->stack_hwm,
                 t->name ? t->name : "Tmr Svc");
        _ps_line++;
        return pdTRUE;
    }

    /* ── Done ── */
    out[0] = '\0';
    _ps_line = -1;
    return pdFALSE;
}

/* ── free command ─────────────────────────────────────────────────────────── */

__SECTION_OS
static BaseType_t _cmd_free_fn(char *out, size_t len, const char *in)
{
    (void)in;

    size_t total = (size_t)configTOTAL_HEAP_SIZE;
    size_t fr    = (size_t)xPortGetFreeHeapSize();
    size_t used  = total - fr;
    size_t minf  = (size_t)xPortGetMinimumEverFreeHeapSize();

    snprintf(out, len,
             "              total       used       free   min-free\r\n"
             "Mem:     %10u %10u %10u %10u\r\n",
             (unsigned)total, (unsigned)used, (unsigned)fr, (unsigned)minf);

    return pdFALSE;
}

/* ── whoami / hostname commands ───────────────────────────────────────────── */

__SECTION_OS
static BaseType_t _cmd_whoami_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len, "root\r\n");
    return pdFALSE;
}

__SECTION_OS
static BaseType_t _cmd_hostname_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len, "%s\r\n", UNAME_NODENAME);
    return pdFALSE;
}

/* ── ping command (multi-shot) ────────────────────────────────────────────── */

#if defined(CONFIG_NET)

#define _PING_DEFAULT_COUNT   4U
#define _PING_MAX_COUNT       64U
#define _PING_TIMEOUT_MS      1000U
#define _PING_INTERVAL_MS     1000U

/*
 * Multi-shot state:
 *   -1            parse args, open session, print header
 *   0..count-1    one echo request + result line per shot
 *   count         print the statistics summary, close, reset to -1
 */
static int16_t  _ping_state = -1;
static uint16_t _ping_count;
static uint16_t _ping_tx;
static uint16_t _ping_rx;
static uint32_t _ping_rtt_min;
static uint32_t _ping_rtt_max;
static uint32_t _ping_rtt_sum;
static char     _ping_ip[16];

__SECTION_OS
static BaseType_t _cmd_ping_fn(char *out, size_t len, const char *in)
{
    /* ── First call: parse args, open session, emit header ── */
    if (_ping_state == -1)
    {
        BaseType_t  plen;
        const char *ip = FreeRTOS_CLIGetParameter(in, 1, &plen);

        if (ip == NULL || plen == 0 || plen >= (BaseType_t)sizeof(_ping_ip))
        {
            snprintf(out, len, "Usage: ping <ip> [count]\r\n");
            return pdFALSE;
        }

        memcpy(_ping_ip, ip, (size_t)plen);
        _ping_ip[plen] = '\0';

        /* Optional decimal packet count. */
        uint16_t    count = _PING_DEFAULT_COUNT;
        const char *cnt   = FreeRTOS_CLIGetParameter(in, 2, &plen);
        if (cnt != NULL && plen > 0)
        {
            uint32_t v = 0;
            for (BaseType_t i = 0; i < plen; i++)
            {
                if (cnt[i] < '0' || cnt[i] > '9') { v = 0; break; }
                v = (v * 10U) + (uint32_t)(cnt[i] - '0');
            }
            if (v == 0)               v = _PING_DEFAULT_COUNT;
            if (v > _PING_MAX_COUNT)   v = _PING_MAX_COUNT;
            count = (uint16_t)v;
        }

        int32_t rc = net_ping_open(_ping_ip);
        if (rc == OS_ERR_INVALID_ARG)
        {
            snprintf(out, len, "ping: invalid address '%s'\r\n", _ping_ip);
            return pdFALSE;
        }
        if (rc != OS_ERR_NONE)
        {
            snprintf(out, len, "ping: cannot start session (err %ld)\r\n",
                     (long)rc);
            return pdFALSE;
        }

        _ping_count   = count;
        _ping_tx      = 0;
        _ping_rx      = 0;
        _ping_rtt_min = 0xFFFFFFFFUL;
        _ping_rtt_max = 0;
        _ping_rtt_sum = 0;
        _ping_state   = 0;

        snprintf(out, len, "PING %s: %u data bytes\r\n",
                 _ping_ip, (unsigned)(NET_PING_PAYLOAD_LEN + 8U));
        return pdTRUE;
    }

    /* ── Per-packet shots ── */
    if (_ping_state < (int16_t)_ping_count)
    {
        uint16_t seq    = (uint16_t)(_ping_state + 1);
        uint32_t rtt_us = 0;

        _ping_tx++;
        int32_t rc = net_ping_once(seq, _PING_TIMEOUT_MS, &rtt_us);

        if (rc == OS_ERR_NONE)
        {
            _ping_rx++;
            _ping_rtt_sum += rtt_us;
            if (rtt_us < _ping_rtt_min) _ping_rtt_min = rtt_us;
            if (rtt_us > _ping_rtt_max) _ping_rtt_max = rtt_us;

            snprintf(out, len,
                     "%u bytes from %s: icmp_seq=%u time=%lu.%03lu ms\r\n",
                     (unsigned)(NET_PING_PAYLOAD_LEN + 8U),
                     _ping_ip, (unsigned)seq,
                     (unsigned long)(rtt_us / 1000U),
                     (unsigned long)(rtt_us % 1000U));

            /* Pace to ~1 packet/sec like the classic ping. */
            uint32_t rtt_ms = rtt_us / 1000U;
            if (rtt_ms < _PING_INTERVAL_MS)
                vTaskDelay(pdMS_TO_TICKS(_PING_INTERVAL_MS - rtt_ms));
        }
        else
        {
            snprintf(out, len, "Request timeout for icmp_seq %u\r\n",
                     (unsigned)seq);
            /* A timeout already consumed ~1 s in the wait — no extra delay. */
        }

        _ping_state++;
        return pdTRUE;
    }

    /* ── Statistics summary (final shot) ── */
    {
        unsigned loss = (_ping_tx > 0)
            ? (unsigned)(((uint32_t)(_ping_tx - _ping_rx) * 100U) / _ping_tx)
            : 0U;

        if (_ping_rx > 0)
        {
            uint32_t avg = _ping_rtt_sum / _ping_rx;
            snprintf(out, len,
                     "--- %s ping statistics ---\r\n"
                     "%u transmitted, %u received, %u%% packet loss\r\n"
                     "rtt min/avg/max = %lu.%03lu/%lu.%03lu/%lu.%03lu ms\r\n",
                     _ping_ip, (unsigned)_ping_tx, (unsigned)_ping_rx, loss,
                     (unsigned long)(_ping_rtt_min / 1000U),
                     (unsigned long)(_ping_rtt_min % 1000U),
                     (unsigned long)(avg / 1000U),
                     (unsigned long)(avg % 1000U),
                     (unsigned long)(_ping_rtt_max / 1000U),
                     (unsigned long)(_ping_rtt_max % 1000U));
        }
        else
        {
            snprintf(out, len,
                     "--- %s ping statistics ---\r\n"
                     "%u transmitted, 0 received, 100%% packet loss\r\n",
                     _ping_ip, (unsigned)_ping_tx);
        }
    }

    net_ping_close();
    _ping_state = -1;
    return pdFALSE;
}

#endif /* CONFIG_NET */

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
#if defined(CONFIG_INC_SERVICE_IIC_MGMT) && (CONFIG_INC_SERVICE_IIC_MGMT == 1)
    FreeRTOS_CLIRegisterCommand(&_cmd_iic_scan);
#endif

    /* Linux-style commands. */
    FreeRTOS_CLIRegisterCommand(&_cmd_echo);
    FreeRTOS_CLIRegisterCommand(&_cmd_clear);
    FreeRTOS_CLIRegisterCommand(&_cmd_uname);
    FreeRTOS_CLIRegisterCommand(&_cmd_ps);
    FreeRTOS_CLIRegisterCommand(&_cmd_free);
    FreeRTOS_CLIRegisterCommand(&_cmd_whoami);
    FreeRTOS_CLIRegisterCommand(&_cmd_hostname);
#if defined(CONFIG_NET)
    FreeRTOS_CLIRegisterCommand(&_cmd_ping);
#endif
}
