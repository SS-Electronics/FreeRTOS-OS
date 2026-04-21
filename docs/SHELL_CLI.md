# OS Shell CLI — FreeRTOS-OS

An interactive command-line interface running over a designated UART, built on FreeRTOS+CLI. Input is received via the interrupt-driven UART RX ring buffer; output is written directly into the TX ring buffer and drained by the TXEIE interrupt path.

---

## Table of Contents

- [Architecture](#architecture)
- [I/O Paths in Detail](#io-paths-in-detail)
  - [RX Path — UART ISR to Shell Task](#rx-path--uart-isr-to-shell-task)
  - [TX Path — Shell Task to UART Wire](#tx-path--shell-task-to-uart-wire)
- [Shell UART Assignment](#shell-uart-assignment)
- [Configuration](#configuration)
- [Built-in Commands](#built-in-commands)
- [Registering Commands](#registering-commands)
  - [Simple command (no arguments)](#simple-command-no-arguments)
  - [Command with arguments](#command-with-arguments)
  - [Multi-page output](#multi-page-output)
- [Line Editing](#line-editing)
- [printk Integration](#printk-integration)
- [File Map](#file-map)

---

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    Application Commands                       │
│   os_shell_register_command(&my_cmd)                        │
└───────────────────────────┬──────────────────────────────────┘
                            │ FreeRTOS_CLIRegisterCommand()
┌───────────────────────────▼──────────────────────────────────┐
│              FreeRTOS+CLI  (kernel/FreeRTOS-Plus-CLI/)        │
│   FreeRTOS_CLIRegisterCommand()                              │
│   FreeRTOS_CLIProcessCommand()  — returns pdTRUE/pdFALSE     │
└──────────┬────────────────────────────────┬──────────────────┘
           │ RX ring buffer poll            │ TX ring buffer write
┌──────────▼──────────┐          ┌──────────▼──────────────────┐
│  _os_shell_task()   │          │  global_uart_tx_mqueue       │
│  (services/)        │          │  drv_uart_tx_kick()          │
└──────────┬──────────┘          └──────────┬──────────────────┘
           │                                │  TXEIE
┌──────────▼──────────┐          ┌──────────▼──────────────────┐
│  global_uart_rx_    │          │  TXE ISR → USART2->DR       │
│  mqueue (ring buf)  │          │  (TXEIE disabled when empty) │
└──────────┬──────────┘          └─────────────────────────────┘
           │  RXNE ISR
┌──────────▼──────────────────────────────────────────────────┐
│               USART2_IRQHandler()  (hal_it_stm32.c)          │
│  RXNE: ringbuffer_putchar(rx_rb, byte)                       │
│        drv_irq_dispatch(IRQ_ID_UART_RX(1))                  │
│  TXE:  drv_uart_tx_get_next_byte(1, &b) → USART2->DR        │
└─────────────────────────────────────────────────────────────┘
```

The shell task never interacts with the `uart_mgmt` management queue. This means:

- Shell output is never delayed by pending `uart_mgmt` TX requests from other tasks.
- `printk()` (which also writes directly to a ring buffer on `COMM_PRINTK_HW_ID`) and shell output can coexist cleanly as long as `COMM_PRINTK_HW_ID != UART_SHELL_HW_ID`.

---

## I/O Paths in Detail

### RX Path — UART ISR to Shell Task

```
USART2_IRQHandler()
  │  RXNE flag set
  └─ hal_uart_stm32_irq_handler(USART2)
       ├─ reads DR → byte
       ├─ ringbuffer_putchar(global_uart_rx_mqueue_list[1], byte)
       └─ drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(1), &byte, &hpt)
            └─ handle_simple_irq() → irqaction chain
                 └─ (any request_irq'd handler on UART_RX(1))

_os_shell_task()
  └─ for (;;):
       rb = ipc_mqueue_get_handle(global_uart_rx_mqueue_list[UART_SHELL_HW_ID])
       byte = ringbuffer_getchar(rb)
       if empty: vTaskDelay(5 ms); continue
       process byte → line buffer
```

The 5 ms yield when the ring buffer is empty prevents the shell task from starving other tasks. On a 100 MHz Cortex-M4 this costs ~1 character latency at 115200 baud (character period ≈ 87 µs), which is imperceptible for interactive use.

### TX Path — Shell Task to UART Wire

```
_shell_write(str, len)
  │
  └─ rb = ipc_mqueue_get_handle(global_uart_tx_mqueue_list[UART_SHELL_HW_ID])
       for each byte: ringbuffer_putchar(rb, byte)
  └─ drv_uart_tx_kick(UART_SHELL_HW_ID)
         SET_BIT(USART2->CR1, USART_CR1_TXEIE)   ← enables TXE interrupt

USART2_IRQHandler()  (TXE flag)
  └─ drv_uart_tx_get_next_byte(1, &b)
       if byte available:  WRITE_REG(USART2->DR, b)
       if ring buffer empty:
           CLEAR_BIT(USART2->CR1, USART_CR1_TXEIE)  ← disable TXEIE
           drv_irq_dispatch(IRQ_ID_UART_TX_DONE(1))  ← TX-done event
```

No heap allocation, no `uart_mgmt` queue item, no task notification required. The interrupt fires once per byte until the ring buffer is drained.

---

## Shell UART Assignment

The shell UART is assigned in two places that must agree:

**1. Board XML** — mark exactly one UART with `role="shell"`:

```xml
<uart id="UART_APP" dev_id="1" instance="USART2" … role="shell">
```

**2. `config/conf_os.h`** — set `UART_SHELL_HW_ID` to the matching `dev_id`:

```c
#define UART_SHELL_HW_ID   (1)   /* UART_APP (USART2) */
```

Running `make board-gen` after editing the XML regenerates `board_device_ids.h` with:

```c
#define BOARD_UART_SHELL_ID   UART_APP   /* role="shell" */
```

`board_get_shell_uart_id()` returns `BOARD_UART_SHELL_ID` at runtime. The management threads (uart_mgmt) also own this UART for normal TX/RX, but the shell task bypasses the queue for its own output.

---

## Configuration

All in `config/conf_os.h` (edit directly or via `make menuconfig`):

| Macro | Default | Description |
|-------|---------|-------------|
| `INC_SERVICE_OS_SHELL_MGMT` | `1` | `1` = compile and start the shell task |
| `UART_SHELL_HW_ID` | `1` | `dev_id` of the shell UART (must match `BOARD_UART_SHELL_ID`) |
| `SHELL_LINE_BUF_LEN` | `128` | Maximum input line length in bytes (including null terminator) |
| `SHELL_OUT_BUF_LEN` | `512` | Output scratch buffer per FreeRTOS+CLI call |
| `TIME_OFFSET_OS_SHELL_MGMT` | `4500` ms | Startup delay — must be greater than `TIME_OFFSET_SERIAL_MANAGEMENT` (4000 ms) so UART is initialised first |

Stack and priority (also in `conf_os.h`):

```c
#define PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE   512   /* words */
#define PROC_SERVICE_OS_SHELL_MGMT_PRIORITY       2
```

---

## Built-in Commands

| Command  | Arguments | Description |
|----------|-----------|-------------|
| `help`   | none      | Lists all registered commands with their one-line help strings |
| `version`| none      | Prints the firmware version string from `FW_VERSION_STRING` |
| `uptime` | none      | Prints the FreeRTOS tick count and wall-clock time |
| `reboot` | none      | Calls `NVIC_SystemReset()` — immediate MCU reset |

---

## Registering Commands

### Simple command (no arguments)

```c
#include <services/os_shell_management.h>

static BaseType_t _cmd_temp(char *out, size_t out_len, const char *args)
{
    int32_t temp_c = sensor_read_temperature();
    snprintf(out, out_len, "Temperature: %ld C\r\n", (long)temp_c);
    return pdFALSE;   /* output complete */
}

static const CLI_Command_Definition_t _temp_cmd = {
    "temp",
    "temp: read temperature sensor\r\n",
    _cmd_temp,
    0   /* 0 required arguments */
};

/* Register before or after os_shell_mgmt_start() — FreeRTOS+CLI is safe to
   call from any task before the shell task starts processing input. */
os_shell_register_command(&_temp_cmd);
```

### Command with arguments

FreeRTOS+CLI parses the command line. Use `FreeRTOS_CLIGetParameter()` to extract arguments:

```c
static BaseType_t _cmd_gpio(char *out, size_t out_len, const char *args)
{
    BaseType_t param_len;
    const char *id_str = FreeRTOS_CLIGetParameter(args, 1, &param_len);
    const char *val_str = FreeRTOS_CLIGetParameter(args, 2, &param_len);

    if (id_str == NULL || val_str == NULL) {
        snprintf(out, out_len, "Usage: gpio <id> <0|1>\r\n");
        return pdFALSE;
    }
    uint8_t gpio_id = (uint8_t)atoi(id_str);
    uint8_t state   = (uint8_t)atoi(val_str);
    gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_WRITE, state, 0);
    snprintf(out, out_len, "GPIO %u set to %u\r\n", gpio_id, state);
    return pdFALSE;
}

static const CLI_Command_Definition_t _gpio_cmd = {
    "gpio",
    "gpio <id> <0|1>: set a GPIO output\r\n",
    _cmd_gpio,
    2   /* 2 required arguments */
};
```

### Multi-page output

When a command produces more output than `SHELL_OUT_BUF_LEN` can hold in one call, return `pdTRUE` to signal that `FreeRTOS_CLIProcessCommand()` should be called again. The shell task loops until `pdFALSE`:

```c
static uint8_t _page = 0;

static BaseType_t _cmd_list(char *out, size_t out_len, const char *args)
{
    if (_page == 0) snprintf(out, out_len, "--- page 1 ---\r\n…");
    else            snprintf(out, out_len, "--- page 2 ---\r\n…");
    _page++;
    if (_page >= 2) { _page = 0; return pdFALSE; }   /* done */
    return pdTRUE;   /* more pages */
}
```

Use a `static` page counter that is reset to `0` when the last page is emitted.

---

## Line Editing

The shell task processes the following control characters before submitting a line to FreeRTOS+CLI:

| Input | Action |
|-------|--------|
| `CR` (`\r`) or `LF` (`\n`) | Submit line to `FreeRTOS_CLIProcessCommand()` |
| `BS` (`0x08`) or `DEL` (`0x7F`) | Erase last character; echo `\b \b` to terminal |
| `Ctrl-C` (`0x03`) | Clear the line buffer; emit `^C\r\n` and re-prompt |
| All other printable characters | Append to line buffer; echo character back |

Lines longer than `SHELL_LINE_BUF_LEN - 1` are silently truncated.

---

## printk Integration

`printk()` is independent of the shell. It writes to the ring buffer for `COMM_PRINTK_HW_ID` (defined in `conf_os.h`):

```c
/* conf_os.h */
#define COMM_PRINTK_HW_ID    (0)   /* UART_DEBUG (USART1) */
#define UART_SHELL_HW_ID     (1)   /* UART_APP   (USART2) */
```

With separate UARTs, debug log output (`printk`) and interactive shell input/output (`os_shell_mgmt`) are completely independent. Connect a terminal to USART2 for the shell; connect a second terminal or logic analyser to USART1 for log output.

`printk()` accepts `printf`-style variadic arguments:

```c
printk("boot: core @ %lu MHz\n", (unsigned long)(SystemCoreClock / 1000000));
printk("sensor[%d] temp = %d°C\n", sensor_id, temp);
```

The formatted string is assembled with `vsnprintf()` into a static buffer, then copied byte-by-byte into the ring buffer under a critical section. `drv_uart_tx_kick(COMM_PRINTK_HW_ID)` enables TXEIE to drain it.

---

## File Map

| File | Role |
|------|------|
| `include/services/os_shell_management.h` | Public API: `os_shell_mgmt_start()`, `os_shell_register_command()` |
| `services/os_shell_management.c` | Shell task, line editor, built-in commands, `_shell_write()` |
| `kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.h` | FreeRTOS+CLI public API |
| `kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c` | FreeRTOS+CLI implementation |
| `include/os/kernel_syscall.h` | `printk(const char *fmt, ...)` declaration |
| `kernel/syscalls.c` | `printk()` implementation (vsnprintf + ring buffer + tx_kick) |
| `config/conf_os.h` | `UART_SHELL_HW_ID`, `SHELL_LINE_BUF_LEN`, `SHELL_OUT_BUF_LEN`, `INC_SERVICE_OS_SHELL_MGMT` |
| `app/board/stm32f411_devboard.xml` | `role="shell"` on `UART_APP` |
| `app/board/board_device_ids.h` | `BOARD_UART_SHELL_ID` (generated) |

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
