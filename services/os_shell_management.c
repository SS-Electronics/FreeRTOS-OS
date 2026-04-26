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

/* ── Built-in command forward declarations ──────────────────────────────── */

static BaseType_t _cmd_help_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_uptime_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_reboot_fn(char *out, size_t len, const char *in);

/* ── CLI command descriptors ─────────────────────────────────────────────── */
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

/* ── Shell state ────────────────────────────────────────────────────────── */

#define SHELL_PROMPT "\r\n> "
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

    drv_uart_tx_kick(UART_SHELL_HW_ID);
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
    _W(" +-----------------------------------------+\r\n");
    _W(" |   ____              ___ _____ ___  ____  |\r\n");
    _W(" |  |  __| ___ ___ ___|  _|_   _/   \\/ ___| |\r\n");
    _W(" |  | |_  |  _| -_| -_|_  | | || o  \\___  | |\r\n");
    _W(" |  |____||_| |___|___|___| |_| \\___/|____/ |\r\n");
    _W(" |              O S   v 1 . 0               |\r\n");
    _W(" +-----------------------------------------+\r\n");

#undef _W

#define _L(fmt, ...) \
    do { n = (uint16_t)snprintf(_out_buf, sizeof(_out_buf), fmt, ##__VA_ARGS__); \
         _shell_write(_out_buf, n); } while (0)

    _L(" | Board    : %-28s|\r\n", brd ? brd->board_name : "unknown");
    _L(" | MCU      : %-28s|\r\n", CONFIG_TARGET_MCU);
    _L(" | CPU Clock: %-3lu MHz                      |\r\n", (unsigned long)mhz);
    _L(" | Kernel   : FreeRTOS %-20s|\r\n", tskKERNEL_VERSION_NUMBER);
    _L(" | Build    : %-28s|\r\n", __DATE__ " " __TIME__);

#undef _L

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

    FreeRTOS_CLIRegisterCommand(&_cmd_help);
    FreeRTOS_CLIRegisterCommand(&_cmd_version);
    FreeRTOS_CLIRegisterCommand(&_cmd_uptime);
    FreeRTOS_CLIRegisterCommand(&_cmd_reboot);

    shell_task_mgmt_register_cmds();

    _print_banner();
    _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));

    memset(_line_buf, 0, sizeof(_line_buf));
    _line_pos = 0;

    for (;;)
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
    snprintf(out, len, "Rebooting...\r\n");
    _shell_write(out, (uint16_t)strlen(out));
    vTaskDelay(pdMS_TO_TICKS(50));
    NVIC_SystemReset();
    return pdFALSE;
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