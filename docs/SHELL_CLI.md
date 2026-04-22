# OS Shell CLI — FreeRTOS-OS

An interactive command-line interface running over UART_APP (USART2, PA2/PA3), built on FreeRTOS+CLI. Connect with PuTTY or any serial terminal at **115200 8N1, no flow control**.

---

## Table of Contents

- [Connecting with PuTTY](#connecting-with-putty)
  - [Windows](#windows)
  - [Linux](#linux)
  - [First Boot Sequence](#first-boot-sequence)
- [Architecture](#architecture)
- [I/O Paths in Detail](#io-paths-in-detail)
- [Shell UART Assignment](#shell-uart-assignment)
- [Configuration](#configuration)
- [Built-in Commands](#built-in-commands)
- [Registering Commands](#registering-commands)
- [Line Editing](#line-editing)
- [printk vs Shell](#printk-vs-shell)
- [File Map](#file-map)

---

## Connecting with PuTTY

### Windows

1. Find the COM port number: **Device Manager → Ports (COM & LPT)** — look for your USB-to-serial adapter (e.g. CH340, CP2102, FTDI). Note the COM number, e.g. `COM5`.

2. Open PuTTY. On the **Session** page:
   - Connection type: **Serial**
   - Serial line: `COM5` (replace with your port)
   - Speed: `115200`

3. Navigate to **Connection → Serial**:

   | Setting | Value |
   |---|---|
   | Speed (baud) | 115200 |
   | Data bits | 8 |
   | Stop bits | 1 |
   | Parity | None |
   | Flow control | **None** |

4. (Optional) **Terminal → Local echo**: set to **Force off** — the firmware echoes your keystrokes itself.

5. (Optional) **Terminal → Local line editing**: set to **Force off** — the firmware handles line editing (backspace, Ctrl-C).

6. Click **Open**. Power-cycle or reset the board. After ~5 seconds the banner appears:

   ```
   === FreeRTOS-OS Shell ===
   Type 'help' for available commands.

   >
   ```

---

### Linux

Connect the USB-to-serial adapter (`/dev/ttyUSB0` or `/dev/ttyACM0`):

**Option A — PuTTY:**
```bash
putty -serial /dev/ttyUSB0 -sercfg 115200,8,n,1,N
```

**Option B — screen:**
```bash
screen /dev/ttyUSB0 115200
# Exit: Ctrl-A then K, then Y
```

**Option C — minicom:**
```bash
minicom -D /dev/ttyUSB0 -b 115200
# Disable hardware flow control: Ctrl-A O → Serial port setup → F = No
# Exit: Ctrl-A X
```

**Option D — picocom (recommended, minimal):**
```bash
picocom -b 115200 --echo /dev/ttyUSB0
# Exit: Ctrl-A Ctrl-X
```

If you get a **Permission denied** error on `/dev/ttyUSB0`:
```bash
sudo usermod -aG dialout $USER
# Log out and back in, or:
newgrp dialout
```

---

### First Boot Sequence

The shell has a startup delay to let the UART driver initialise first:

```
T + 0 s   MCU reset / power-on
T + 4 s   uart_mgmt_thread wakes, initialises USART1 and USART2
T + 5 s   os_shell_task wakes, prints banner, enters read loop
```

```
=== FreeRTOS-OS Shell ===
Type 'help' for available commands.

>
```

Type `help` and press Enter to list all registered commands.

---

### Quick Session Example

```
> help
help
  List all registered commands.
version
  Print firmware version and build date.
uptime
  Print system uptime in milliseconds.
reboot
  Trigger a software reset (NVIC_SystemReset).
heap
  Show FreeRTOS heap: total, used, free, min-ever-free, malloc failures.
tasks
  List all OS tasks with state and stack high-watermark (5 s snapshot).

> version
FreeRTOS-OS v1.0  built Apr 22 2026 03:10:00
Kernel: FreeRTOS V202212.00

> heap
Heap usage:
  Total  :  80000 B
  Used   :  14720 B
  Free   :  65280 B
  MinFree:  64912 B  (min ever free)
  Errors : 0 malloc failure(s)

> tasks
Tasks  (4 registered)
  ID  Name             State      StackHWM
  ----------------------------------------
   1  task_mgr         blocked    112 w
   2  uart_mgmt        blocked    256 w
   3  heartbeat        blocked    128 w
   4  btn              blocked    128 w
  [T] Tmr Svc          blocked    256 w  (timer daemon)
  Stack overflow events: 0

> uptime
Uptime: 12438 ms  (12 s)

> reboot
Rebooting...
```

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
└──────────┬────────────────────────────┬──────────────────────┘
           │ RX ring buffer poll        │ TX ring buffer write
┌──────────▼──────────┐      ┌──────────▼──────────────────────┐
│  _os_shell_task()   │      │  global_uart_tx_mqueue[1]        │
│  (services/)        │      │  drv_uart_tx_kick(UART_APP)      │
└──────────┬──────────┘      └──────────┬──────────────────────┘
           │                            │  TXEIE enabled
┌──────────▼──────────┐      ┌──────────▼──────────────────────┐
│ global_uart_rx_     │      │  TXE ISR → USART2->DR            │
│ mqueue[1] (ring buf)│      │  (TXEIE disabled when empty)     │
└──────────┬──────────┘      └──────────────────────────────────┘
           │  filled by RXNE ISR
┌──────────▼──────────────────────────────────────────────────┐
│               USART2_IRQHandler  (NVIC priority 5)           │
│  RXNE: hal_uart_stm32_irq_handler → ringbuffer_putchar       │
│  TXE:  drv_uart_tx_get_next_byte → USART2->DR                │
└─────────────────────────────────────────────────────────────┘
             ▲
             │  PA2 TX → USB-to-Serial → /dev/ttyUSB0 → PuTTY
             │  PA3 RX ← USB-to-Serial ← /dev/ttyUSB0 ← PuTTY
```

The shell task never interacts with the `uart_mgmt` management queue — it writes directly into the TX ring buffer, and reads directly from the RX ring buffer. This means:

- Shell output is never delayed by pending `uart_mgmt` TX requests from other tasks.
- `printk()` (UART_DEBUG / USART1) and shell I/O (UART_APP / USART2) are on separate UARTs and do not interfere.

---

## I/O Paths in Detail

### RX Path — UART ISR to Shell Task

```
USART2_IRQHandler()  (RXNE flag)
  └─ hal_uart_stm32_irq_handler(USART2)
       ├─ reads DR → byte
       ├─ ringbuffer_putchar(global_uart_rx_mqueue[1], byte)
       └─ irq_desc chain → uart_mgmt _uart_rx_cb (also runs, stores same byte)

_os_shell_task()
  └─ for (;;):
       rx_rb = ipc_mqueue_get_handle(global_uart_rx_mqueue[UART_SHELL_HW_ID])
       if ringbuffer_getchar(rx_rb, &byte) == 0: vTaskDelay(5 ms); continue
       process byte → line buffer → FreeRTOS_CLIProcessCommand()
```

The 5 ms yield when empty keeps CPU load near zero for interactive use.

### TX Path — Shell Task to UART Wire

```
_shell_write(str, len)
  ├─ tx_rb = ipc_mqueue_get_handle(global_uart_tx_mqueue[UART_SHELL_HW_ID])
  ├─ for each byte: ringbuffer_putchar(tx_rb, byte)   ← ISR-safe
  └─ drv_uart_tx_kick(UART_SHELL_HW_ID)
         SET_BIT(USART2->CR1, USART_CR1_TXEIE)

USART2_IRQHandler()  (TXE flag)
  └─ drv_uart_tx_get_next_byte(UART_APP, &b)          ← ISR-safe (from_isr)
       if byte available:  WRITE_REG(USART2->DR, b)
       if ring buffer empty: CLEAR_BIT(USART2->CR1, USART_CR1_TXEIE)
```

---

## Shell UART Assignment

The shell runs on **UART_APP** (dev_id = 1, USART2, PA2 TX / PA3 RX).

Set in `config/conf_os.h`:

```c
#define UART_SHELL_HW_ID    (1)   /* UART_APP — USART2, PA2/PA3, /dev/ttyUSB0 */
#define COMM_PRINTK_HW_ID   (0)   /* UART_DEBUG — USART1, PA9/PA10, ST-Link VCP */
```

`printk()` output goes to UART_DEBUG (USART1), completely separate from the shell UART. Attach a second serial terminal (or the ST-Link virtual COM port) to see `printk` log output.

---

## Configuration

All in `config/conf_os.h`:

| Macro | Default | Description |
|---|---|---|
| `INC_SERVICE_OS_SHELL_MGMT` | `1` | `1` = compile and start the shell task |
| `UART_SHELL_HW_ID` | `1` | `dev_id` of the shell UART |
| `SHELL_LINE_BUF_LEN` | `128` | Maximum input line length (bytes, including NUL) |
| `SHELL_OUT_BUF_LEN` | `512` | Output buffer per `FreeRTOS_CLIProcessCommand()` call |
| `TIME_OFFSET_OS_SHELL_MGMT` | `5000` ms | Startup delay — must be > `TIME_OFFSET_SERIAL_MANAGEMENT` |
| `PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE` | `1024` words | Shell task stack |
| `PROC_SERVICE_OS_SHELL_MGMT_PRIORITY` | `1` | Shell task FreeRTOS priority |

---

## Built-in Commands

| Command  | Arguments | Description |
|---|---|---|
| `help`   | none | Lists all registered commands |
| `version`| none | Firmware version and build date |
| `uptime` | none | System uptime in ms and seconds |
| `reboot` | none | Software reset via `NVIC_SystemReset()` |
| `heap`   | none | FreeRTOS heap: total / used / free / min-ever-free / fault count |
| `tasks`  | none | Per-task: ID, name, state, stack high-watermark |

---

## Registering Commands

Call `os_shell_mgmt_start()` from `app_main()`. Register extra commands before or after — FreeRTOS+CLI's linked list is safe to modify at any time before the shell processes input.

### Simple command (no arguments)

```c
#include <services/os_shell_management.h>

static BaseType_t _cmd_temp_fn(char *out, size_t len, const char *args)
{
    (void)args;
    int32_t t = sensor_read_temperature();
    snprintf(out, len, "Temperature: %ld C\r\n", (long)t);
    return pdFALSE;   /* single-shot output */
}

static const CLI_Command_Definition_t _cmd_temp = {
    "temp",
    "temp\r\n  Read on-chip temperature sensor.\r\n",
    _cmd_temp_fn, 0
};

int app_main(void)
{
    os_shell_register_command(&_cmd_temp);
    os_shell_mgmt_start();
    return 0;
}
```

### Command with arguments

Use `FreeRTOS_CLIGetParameter()` to extract space-separated arguments:

```c
static BaseType_t _cmd_gpio_fn(char *out, size_t len, const char *args)
{
    BaseType_t plen;
    const char *id_s  = FreeRTOS_CLIGetParameter(args, 1, &plen);
    const char *val_s = FreeRTOS_CLIGetParameter(args, 2, &plen);

    if (!id_s || !val_s) {
        snprintf(out, len, "Usage: gpio <id> <0|1>\r\n");
        return pdFALSE;
    }
    gpio_mgmt_post((uint8_t)atoi(id_s), GPIO_MGMT_CMD_WRITE, (uint8_t)atoi(val_s), 0);
    snprintf(out, len, "GPIO %s → %s\r\n", id_s, val_s);
    return pdFALSE;
}

static const CLI_Command_Definition_t _cmd_gpio = {
    "gpio",
    "gpio <id> <0|1>\r\n  Set a GPIO output high or low.\r\n",
    _cmd_gpio_fn, 2
};
```

### Multi-page output

Return `pdTRUE` to request another call; `pdFALSE` to signal done. Use a `static` counter that resets on the final return:

```c
static int16_t _page = 0;

static BaseType_t _cmd_list_fn(char *out, size_t len, const char *args)
{
    (void)args;
    if (_page == 0) snprintf(out, len, "--- page 1 ---\r\n…\r\n");
    else            snprintf(out, len, "--- page 2 ---\r\n…\r\n");

    _page++;
    if (_page >= 2) { _page = 0; return pdFALSE; }
    return pdTRUE;
}
```

---

## Line Editing

| Keystroke | Action |
|---|---|
| Any printable character | Appended to line buffer; echoed to terminal |
| `Enter` (`CR` or `LF`) | Submit line to `FreeRTOS_CLIProcessCommand()` |
| `Backspace` (`0x08`) | Erase last character; sends `\b \b` to terminal |
| `Delete` (`0x7F`) | Same as Backspace |
| `Ctrl-C` (`0x03`) | Clear line buffer; prints `^C` and re-prompts |

Lines longer than `SHELL_LINE_BUF_LEN - 1` bytes are silently truncated.

**PuTTY note:** If Backspace doesn't work, go to **Terminal → Keyboard → The Backspace key** and switch between `Control-H` and `Control-? (127)`. The firmware handles both `0x08` and `0x7F`.

---

## printk vs Shell

`printk()` and the shell are on separate UARTs and completely independent:

| | `printk()` | Shell |
|---|---|---|
| UART | UART_DEBUG — USART1, PA9/PA10 | UART_APP — USART2, PA2/PA3 |
| Terminal | ST-Link VCP (e.g. `/dev/ttyACM0`) | USB-to-Serial (e.g. `/dev/ttyUSB0`) |
| Direction | TX only — log output | TX (output) + RX (keyboard input) |
| API | `printk("fmt %d\n", val)` | FreeRTOS+CLI commands |
| Config key | `COMM_PRINTK_HW_ID = 0` | `UART_SHELL_HW_ID = 1` |

```c
/* conf_os.h — separation of debug log and interactive shell */
#define COMM_PRINTK_HW_ID   (0)   /* UART_DEBUG — USART1 — log output  */
#define UART_SHELL_HW_ID    (1)   /* UART_APP   — USART2 — shell I/O   */
```

---

## File Map

| File | Role |
|---|---|
| `include/services/os_shell_management.h` | Public API: `os_shell_mgmt_start()`, `os_shell_register_command()` |
| `services/os_shell_management.c` | Shell task, line editor, built-in commands, `_shell_write()` |
| `shell/shell_task_mgmt.c` | `heap` and `tasks` commands |
| `kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.h` | FreeRTOS+CLI public API |
| `kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c` | FreeRTOS+CLI implementation |
| `include/os/kernel_syscall.h` | `printk(const char *fmt, ...)` declaration |
| `config/conf_os.h` | `UART_SHELL_HW_ID`, `COMM_PRINTK_HW_ID`, `SHELL_LINE_BUF_LEN`, `SHELL_OUT_BUF_LEN` |

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
