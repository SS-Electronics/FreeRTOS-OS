/*
 * os_shell_management.c — FreeRTOS+CLI interactive shell service
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Shell I/O wiring
 * ────────────────
 *   RX: UART_SHELL (UART_APP, USART2) RX ring buffer — filled by the RXNE ISR
 *       via drv_uart_rx_isr_dispatch() → _uart_rx_cb (uart_mgmt.c).
 *       The shell task polls the ring buffer with a short yield when empty.
 *
 *   TX: Written directly to the UART_SHELL TX ring buffer via
 *       ringbuffer_put(), then drv_uart_tx_kick() enables TXEIE so the
 *       existing TXE ISR drains the buffer byte by byte.
 *
 * Line editing
 * ────────────
 *   • Printable characters are echoed and buffered.
 *   • BS (0x08) and DEL (0x7F) erase the last character.
 *   • CR / LF submits the line to FreeRTOS_CLIProcessCommand().
 *   • Lines longer than SHELL_LINE_BUF_LEN-1 bytes are silently truncated.
 *
 * FreeRTOS+CLI output loop
 * ────────────────────────
 *   FreeRTOS_CLIProcessCommand() is called repeatedly until it returns
 *   pdFALSE (no more output pending) so multi-page command output works.
 *   Each chunk is written to the TX ring buffer before the next call.
 */

#include <services/os_shell_management.h>

#include <string.h>
#include <stdio.h>

#include <os/kernel.h>
#include <board/mcu_config.h>
#include <config/conf_os.h>
#include <def_err.h>

#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>
#include <drivers/drv_handle.h>
#include <drivers/timer/drv_time.h>

#include <FreeRTOS_CLI.h>

#if (INC_SERVICE_OS_SHELL_MGMT == 1)

/* ── Built-in command forward declarations ────────────────────────────────── */

static BaseType_t _cmd_help_fn   (char *out, size_t len, const char *in);
static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in);
static BaseType_t _cmd_uptime_fn (char *out, size_t len, const char *in);
static BaseType_t _cmd_reboot_fn (char *out, size_t len, const char *in);

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

/* ── Shell state ──────────────────────────────────────────────────────────── */

#define SHELL_PROMPT        "\r\n> "
#define SHELL_BANNER        "\r\n\r\n=== FreeRTOS-OS Shell ===\r\n"   \
                            "Type 'help' for available commands.\r\n"  \
                            SHELL_PROMPT

static char     _line_buf[SHELL_LINE_BUF_LEN];
static uint16_t _line_pos;
static char     _out_buf[SHELL_OUT_BUF_LEN];

/* ── TX helper — writes directly to the shell UART TX ring buffer ──────────
 *
 * Bypasses the uart_mgmt queue so the interrupt-driven TX path (TXEIE ISR +
 * ring buffer, set up by uart_mgmt at startup) handles transmission.
 * This is the same mechanism printk() uses.
 */
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

/* ── Line processor ───────────────────────────────────────────────────────── */

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

/* ── Shell task ───────────────────────────────────────────────────────────── */

static void _os_shell_task(void *arg)
{
    (void)arg;

    /* Wait for UART_SHELL to be registered by uart_mgmt */
    os_thread_delay(TIME_OFFSET_OS_SHELL_MGMT);

    /* Register built-in commands */
    FreeRTOS_CLIRegisterCommand(&_cmd_help);
    FreeRTOS_CLIRegisterCommand(&_cmd_version);
    FreeRTOS_CLIRegisterCommand(&_cmd_uptime);
    FreeRTOS_CLIRegisterCommand(&_cmd_reboot);

    /* Print welcome banner */
    _shell_write(SHELL_BANNER, (uint16_t)(sizeof(SHELL_BANNER) - 1));

    memset(_line_buf, 0, sizeof(_line_buf));
    _line_pos = 0;

    for (;;)
    {
        uint8_t byte;

        /* Poll the UART_SHELL RX ring buffer; yield when empty */
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

            case '\b':   /* BS  */
            case 0x7FU:  /* DEL */
                if (_line_pos > 0)
                {
                    _line_pos--;
                    _line_buf[_line_pos] = '\0';
                    _shell_write("\b \b", 3);
                }
                break;

            case 0x03U:  /* Ctrl-C — discard current line */
                memset(_line_buf, 0, sizeof(_line_buf));
                _line_pos = 0;
                _shell_write("^C", 2);
                _shell_write(SHELL_PROMPT, (uint16_t)(sizeof(SHELL_PROMPT) - 1));
                break;

            default:
                if (byte >= 0x20U && _line_pos < (SHELL_LINE_BUF_LEN - 1U))
                {
                    _line_buf[_line_pos++] = (char)byte;
                    _shell_write((const char *)&byte, 1); /* local echo */
                }
                break;
        }
    }
}

/* ── Built-in command implementations ─────────────────────────────────────── */

static BaseType_t _cmd_help_fn(char *out, size_t len, const char *in)
{
    (void)in;
    /* FreeRTOS+CLI has a built-in "help" list, but we can produce a custom one.
     * Returning pdFALSE lets the CLI engine print the registered help strings. */
    snprintf(out, len,
             "Use FreeRTOS+CLI registered commands:\r\n"
             "  Type any command and press Enter.\r\n");
    return pdFALSE;
}

static BaseType_t _cmd_version_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len,
             "FreeRTOS-OS v1.0  built %s %s\r\n"
             "Kernel: FreeRTOS V202212.00\r\n",
             __DATE__, __TIME__);
    return pdFALSE;
}

static BaseType_t _cmd_uptime_fn(char *out, size_t len, const char *in)
{
    (void)in;
    uint32_t ms = drv_time_get_ticks();
    snprintf(out, len,
             "Uptime: %lu ms  (%lu s)\r\n",
             (unsigned long)ms, (unsigned long)(ms / 1000UL));
    return pdFALSE;
}

static BaseType_t _cmd_reboot_fn(char *out, size_t len, const char *in)
{
    (void)in;
    snprintf(out, len, "Rebooting...\r\n");
    /* Flush output before reset */
    _shell_write(out, (uint16_t)strlen(out));
    vTaskDelay(pdMS_TO_TICKS(50));
    NVIC_SystemReset();
    return pdFALSE; /* unreachable */
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int32_t os_shell_mgmt_start(void)
{
    int32_t tid = os_thread_create(_os_shell_task,
                                   "os_shell",
                                   PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE,
                                   PROC_SERVICE_OS_SHELL_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* INC_SERVICE_OS_SHELL_MGMT == 1 */
