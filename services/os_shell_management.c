/**
 * @file    os_shell_management.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  services
 * @brief   FreeRTOS+CLI interactive shell service for UART-based console
 *
 * @details
 * This module implements an interactive command-line shell running on top
 * of FreeRTOS+CLI. It provides a UART-based console interface for system
 * debugging, diagnostics, and runtime control.
 *
 * The shell operates as a dedicated FreeRTOS task and integrates with the
 * UART driver stack through ringbuffer-based I/O.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Shell I/O architecture
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * RX path:
 *   UART RX interrupt (RXNE)
 *        ↓
 *   drv_uart_rx_isr_dispatch()
 *        ↓
 *   _uart_rx_cb (uart_mgmt.c)
 *        ↓
 *   RX ringbuffer (global_uart_rx_mqueue_list)
 *        ↓
 *   shell task polls buffer
 *
 * TX path:
 *   shell task
 *        ↓
 *   ringbuffer_put() (UART TX buffer)
 *        ↓
 *   drv_uart_tx_kick()
 *        ↓
 *   TXE ISR drains buffer byte-by-byte
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Line editing behavior
 * ─────────────────────────────────────────────────────────────────────────────
 * - Printable ASCII characters are echoed and stored in a line buffer
 * - Backspace (0x08) and DEL (0x7F) remove last character
 * - CR/LF terminates input and triggers command execution
 * - Long lines are truncated safely
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * CLI execution model
 * ─────────────────────────────────────────────────────────────────────────────
 * - Uses FreeRTOS_CLIProcessCommand()
 * - Supports multi-part command output
 * - Continues calling CLI until pdFALSE is returned
 * - Each output chunk is streamed to UART TX buffer
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Initialization flow
 * ─────────────────────────────────────────────────────────────────────────────
 * @code
 * os_shell_mgmt_start()
 *      ↓
 * Create shell task
 *      ↓
 * Wait for UART subsystem initialization
 *      ↓
 * Register built-in CLI commands
 *      ↓
 * Register system command modules
 *      ↓
 * Print boot banner
 *      ↓
 * Enter interactive loop
 * @endcode
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Built-in commands
 * ─────────────────────────────────────────────────────────────────────────────
 * - help     : Show available commands
 * - version  : Firmware version info
 * - uptime   : System uptime
 * - reboot   : Software reset (NVIC_SystemReset)
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Dependencies
 * ─────────────────────────────────────────────────────────────────────────────
 * services/os_shell_management.h,
 * os/kernel.h, os/kernel_syscall.h,
 * board/mcu_config.h, config/conf_os.h,
 * ipc/ringbuffer.h, ipc/global_var.h, ipc/mqueue.h,
 * drivers/drv_time.h, drivers/drv_uart.h,
 * task.h, FreeRTOS_CLI.h,
 * shell/shell_task_mgmt.h, board/board_config.h
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Notes
 * ─────────────────────────────────────────────────────────────────────────────
 * - Runs as a dedicated system service task
 * - Relies on UART management layer for transport
 * - Uses polling for RX (no blocking ISR dependency in shell task)
 * - TX is fully interrupt-driven via ringbuffer
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Warnings
 * ─────────────────────────────────────────────────────────────────────────────
 * - RX buffer overflow leads to silent truncation of input
 * - CLI commands must be thread-safe if they access shared resources
 * - Shell task assumes UART_SHELL is initialized by uart_mgmt
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * License
 * ─────────────────────────────────────────────────────────────────────────────
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

#include <services/os_shell_management.h>
#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>
#include <drivers/drv_time.h>
#include <drivers/drv_uart.h>
#include <FreeRTOS_CLI.h>
#include <shell/shell_task_mgmt.h>
#include <board/board_config.h>
#include <board/board_config.h>


#if (BOARD_UART_COUNT > 0)



/* ── Shell state ────────────────────────────────────────────────────────── */

#define SHELL_PROMPT "\r\nOS >"

__SECTION_OS_DATA __USED
static char     _line_buf[SHELL_LINE_BUF_LEN];

__SECTION_OS_DATA __USED
static uint16_t _line_pos;

__SECTION_OS_DATA __USED
static char     _out_buf[SHELL_OUT_BUF_LEN];


/* ── TX helper (UART ringbuffer backend) ────────────────────────────────── */
__SECTION_OS __USED
static void _shell_write(const char *str, uint16_t len)
{
    if (str == NULL || len == 0)
        return;

    struct ringbuffer *rb = (struct ringbuffer *)
        ipc_mqueue_get_handle(global_uart_tx_mqueue_list[UART_SHELL_HW_ID]);

    if (rb == NULL)
        return;

    for (uint16_t i = 0; i < len; i++)
        ringbuffer_putchar(rb, (uint8_t)str[i]);

    drv_uart_tx_start(UART_SHELL_HW_ID);
}

/* ── Boot banner printer ────────────────────────────────────────────────── */

__SECTION_OS __USED
static void _print_banner(void)
{
    extern uint32_t SystemCoreClock;
    const board_config_t *brd = board_get_config();
    uint32_t mhz = SystemCoreClock / 1000000UL;
    uint16_t n;

#define _W(s) _shell_write((s), (uint16_t)(sizeof(s) - 1))

    _W("\r\n");
    _W(" +--------------------------------------------+\r\n");
    _W(" |   ____              ___ _____ ___  ____    |\r\n");
    _W(" |  |  __| ___ ___ ___|  _|_   _/   \\/ ___|  |\r\n");
    _W(" |  | |_  |  _| -_| -_|_  | | || o  \\___  |  |\r\n");
    _W(" |  |____||_| |___|___|___| |_| \\___/|____/  |\r\n");
    _W(" |              O S   v 1 . 0                 |\r\n");
    _W(" +--------------------------------------------+\r\n");

#undef _W

os_thread_delay(100);

#define _L(fmt, ...) \
    do { n = (uint16_t)snprintf(_out_buf, sizeof(_out_buf), fmt, ##__VA_ARGS__); \
         _shell_write(_out_buf, n); } while (0)

    _L(" | Board    : %-28s\r\n", brd ? brd->board_name : "unknown");
    _L(" | MCU      : %-28s\r\n", CONFIG_TARGET_MCU);
    _L(" | CPU Clock: %-3lu MHz \r\n", (unsigned long)mhz);
    _L(" | Kernel   : FreeRTOS %-20s\r\n", tskKERNEL_VERSION_NUMBER);
    _L(" | Build    : %-28s\r\n", __DATE__ " " __TIME__);

#undef _L


os_thread_delay(100);

#define _W(s) _shell_write((s), (uint16_t)(sizeof(s) - 1))
    _W(" +-----------------------------------------+\r\n");
    _W("   Type 'help' for available commands.\r\n");
#undef _W
}


/* ── Command processing ─────────────────────────────────────────────────── */
__SECTION_OS __USED
static void _shell_process_line(void)
{
    BaseType_t more;

    if (_line_pos == 0)
    {
        _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));
        return;
    }

    _shell_write("\r\n", 2);

    do {
        memset(_out_buf, 0, sizeof(_out_buf));
        more = FreeRTOS_CLIProcessCommand(_line_buf, _out_buf, sizeof(_out_buf));

        uint16_t olen = (uint16_t)strlen(_out_buf);
        if (olen > 0)
            _shell_write(_out_buf, olen);

    } while (more == pdTRUE);

    _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));

    memset(_line_buf, 0, sizeof(_line_buf));
    _line_pos = 0;
}

/* ── Shell task ─────────────────────────────────────────────────────────── */
__SECTION_OS __USED
static void _os_shell_task(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_OS_SHELL_MGMT);

    /* ── Bind printk to the shell UART TX ring buffer ── */
    printk_init();

    printk_enable();

    shell_task_mgmt_register_cmds();

    _print_banner();

    _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));

    memset(_line_buf, 0, sizeof(_line_buf));
    _line_pos = 0;

    while(true)
    {
        uint8_t byte;

        struct ringbuffer *rx_rb = (struct ringbuffer *)
            ipc_mqueue_get_handle(global_uart_rx_mqueue_list[UART_SHELL_HW_ID]);

        if (rx_rb == NULL || ringbuffer_getchar(rx_rb, &byte) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        switch (byte)
        {
            case '\r':
            case '\n':
                _line_buf[_line_pos] = '\0';
                _shell_process_line();
                break;

            case '\b':
            case 0x7FU:
                if (_line_pos > 0)
                {
                    _line_pos--;
                    _line_buf[_line_pos] = '\0';
                    _shell_write("\b \b", 3);
                }
                break;

            case 0x1BU:
                printk_disable();
                _shell_write("\r\n[debug off]\r\n", 15);
                _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));
                break;

            case 0x03U:
                memset(_line_buf, 0, sizeof(_line_buf));
                _line_pos = 0;
                _shell_write("^C", 2);
                _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));
                break;

            default:
                if (byte >= 0x20U && _line_pos < (SHELL_LINE_BUF_LEN - 1U))
                {
                    _line_buf[_line_pos++] = (char)byte;
                    _shell_write((const char *)&byte, 1);
                }
                break;
        }
    }
}


/* ── Public API ─────────────────────────────────────────────────────────── */

__SECTION_OS __USED
int32_t os_shell_mgmt_start(void)
{
    int32_t tid = os_thread_create(_os_shell_task,
                                   "MGMT_SHELL",
                                   PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE,
                                   PROC_SERVICE_OS_SHELL_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* BOARD_UART_COUNT > 0 */