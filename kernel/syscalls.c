/*
 * syscalls.c — Newlib syscall stubs + printk (bare-metal ARM)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Two responsibilities:
 *
 * 1. Newlib low-level I/O hooks required by the C library on a bare-metal
 *    target.  _write / _read are weak so board-level code can override them.
 *    __io_putchar / __io_getchar are the byte-level hooks used by _write/_read.
 *
 * 2. printk — timestamped formatted string output routed through the UART TX
 *    ring buffer.  printk_init() must be called after ipc_queues_init() so the
 *    ring-buffer handle is valid.
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#include <os/kernel_syscall.h>
#include <ipc/mqueue.h>
#include <drivers/drv_uart.h>
#include <stdarg.h>

/* ── printk state ─────────────────────────────────────────────────────────── */

static char              _time_buf[10];
static char              _msg_buf[CONF_MAX_CHAR_IN_PRINTK];
static struct ringbuffer *_printk_rb;
static volatile uint8_t  _printk_enabled = 0;

void printk_enable(void)  { _printk_enabled = 1; }
void printk_disable(void) { _printk_enabled = 0; }

/* ── Newlib byte-level I/O hooks ──────────────────────────────────────────── */

int __io_putchar(int ch) { (void)ch; return 0; }
int __io_getchar(void)         { return 0; }

/* ── Newlib environment ───────────────────────────────────────────────────── */

char  *__env[1] = { 0 };
char **environ  = __env;

void initialise_monitor_handles(void) {}

/* ── Newlib syscall stubs ─────────────────────────────────────────────────── */

int _getpid(void) { return 1; }

int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    _kill(status, -1);
    while (1) {}
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
        *ptr++ = (char)__io_getchar();
    return len;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
        __io_putchar((unsigned char)*ptr++);
    return len;
}

int _close(int file)  { (void)file; return -1; }

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) { (void)file; return 1; }

int _lseek(int file, int ptr, int dir)
{
    (void)file; (void)ptr; (void)dir;
    return 0;
}

int _open(char *path, int flags, ...)
{
    (void)path; (void)flags;
    return -1;
}

int _wait(int *status)      { (void)status; errno = ECHILD; return -1; }
int _unlink(char *name)     { (void)name;   errno = ENOENT; return -1; }
int _times(struct tms *buf) { (void)buf;    return -1; }

int _stat(char *file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _link(char *old, char *new)
{
    (void)old; (void)new;
    errno = EMLINK;
    return -1;
}

int _fork(void) { errno = EAGAIN; return -1; }

int _execve(char *name, char **argv, char **env)
{
    (void)name; (void)argv; (void)env;
    errno = ENOMEM;
    return -1;
}

/* ── printk ───────────────────────────────────────────────────────────────── */

void printk_init(void)
{
#ifdef COMM_PRINTK_HW_ID
    if (COMM_PRINTK_HW_ID < NO_OF_UART)
        _printk_rb = (struct ringbuffer *)
                     ipc_mqueue_get_handle(global_uart_tx_mqueue_list[COMM_PRINTK_HW_ID]);
#else
#  error "printk: COMM_PRINTK_HW_ID not defined — set it in conf_os.h"
#endif
}

int32_t printk(const char *fmt, ...)
{
#if (NO_OF_UART > 0)
    if (!_printk_enabled)
        return 0;

    va_list args;
    int     ts_len, msg_len, i;

    va_start(args, fmt);

    ATOMIC_ENTER_CRITICAL();

    ts_len = snprintf(_time_buf, sizeof(_time_buf), "[%lu] ",
                      (unsigned long)drv_time_get_ticks());
    for (i = 0; i < ts_len; i++)
        if (_printk_rb) ringbuffer_putchar(_printk_rb, (uint8_t)_time_buf[i]);

    msg_len = vsnprintf(_msg_buf, sizeof(_msg_buf), fmt, args);
    if (msg_len > 0)
        for (i = 0; i < msg_len; i++)
            if (_printk_rb) ringbuffer_putchar(_printk_rb, (uint8_t)_msg_buf[i]);

    ATOMIC_EXIT_CRITICAL();

    va_end(args);

    drv_uart_tx_kick(COMM_PRINTK_HW_ID);
    return msg_len;
#else
    (void)fmt;
    return 0;
#endif
}
