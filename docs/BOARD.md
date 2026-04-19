# Board Configuration — FreeRTOS-OS

This document explains how hardware board configuration flows from a user-authored XML file all the way through code generation, compilation, driver registration, and application callback use.

---

## Table of Contents

- [Concept](#concept)
- [Directory Layout](#directory-layout)
- [End-to-End Flow](#end-to-end-flow)
- [XML Board Description](#xml-board-description)
  - [Root Element](#root-element)
  - [UART / USART](#uart--usart)
  - [I2C](#i2c)
  - [SPI](#spi)
  - [GPIO](#gpio)
  - [Pin Attribute Reference](#pin-attribute-reference)
- [Code Generator](#code-generator)
  - [Generator Architecture](#generator-architecture)
  - [VendorCodegen Class Hierarchy](#vendorcodegen-class-hierarchy)
  - [STM32Codegen Attribute Mappings](#stm32codegen-attribute-mappings)
  - [InfineonCodegen Notes](#infineonecodegen-notes)
  - [MicrochipCodegen Notes](#microchipcodegen-notes)
- [Generated Files](#generated-files)
  - [board_device_ids.h](#board_device_idsh)
  - [board_handles.h](#board_handlesh)
  - [board_config.c](#board_configc)
- [Makefile Integration](#makefile-integration)
- [Driver Registration Flow](#driver-registration-flow)
- [Application Callback Registration](#application-callback-registration)
- [Adding a New Board](#adding-a-new-board)
- [Adding a New Vendor Backend](#adding-a-new-vendor-backend)

---

## Concept

The board configuration system is analogous to a Linux Device Tree, but resolved entirely at compile time as constant C structs.

Instead of scattering pin assignments and peripheral parameters across multiple C files, the user defines all hardware peripherals once in a single XML file. The code generator translates this XML into C artifacts that become the single source of truth for drivers, management threads, and application code.

**Board files belong to the application, not the OS.** Everything board-specific lives under `app/board/`. The OS only owns the stable API header (`include/board/board_config.h`) that defines the types and function signatures.

---

## Directory Layout

```
FreeRTOS-OS-App/
│
├── app/
│   └── board/
│       ├── stm32f411_devboard.xml     ← user-authored board description
│       ├── board_config.c             ← generated — do not edit
│       ├── board_device_ids.h         ← generated — do not edit
│       └── board_handles.h            ← generated — do not edit
│
└── FreeRTOS-OS/
    ├── include/
    │   └── board/
    │       ├── board_config.h         ← stable OS API (hand-written, not generated)
    │       ├── board_device_ids.h     ← stub — zero counts for standalone kernel builds
    │       └── board_handles.h        ← stub — empty for standalone kernel builds
    └── scripts/
        └── gen_board_config.py        ← code generator
```

**Include path resolution** — when `APP_DIR` is set (i.e. `make app`), `-I$(APP_DIR)` is prepended to the compiler include search path. This ensures `<board/board_device_ids.h>` and `<board/board_handles.h>` resolve to the real generated files in `app/board/`, shadowing the stubs in `FreeRTOS-OS/include/board/`. For a standalone `make kernel` build (no app), the stubs are used so the OS still compiles with zero peripheral counts.

---

## End-to-End Flow

```
app/board/<board>.xml          (user authors / edits)
        │
        │  make board-gen APP_DIR=../app
        │  (python3 scripts/gen_board_config.py app/board/<board>.xml
        │           --outdir app/board --incdir app/board)
        ▼
┌───────────────────────────────────────────────────────┐
│  gen_board_config.py                                  │
│  ├── Parses XML with ElementTree                      │
│  ├── Selects VendorCodegen backend (STM / Infineon …) │
│  ├── Translates XML attributes → HAL constants        │
│  └── Emits three files ────────────────────────────── │
└───────────────────────────────────────────────────────┘
        │                │                │
        ▼                ▼                ▼
app/board/          app/board/       app/board/
board_device_ids.h  board_handles.h  board_config.c
  #define UART_DEBUG 0  extern huart1   _uart_table[]
  #define BOARD_UART_COUNT 2            _gpio_table[]
  …                                     board_get_config()
                                        board_uart_register_rx_cb()
                                        …
        │                │                │
        └────────────────┴────────────────┘
                         │  #include <board/board_device_ids.h>
                         │  #include <board/board_config.h>
                         ▼
              include/config/mcu_config.h
                #define NO_OF_UART  BOARD_UART_COUNT
                (re-exports counts for backward compatibility)
                         │
                         ▼
           drivers/drv_uart.c   drv_iic.c   drv_spi.c   drv_gpio.c
             (array sizes come from BOARD_*_COUNT)
                         │
                         ▼
           services/uart_mgmt.c   iic_mgmt.c   spi_mgmt.c   gpio_mgmt.c
             (read board_get_config() at startup, register all peripherals
              into the generic driver layer automatically)
                         │
                         ▼
             Application code
               #include <board/board_device_ids.h>
               uart_mgmt_async_transmit(UART_DEBUG, data, len);
               board_uart_register_rx_cb(UART_DEBUG, my_rx_handler);
```

---

## XML Board Description

Board XML files live in `app/board/<board_name>.xml`. The filename (without `.xml`) is the board name used by `CONFIG_BOARD`.

### Root Element

```xml
<board name="my_board"
       vendor="STM"
       mcu="STM32F411VET6"
       description="My custom board">
  <peripherals>
    <!-- peripheral elements go here -->
  </peripherals>
</board>
```

| Attribute     | Required | Description |
|---------------|----------|-------------|
| `name`        | yes      | Board identifier — must match the filename |
| `vendor`      | yes      | `STM`, `INFINEON`, or `MICROCHIP` |
| `mcu`         | yes      | MCU part string (for comment generation) |
| `description` | no       | Free-form description |

---

### UART / USART

```xml
<uart id="UART_DEBUG"
      dev_id="0"
      instance="USART1"
      baudrate="115200"
      word_len="8"
      stop_bits="1"
      parity="none"
      mode="tx_rx">
  <tx  port="A" pin="9"  af="7" speed="very_high" pull="none"/>
  <rx  port="A" pin="10" af="7" speed="very_high" pull="none"/>
  <irq name="USART1_IRQn" priority="2"/>
</uart>
```

| Attribute    | Valid values                     | Notes |
|--------------|----------------------------------|-------|
| `id`         | any identifier string            | Becomes `#define` name in `board_device_ids.h` |
| `dev_id`     | 0-based integer                  | Index into the driver handle array |
| `instance`   | `USART1` … `USART6`              | HAL peripheral register block |
| `baudrate`   | integer                          | e.g. `9600`, `115200`, `921600` |
| `word_len`   | `8`, `9`                         | |
| `stop_bits`  | `1`, `2`                         | |
| `parity`     | `none`, `even`, `odd`            | |
| `mode`       | `tx_rx`, `tx`, `rx`              | |

Child `<tx>` / `<rx>` pin attributes — see [Pin Attribute Reference](#pin-attribute-reference).

Child `<irq>`:

| Attribute  | Description |
|------------|-------------|
| `name`     | IRQ vector name, e.g. `USART1_IRQn` |
| `priority` | NVIC pre-emption priority (0 = highest) |

---

### I2C

```xml
<i2c id="I2C_SENSOR_BUS"
     dev_id="0"
     instance="I2C1"
     clock_hz="400000"
     addr_mode="7bit"
     dual_addr="disable">
  <scl port="B" pin="6" af="4" speed="very_high" pull="none"/>
  <sda port="B" pin="7" af="4" speed="very_high" pull="none"/>
  <irq_ev name="I2C1_EV_IRQn" priority="3"/>
  <irq_er name="I2C1_ER_IRQn" priority="3"/>
</i2c>
```

| Attribute    | Valid values                                     |
|--------------|--------------------------------------------------|
| `instance`   | `I2C1`, `I2C2`, `I2C3`                           |
| `clock_hz`   | `100000` (standard mode), `400000` (fast mode)  |
| `addr_mode`  | `7bit`, `10bit`                                  |
| `dual_addr`  | `enable`, `disable`                              |

I2C pin mode is always open-drain (`GPIO_MODE_AF_OD`) — the generator sets this automatically regardless of the `pull` attribute.

Child `<irq_ev>` and `<irq_er>` are the event and error interrupt vectors.

---

### SPI

```xml
<spi id="SPI_FLASH_BUS"
     dev_id="0"
     instance="SPI1"
     mode="master"
     direction="2lines"
     data_size="8"
     clk_polarity="low"
     clk_phase="1edge"
     nss="soft"
     baud_prescaler="16"
     bit_order="msb">
  <sck  port="A" pin="5" af="5" speed="very_high" pull="none"/>
  <miso port="A" pin="6" af="5" speed="very_high" pull="none"/>
  <mosi port="A" pin="7" af="5" speed="very_high" pull="none"/>
  <!-- omit <nss_pin> for software NSS -->
  <irq name="SPI1_IRQn" priority="3"/>
</spi>
```

| Attribute        | Valid values                                  |
|------------------|-----------------------------------------------|
| `instance`       | `SPI1` … `SPI6`                               |
| `mode`           | `master`, `slave`                             |
| `direction`      | `2lines`, `1line`                             |
| `data_size`      | `8`, `16`                                     |
| `clk_polarity`   | `low`, `high`                                 |
| `clk_phase`      | `1edge`, `2edge`                              |
| `nss`            | `soft`, `hard_input`, `hard_output`           |
| `baud_prescaler` | `2`, `4`, `8`, `16`, `32`, `64`, `128`, `256` |
| `bit_order`      | `msb`, `lsb`                                  |

Add an optional `<nss_pin>` child element with the same pin attributes for hardware NSS.

---

### GPIO

```xml
<gpio id="LED_BOARD"
      dev_id="0"
      port="C" pin="13"
      mode="output_pp"
      pull="none"
      speed="low"
      active_state="low"
      initial_state="inactive"/>
```

| Attribute       | Valid values                                              |
|-----------------|-----------------------------------------------------------|
| `id`            | any identifier string (becomes `#define`)                 |
| `dev_id`        | 0-based integer                                           |
| `port`          | `A` … `E` (or as many as the MCU has)                    |
| `pin`           | `0` … `15`                                               |
| `mode`          | `output_pp`, `output_od`, `input`, `it_rising`, `it_falling`, `it_both` |
| `pull`          | `none`, `pullup`, `pulldown`                             |
| `speed`         | `low`, `medium`, `high`, `very_high`                     |
| `active_state`  | `high`, `low` — logical meaning of "active"              |
| `initial_state` | `active`, `inactive` — state applied at boot             |

---

### Pin Attribute Reference

All `<tx>`, `<rx>`, `<scl>`, `<sda>`, `<sck>`, `<miso>`, `<mosi>`, `<nss_pin>` elements share these attributes:

| Attribute | Description |
|-----------|-------------|
| `port`    | GPIO port letter (`A` … `E`) |
| `pin`     | GPIO pin number (`0` … `15`) |
| `af`      | Alternate function number from the MCU datasheet AF table |
| `speed`   | `low` \| `medium` \| `high` \| `very_high` |
| `pull`    | `none` \| `pullup` \| `pulldown` |

---

## Code Generator

### Generator Architecture

`scripts/gen_board_config.py` is a single-file Python 3 script. Its structure:

```
gen_board_config.py
├── _BANNER / _INC_GUARD_OPEN/_CLOSE   — file header templates
├── VendorCodegen (abstract base class)
│   ├── STM32Codegen                   — STM32 HAL constant translation
│   ├── InfineonCodegen                — XMC baremetal stubs
│   └── MicrochipCodegen               — SERCOM/USART stubs
└── BoardConfigGen
    ├── generate(c_outdir, h_outdir)   — top-level entry point
    ├── _gen_board_config_c()
    │   ├── _emit_uart_table()
    │   ├── _emit_iic_table()
    │   ├── _emit_spi_table()
    │   ├── _emit_gpio_table()
    │   ├── _emit_gpio_clk_enable()
    │   ├── _emit_callback_tables()
    │   └── _emit_board_api()
    ├── _gen_device_ids_h()
    └── _gen_board_handles_h()
```

**CLI:**

```
python3 scripts/gen_board_config.py <xml>
    [--outdir / -o <dir>]   directory for board_config.c   (default: FreeRTOS-OS/boards/<board>/)
    [--incdir / -i <dir>]   directory for board_device_ids.h and board_handles.h
                            (default: FreeRTOS-OS/include/board/)
```

When invoked by `make board-gen` both flags are set to `$(APP_DIR)/board`:

```
python3 scripts/gen_board_config.py app/board/stm32f411_devboard.xml \
    --outdir app/board \
    --incdir app/board
```

### VendorCodegen Class Hierarchy

`VendorCodegen` is an abstract base that defines the translation interface. Vendor is selected automatically from the XML `vendor` attribute:

```python
vendor_map = {
    'STM':       STM32Codegen,
    'INFINEON':  InfineonCodegen,
    'MICROCHIP': MicrochipCodegen,
}
cg = vendor_map[vendor.upper()]()
```

Each subclass implements:

```python
class VendorCodegen:
    def gpio_port(self, letter: str) -> str: ...     # "A" → "GPIOA" / "XMC_GPIO_PORT_A"
    def gpio_pin(self, num: str) -> str: ...          # "9" → "GPIO_PIN_9"
    def gpio_speed(self, speed: str) -> str: ...
    def gpio_pull(self, pull: str) -> str: ...
    def gpio_af_mode(self, is_od=False) -> str: ...
    def gpio_out_mode(self, is_od=False) -> str: ...
    def gpio_in_mode(self) -> str: ...
    def gpio_it_mode(self, edge: str) -> str: ...
    def gpio_active_state(self, state: str) -> str: ...
    def af_constant(self, af_num: str, instance: str) -> str: ...
    def uart_wordlen / stopbits / parity / mode(…): ...
    def i2c_addr_mode / dual_addr(…): ...
    def spi_mode / direction / datasize / polarity / phase / nss / prescaler / bitorder(…): ...
    def periph_clk_fn(self, periph_type, instance): ...
    def gpio_clk_enable_body(self, ports: set) -> str: ...
    def device_includes(self) -> list: ...
```

### STM32Codegen Attribute Mappings

| XML value | Generated C constant |
|-----------|----------------------|
| `speed="very_high"` | `GPIO_SPEED_FREQ_VERY_HIGH` |
| `pull="pullup"` | `GPIO_PULLUP` |
| `mode="output_pp"` | `GPIO_MODE_OUTPUT_PP` |
| `mode="it_rising"` | `GPIO_MODE_IT_RISING` |
| `active_state="low"` | `GPIO_PIN_RESET` |
| `word_len="8"` | `UART_WORDLENGTH_8B` |
| `parity="none"` | `UART_PARITY_NONE` |
| `addr_mode="7bit"` | `I2C_ADDRESSINGMODE_7BIT` |
| `mode="master"` (SPI) | `SPI_MODE_MASTER` |
| `baud_prescaler="16"` | `SPI_BAUDRATEPRESCALER_16` |
| `af="7"` for `USART1` | `GPIO_AF7_USART1` |

Clock enables are emitted as static inline wrappers:

```c
static void _usart1_clk_en(void) { __HAL_RCC_USART1_CLK_ENABLE(); }
```

`board_gpio_clk_enable(GPIO_TypeDef *port)` is generated as a chain of `if / else if` comparisons covering every port used by any GPIO pin across all peripheral types.

### InfineonCodegen Notes

- GPIO port → `XMC_GPIO_PORT_x`
- GPIO pins use bare integer strings
- Clock enable bodies emit `TODO` comments (XMC clock module varies per family)
- Interrupt mode emits a `TODO: ERU config needed` comment

### MicrochipCodegen Notes

- Peripheral instances become `SERCOM0` … `SERCOMn` style stubs
- All HAL constant translations emit `TODO` comments
- Intended as a starting scaffold; fill in actual constants per family

---

## Generated Files

All three generated files are placed in `app/board/` and are regenerated on every `make board-gen`. **Do not edit them** — edit the XML instead.

### board_device_ids.h

`app/board/board_device_ids.h`

```c
/* UART device IDs */
#define UART_DEBUG           0
#define UART_APP             1
#define BOARD_UART_COUNT     2

/* I2C device IDs */
#define I2C_SENSOR_BUS       0
#define BOARD_IIC_COUNT      1

/* SPI device IDs */
#define SPI_FLASH_BUS        0
#define BOARD_SPI_COUNT      1

/* GPIO device IDs */
#define LED_BOARD            0
#define BTN_USER             1
#define LED_STATUS           2
#define BOARD_GPIO_COUNT     3
```

`BOARD_*_COUNT` constants drive static array sizes in the driver layer. `include/config/mcu_config.h` re-exports them as `NO_OF_UART`, `NO_OF_IIC`, `NO_OF_SPI`, `NO_OF_GPIO` for driver source compatibility.

### board_handles.h

`app/board/board_handles.h`

Declares (and defines the storage for) the HAL peripheral handle globals. Included by `device.h` so every translation unit that uses a HAL handle can reach it.

```c
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f4xx_hal_i2c.h"
extern I2C_HandleTypeDef hi2c1;
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f4xx_hal_spi.h"
extern SPI_HandleTypeDef hspi1;
#endif
```

The actual definitions (`UART_HandleTypeDef huart1;` etc.) live in `board_config.c`, wrapped in the same `#ifdef` guards.

> **Standalone kernel build (no APP_DIR):** `FreeRTOS-OS/include/board/board_handles.h` is a stub containing only the include guard. This lets the OS compile without an application attached.

### board_config.c

`app/board/board_config.c`

Compiled as an **application source** (via `app-obj-y += board/board_config.o` in `app/Makefile`). Contains:

1. **HAL handle definitions** — `UART_HandleTypeDef huart1;` etc., guarded by `HAL_xxx_MODULE_ENABLED`
2. **Peripheral clock-enable wrappers** — one static function per peripheral instance
3. **Peripheral descriptor tables** — `_uart_table[]`, `_iic_table[]`, `_spi_table[]`, `_gpio_table[]`; each entry is a fully initialised descriptor struct with all HAL constants filled in; each table is wrapped in `#ifdef HAL_xxx_MODULE_ENABLED`
4. **Top-level `g_board_config`** — the one `board_config_t` instance returned by `board_get_config()`
5. **`board_get_config()`** — returns `&g_board_config`
6. **`board_find_uart/iic/spi()`** — linear search helpers returning a descriptor by peripheral instance pointer
7. **`board_gpio_clk_enable()`** — if/else chain enabling the RCC clock for a given GPIO port
8. **Mutable callback tables** — `board_uart_cbs_t _uart_cbs[BOARD_UART_COUNT]` etc., zero-initialised
9. **Callback registration and getter functions** — `board_uart_register_rx_cb()`, `board_get_uart_cbs()`, etc.

Example UART table entry:

```c
static const board_uart_desc_t _uart_table[BOARD_UART_COUNT] = {
    [0] = {
        .dev_id            = 0,
        .instance          = USART1,
        .baudrate          = 115200,
        .word_len          = UART_WORDLENGTH_8B,
        .stop_bits         = UART_STOPBITS_1,
        .parity            = UART_PARITY_NONE,
        .mode              = UART_MODE_TX_RX,
        .tx_pin = {
            .port      = GPIOA, .pin = GPIO_PIN_9,
            .mode      = GPIO_MODE_AF_PP, .pull = GPIO_NOPULL,
            .speed     = GPIO_SPEED_FREQ_VERY_HIGH,
            .alternate = GPIO_AF7_USART1,
        },
        .rx_pin = { … },
        .irqn              = USART1_IRQn,
        .irq_priority      = 2,
        .periph_clk_enable = _usart1_clk_en,
    },
};
```

---

## Makefile Integration

Board files live in the application tree. The build system wires them in automatically when `APP_DIR` is provided.

### Key variables in `FreeRTOS-OS/Makefile`

```makefile
CONFIG_BOARD    ?= stm32f411_devboard

ifdef APP_DIR
BOARD_XML       := $(APP_DIR)/board/$(CONFIG_BOARD).xml
BOARD_BSP_C     := $(APP_DIR)/board/board_config.c
BOARD_BSP_H     := $(APP_DIR)/board/board_device_ids.h
BOARD_HANDLES_H := $(APP_DIR)/board/board_handles.h
# Prepend so app/board/ headers shadow the include/board/ stubs
INCLUDES        += -I$(APP_DIR)    # added first, before arch and OS includes
endif
```

### board-gen recipe

```makefile
$(BOARD_BSP_C) $(BOARD_BSP_H) $(BOARD_HANDLES_H): $(BOARD_XML)
    python3 scripts/gen_board_config.py $< \
        --outdir $(APP_DIR)/board \
        --incdir $(APP_DIR)/board
```

### `app/Makefile` fragment

```makefile
app-obj-y += board/board_config.o   # compiled as an app source
```

### Make targets

| Target | Command | Effect |
|--------|---------|--------|
| Generate BSP | `make board-gen APP_DIR=../app` | Runs generator, writes files into `app/board/` |
| Full app build | `make app` (from project root) | Auto-generates BSP if XML is newer, then builds |
| Different board | `make board-gen CONFIG_BOARD=my_board APP_DIR=../app` | Select a different XML file |
| Clean | `make clean` (from project root) | Removes build artifacts and generated BSP files |

> `make board-gen` without `APP_DIR` prints an error and exits. The top-level `Makefile` passes `APP_DIR=../app` automatically for all `app` and `board-gen` targets.

---

## Driver Registration Flow

When the OS starts, the management service threads initialise in order (governed by startup delay offsets). Each thread reads the board config and registers its peripherals into the generic driver layer automatically — no user code required:

```
OS start
  └─ kernel_service_core_start()
       ├─ uart_mgmt_start()   → creates uart_mgmt task
       ├─ iic_mgmt_start()    → creates iic_mgmt task
       ├─ spi_mgmt_start()    → creates spi_mgmt task
       └─ gpio_mgmt_start()   → creates gpio_mgmt task

uart_mgmt task startup:
  os_thread_delay(TIME_OFFSET_UART_MANAGEMENT)
  ops = hal_uart_stm32_get_ops()
  bc  = board_get_config()
  for i in 0..bc->uart_count:
      d = &bc->uart_table[i]
      h = drv_uart_get_handle(d->dev_id)
      hal_uart_stm32_set_config(h, d->instance, …)   ← stores params in h->hw
      drv_uart_register(d->dev_id, ops, d->baudrate, timeout)
          └─ ops->hw_init(h)                          ← HAL_UART_Init() runs here
  → message loop …

gpio_mgmt task startup:
  ops = hal_gpio_stm32_get_ops()
  bc  = board_get_config()
  for i in 0..bc->gpio_count:
      d = &bc->gpio_table[i]
      h = drv_gpio_get_handle(d->dev_id)
      board_gpio_clk_enable(d->port)                 ← __HAL_RCC_GPIOx_CLK_ENABLE()
      hal_gpio_stm32_set_config(h, d->port, d->pin,
                                d->mode, d->pull,
                                d->speed, d->active_state)
      drv_gpio_register(d->dev_id, ops)
          └─ ops->hw_init(h)                          ← HAL_GPIO_Init() runs here
      if initial_state == active: h->ops->set(h)
  → message loop …
```

The ordering constraint for GPIO is critical: the RCC clock must be enabled before `HAL_GPIO_Init`, and `hal_gpio_stm32_set_config` must be called before `drv_gpio_register` (which calls `hw_init`). `gpio_mgmt` handles this sequence correctly.

---

## Application Callback Registration

The generated `board_config.c` holds a mutable callback table for each peripheral class. Applications register functions that are called directly from driver ISR/DMA completion context:

### UART callbacks

```c
#include <board/board_device_ids.h>
#include <board/board_config.h>

static void my_rx_handler(uint8_t dev_id, uint8_t byte)
{
    /* process byte — keep this short, ISR context */
}

static void my_tx_done(uint8_t dev_id)  { /* signal a task if needed */  }
static void my_error  (uint8_t dev_id, uint32_t error_code) { /* log */ }

void app_setup_callbacks(void)
{
    board_uart_register_rx_cb      (UART_DEBUG, my_rx_handler);
    board_uart_register_tx_done_cb (UART_DEBUG, my_tx_done);
    board_uart_register_error_cb   (UART_DEBUG, my_error);
}
```

### I2C callbacks

```c
board_iic_register_done_cb (I2C_SENSOR_BUS, my_iic_done);
board_iic_register_error_cb(I2C_SENSOR_BUS, my_iic_error);
```

### SPI callbacks

```c
board_spi_register_done_cb(SPI_FLASH_BUS, my_spi_done);
```

### GPIO interrupt callbacks

```c
static void btn_irq(uint8_t dev_id) { /* debounce and signal task */ }

board_gpio_register_irq_cb(BTN_USER, btn_irq);
```

### Callback dispatch chain (UART RX example)

```
USART1_IRQHandler() [hal_it_stm32.c]
  └─ hal_uart_stm32_irq_handler(USART1)
       └─ HAL_UART_IRQHandler(&h->hw.huart)
            └─ HAL_UART_RxCpltCallback(huart)
                 └─ drv_uart_rx_isr_dispatch(dev_id, rx_byte)
                      ├─ pushes byte into ring buffer (IPC queue for os_read())
                      └─ board_get_uart_cbs(dev_id)->on_rx_byte(dev_id, byte)
                           └─ application my_rx_handler()
```

---

## Adding a New Board

1. **Create the XML file** — copy `app/board/stm32f411_devboard.xml` to `app/board/<my_board>.xml` and edit to match your hardware.

2. **Generate the BSP** (from the project root):

   ```bash
   make board-gen CONFIG_BOARD=my_board
   ```

   This creates (or overwrites):
   - `app/board/board_config.c`
   - `app/board/board_device_ids.h`
   - `app/board/board_handles.h`

3. **Build:**

   ```bash
   make app CONFIG_BOARD=my_board
   ```

4. **Use device IDs in application code:**

   ```c
   #include <board/board_device_ids.h>

   uart_mgmt_async_transmit(UART_DEBUG, msg, len);
   board_uart_register_rx_cb(UART_APP, my_handler);
   gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
   ```

No other source files need to change. The management threads self-register all peripherals defined in the XML at startup.

---

## Adding a New Vendor Backend

1. **Subclass `VendorCodegen`** in `scripts/gen_board_config.py`:

   ```python
   class MyVendorCodegen(VendorCodegen):
       def gpio_port(self, letter):      return f'MY_PORT_{letter}'
       def gpio_pin(self, num):          return f'MY_PIN_{num}'
       def gpio_speed(self, s):          return 'MY_SPEED_HIGH'
       # … implement all abstract methods …
       def device_includes(self):        return ['#include <my_vendor_hal.h>']
   ```

2. **Register it in the vendor map:**

   ```python
   vendor_map = {
       'STM':       STM32Codegen,
       'INFINEON':  InfineonCodegen,
       'MICROCHIP': MicrochipCodegen,
       'MYVENDOR':  MyVendorCodegen,    # add this line
   }
   ```

3. **Add a vendor HAL backend** in `drivers/hal/myvendor/` implementing `drv_uart_hal_ops_t`, `drv_iic_hal_ops_t`, `drv_spi_hal_ops_t`, `drv_gpio_hal_ops_t` — see [DRIVERS.md](DRIVERS.md) for the ops vtable interface.

4. **Guard the HAL includes** in management service source files with `#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_MYVENDOR)` blocks, following the existing pattern in `uart_mgmt.c`, `iic_mgmt.c`, etc.

5. **Set `vendor="MYVENDOR"`** in your board XML and run `make board-gen`.
