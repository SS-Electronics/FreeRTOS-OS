/**
 * @file        syscalls.c
 * @brief       syscalls.c — Newlib syscall stubs + printk (bare-metal ARM)
 * @ingroup     kernel_glue
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Kernel Glue
 * @info        C-runtime / newlib shim: malloc/free wrappers, syscalls, FreeRTOS kernel adapters.
 * @dependency  newlib-nano, FreeRTOS-Kernel
 *
 * @details
 * syscalls.c — Newlib syscall stubs + printk (bare-metal ARM)
 *
 * Two responsibilities:
 *
 * 1. Newlib low-level I/O hooks required by the C library on a bare-metal
 *    target.  _write / _read are weak so board-level code can override them.
 *    __io_putchar / __io_getchar are the byte-level hooks used by _write/_read.
 *
 * 2. printk — timestamped formatted string output routed through the UART TX
 *    ring buffer.  printk_init() must be called after uart_mgmt_thread registers
 *    the UART ring-buffer queues so the handle is valid.
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

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <os/kernel_syscall.h>
#include <ipc/mqueue.h>
#include <drivers/drv_uart.h>


/* ── printk state ─────────────────────────────────────────────────────────── */
__SECTION_OS_DATA
static char              _time_buf[10];

__SECTION_OS_DATA
static char              _msg_buf[CONF_MAX_CHAR_IN_PRINTK];

__SECTION_OS_DATA
static struct ringbuffer *_printk_rb;

__SECTION_OS_DATA
static volatile uint8_t  _printk_enabled = 0;




/* ── printk functions──────────────────────────────────────────────────────── */
__SECTION_OS
void printk_enable(void)  
{ 
    _printk_enabled = 1; 
}


__SECTION_OS
void printk_disable(void) 
{ 
    _printk_enabled = 0; 
}


/* ── Newlib byte-level I/O hooks ──────────────────────────────────────────── */
__SECTION_OS
int __io_putchar(int ch) 
{ 
    (void)ch; return 0; 
}

__SECTION_OS
int __io_getchar(void)         
{ 
    return 0; 
}

/* ── Newlib environment ───────────────────────────────────────────────────── */
__SECTION_OS_DATA
char  *__env[1] = { 0 };

__SECTION_OS_DATA
char **environ  = __env;

__SECTION_OS
void initialise_monitor_handles(void) 
{

}

/* ── Newlib syscall stubs ─────────────────────────────────────────────────── */
__SECTION_OS
int _getpid(void) 
{ 
    return 1; 
}

__SECTION_OS
int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

__SECTION_OS
void _exit(int status)
{
    _kill(status, -1);
    while (1) {}
}

__SECTION_OS __WEAK
int _read(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
        *ptr++ = (char)__io_getchar();
    return len;
}

__SECTION_OS __WEAK
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
        __io_putchar((unsigned char)*ptr++);
    return len;
}

__SECTION_OS
int _close(int file)  
{ 
    (void)file; return -1;
}

__SECTION_OS
int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

__SECTION_OS
int _isatty(int file) 
{ 
    (void)file; return 1; 
}

__SECTION_OS
int _lseek(int file, int ptr, int dir)
{
    (void)file; (void)ptr; (void)dir;
    return 0;
}

__SECTION_OS
int _open(char *path, int flags, ...)
{
    (void)path; (void)flags;
    return -1;
}

__SECTION_OS
int _wait(int *status)      
{ 
    (void)status; errno = ECHILD; return -1; 
}

__SECTION_OS
int _unlink(char *name)     
{ 
    (void)name;   errno = ENOENT; return -1; 
}

__SECTION_OS
int _times(struct tms *buf) 
{ 
    (void)buf;    return -1; 
}

__SECTION_OS
int _stat(char *file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

__SECTION_OS
int _link(char *old, char *new)
{
    (void)old; (void)new;
    errno = EMLINK;
    return -1;
}

__SECTION_OS
int _fork(void) 
{ 
    errno = EAGAIN; return -1; 
}

__SECTION_OS
int _execve(char *name, char **argv, char **env)
{
    (void)name; (void)argv; (void)env;
    errno = ENOMEM;
    return -1;
}

/* ── printk ───────────────────────────────────────────────────────────────── */
__SECTION_OS
void printk_init(void)
{
#ifdef UART_SHELL_HW_ID
    if (UART_SHELL_HW_ID < BOARD_UART_COUNT)
        _printk_rb = (struct ringbuffer *)
                     ipc_mqueue_get_handle(global_uart_tx_mqueue_list[UART_SHELL_HW_ID]);
#else
#  error "printk: UART_SHELL_HW_ID not defined — run 'make board-gen' to regenerate board_device_ids.h"
#endif
}

__SECTION_OS
int32_t printk(const char *fmt, ...)
{
#if (BOARD_UART_COUNT > 0)
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

     drv_uart_tx_start(UART_SHELL_HW_ID);

    va_end(args);

    return msg_len;
#else
    (void)fmt;
    return 0;
#endif
}
