# FreeRTOS-OS — Test Plan

Verification cases for every communication peripheral and OS service, across
the supported MCU variants. Each case lists **how to run it** and the
**expected result** so a release can be signed off reproducibly.

The board files are generated from XML, so every test below also implicitly
verifies the board-config generator for that peripheral.

---

## 1. Supported variants and feature matrix

| Variant     | Core        | UART/Shell | I2C | SPI | Ethernet / ping | Notes                         |
|-------------|-------------|:----------:|:---:|:---:|:---------------:|-------------------------------|
| stm32f411   | Cortex-M4F  |     ✓      |  ✓  |  ✓  |       —         | NUCLEO/F411 devboard          |
| stm32h723   | Cortex-M7   |     ✓      |  ✓  |  ✓  |       ✓         | NUCLEO-H723ZG, LAN8742 RMII   |
| stm32u575   | Cortex-M33  |     ✓      |  ✓  |  ✓  |       —         | NUCLEO-U575ZI-Q, TrustZone    |

A feature is "present" when its `CONFIG_HAL_*_MODULE_ENABLED` and (for services)
`CONFIG_INC_SERVICE_*` are set in the variant's `examples/<variant>/kconfig.conf`
**and** the matching peripheral element exists in the board XML.

---

## 2. Build verification (requirement: all variants build)

| ID    | Command                              | Expected                                  |
|-------|--------------------------------------|-------------------------------------------|
| B-1   | `make dev-stm32f411 DEBUG=1`         | `Build complete: build/stm32f411.elf`     |
| B-2   | `make dev-stm32h723 DEBUG=1`         | `Build complete: build/stm32h723.elf`     |
| B-3   | `make dev-stm32u575 DEBUG=1`         | `Build complete: build/stm32u575.elf`     |
| B-4   | `make dev-stm32h723 DEBUG=0`         | Release image builds (LTO, `-Os`)         |

Prerequisite for B-3: HAL submodules checked out —
`git submodule update --init arch/devices/STM/stm32u5xx-hal-driver arch/devices/STM/cmsis-device-u5`

Pass criteria: each command exits 0 and prints its `Build complete` line. No
`error:` in the log (pre-existing `-Wunused`/`-Wimplicit` warnings in
`drv_dma.c`/`hal_spi_stm32.c` are tracked separately and do not fail the build).

---

## 3. Console / serial setup (all variants)

The shell and `printk` share the board's debug UART, routed to the ST-LINK
virtual COM port.

| Variant     | Debug UART | Pins      | Host device     |
|-------------|------------|-----------|-----------------|
| stm32h723   | USART3     | PD8/PD9   | `/dev/ttyACM0`  |
| stm32f411   | USART2     | PA2/PA3   | `/dev/ttyACM0`  |
| stm32u575   | LPUART1    | PG7/PG8   | `/dev/ttyACM0`  |

(The shell/debug UART is whichever `<uart role="shell">` element the board XML
declares; the table records the current devboard mapping.)

Open the port (the `clocal` flag is required, or the read blocks on carrier):

```sh
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb -echo -icrnl -ixon clocal raw
```

| ID    | Step                                   | Expected                                  |
|-------|----------------------------------------|-------------------------------------------|
| C-1   | Reset board, read console              | `[boot] FreeRTOS-OS running` banner        |
| C-2   | Observe `OS >` prompt                  | Shell prompt appears, accepts input        |

---

## 4. GPIO service (heartbeat LED)

| ID    | Step                                   | Expected                                  |
|-------|----------------------------------------|-------------------------------------------|
| G-1   | Power the board                        | `LED_BOARD` toggles at ~1 Hz (500 ms on/off) |
| G-2   | `ps` in shell                          | `heartbeat` task listed, state `S`/blocked |

GPIO mapping (`LED_BOARD`, button) comes from the board XML `<gpio>` elements.

---

## 5. UART service + shell commands

All commands run at the `OS >` prompt. Expected output abbreviated.

| ID    | Command            | Expected                                                      |
|-------|--------------------|---------------------------------------------------------------|
| U-1   | `help`             | Lists every registered command, complete, no truncation       |
| U-2   | `version`          | `FreeRTOS-OS vX.Y built <date> <time>`                          |
| U-3   | `uptime`           | `Uptime: <ms> ms (<s> s)`                                       |
| U-4   | `uname -a`         | `FreeRTOS-OS freertos-os <ver> <machine> <core>`               |
| U-5   | `uname -s/-n/-r/-m`| Prints just sysname / nodename / release / machine            |
| U-6   | `whoami`           | `root`                                                         |
| U-7   | `hostname`         | `freertos-os`                                                  |
| U-8   | `echo hello world` | `hello world` then newline                                    |
| U-9   | `echo -n abc`      | `abc` with no trailing newline (prompt follows immediately)   |
| U-10  | `clear`            | Terminal clears (ANSI `ESC[2J ESC[H`)                          |
| U-11  | `free`             | Heap table: total / used / free / min-free in bytes           |
| U-12  | `heap`             | Heap usage incl. min-ever-free and malloc-failure count       |
| U-13  | `tasks`            | Per-task table: ID, name, state, stack high-watermark         |
| U-14  | `ps`               | Linux-style: `PID STAT STACK COMMAND`, incl. timer daemon     |
| U-15  | `debug dis` / `en` | Toggles `printk` echo; confirms with a status line            |
| U-16  | `bogus`            | `Command not recognised. Enter 'help' ...`                     |
| U-17  | `reboot`           | Board resets, boot banner reappears                           |

Echo integrity check (regression for TX ring overflow): U-1 `help` must print
**all** commands with no dropped/garbled bytes. Echo must not show `[timestamp]`
prefixes (regression for the duplicate UART echo task).

---

## 6. I2C service (`iic-scan`)

Requires `CONFIG_INC_SERVICE_IIC_MGMT=y` and an `<iic>` element in the XML.

| ID    | Command            | Expected                                                      |
|-------|--------------------|---------------------------------------------------------------|
| I-1   | `iic-scan 0`       | `I2C bus 0 scan (0x00-0xFF):` header, then ACKing addresses    |
| I-2   | `iic-scan 0` (idle bus) | `No devices found.` when nothing is attached             |
| I-3   | `iic-scan 9`       | `bus 9 not available (BOARD_IIC_COUNT=...)` error             |

With a known device (e.g. an LSM303 at 0x19/0x1E) on the bus, I-1 lists those
addresses and a `Found: N device(s)` summary.

---

## 7. SPI service

SPI is exercised at the driver/board-config level (instances from `<spi>` XML).

| ID    | Step                                                  | Expected                          |
|-------|-------------------------------------------------------|-----------------------------------|
| S-1   | Build a variant with `<spi>` present                  | `BOARD_SPI_COUNT > 0`, handles emitted |
| S-2   | On stm32h723 with `CONFIG_NET=y`                      | SPI gated off (PA7 freed for RMII), `BOARD_SPI_COUNT == 0` |

S-2 verifies the mutual-exclusion guard: SPI1 MOSI (PA7) and RMII CRS_DV share a
pin, so the board XML `guard` attribute disables SPI when networking is on.

---

## 8. Ethernet / networking (stm32h723 only)

Static identity comes from `lwipopts.h`
(`NET_IP_ADDR*`, `NET_NETMASK*`, `NET_GW_ADDR*`, `NET_MAC_ADDR*`).
Default: board `192.168.1.50/24`, gateway `192.168.1.1`,
MAC `02:00:00:00:00:01`. Host assumed at `192.168.1.10`.

### 8a. Board as ICMP responder (host → board)

```sh
ip neigh del 192.168.1.50 dev <hostif>      # clear ARP first
ping -c 4 192.168.1.50
```

| ID    | Expected                                                                |
|-------|-------------------------------------------------------------------------|
| N-1   | Replies received; ≤1 lost (the cold-ARP first packet); RTT < 1 ms       |
| N-2   | `ip neigh show 192.168.1.50` → `lladdr 02:00:00:00:00:01 REACHABLE`     |

### 8b. Board as ICMP requester (board → host, shell `ping`)

| ID    | Command (at `OS >`)         | Expected                                              |
|-------|-----------------------------|------------------------------------------------------|
| N-3   | `ping 192.168.1.10 5`       | 5 replies, 0% loss, sub-ms RTT (`time=0.xxx ms`)      |
| N-4   | `ping 192.168.1.99 3`       | 3 timeouts, `100% packet loss` (no host present)     |
| N-5   | `ping notanip`              | `ping: invalid address 'notanip'`                    |
| N-6   | `ping 192.168.1.10`         | Defaults to 4 packets; ARP pre-warm → first reply OK |

N-3 RTT uses the DWT cycle counter (microsecond resolution); statistics line
shows `rtt min/avg/max`.

---

## 9. Task manager / watchdog services

| ID    | Step                                       | Expected                                  |
|-------|--------------------------------------------|-------------------------------------------|
| T-1   | `tasks` / `ps`                             | Health snapshot refreshes (≤ scan period) |
| T-2   | `heap` after running commands              | `MinFree` ≥ 0, `0 malloc failure(s)`      |
| T-3   | (if WDOG enabled) stall a monitored task   | Watchdog resets the MCU into safe state   |
| T-4   | Stack-overflow hook                        | Logs offending task, enters safe state    |

T-3/T-4 are destructive; run only in a dedicated fault-injection build.

---

## 10. Deploy + acceptance on NUCLEO-H723ZG

End-to-end sign-off on real hardware.

```sh
make dev-stm32h723 DEBUG=1
make dev-stm32h723-flash
```

| ID    | Step                                | Expected                                          |
|-------|-------------------------------------|---------------------------------------------------|
| D-1   | Flash + reset                       | Boot banner, `OS >` prompt                        |
| D-2   | Run U-1..U-17                       | All shell commands behave per §5                  |
| D-3   | Run N-1..N-2 (host → board)         | Ping responder works                              |
| D-4   | Run N-3..N-6 (board → host)         | Ping client works                                 |
| D-5   | Leave running 5 min, re-run `heap`  | No malloc failures, no stack-overflow events      |

Acceptance = D-1 through D-5 all pass on the physical board.

---

## 11. Regression checklist (issues fixed in this line of work)

| Item                                   | Guarded by |
|----------------------------------------|------------|
| PA7 SPI vs RMII pin conflict           | S-2, N-1   |
| Open TXD1 → corrupt TX frames          | N-1, N-3   |
| `help` TX-ring overflow / truncation   | U-1        |
| Duplicate UART echo `[timestamp]` noise| U-8, U-15  |
| Heartbeat console spam                 | C-1        |
| u575 build (missing HAL submodule)     | B-3        |
