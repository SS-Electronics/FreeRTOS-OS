# Board Configuration — FreeRTOS-OS

This document explains how hardware board configuration flows from a user-authored XML file all the way through code generation, compilation, driver registration, and application callback use.

---

## Table of Contents

- [Concept](#concept)
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
  - [board_config.c](#board_configc)
- [Makefile Integration](#makefile-integration)
- [Driver Registration Flow](#driver-registration-flow)
- [Application Callback Registration](#application-callback-registration)
- [Adding a New Board](#adding-a-new-board)
- [Adding a New Vendor Backend](#adding-a-new-vendor-backend)

---

## Concept

The board configuration system is analogous to a Linux Device Tree, but resolved entirely at compile time as constant C structs.

Instead of scattering pin assignments and peripheral parameters across multiple C files, the user defines all hardware peripherals once in a single XML file. The code generator translates this XML into two C artifacts:

1. `include/board/board_device_ids.h` — user-facing `#define` constants (device IDs and counts)
2. `boards/<board_name>/board_config.c` — peripheral descriptor tables, callback tables, and the board API implementation

All drivers, management threads, and application code share these two files as the single source of truth.

---

## End-to-End Flow

```
boards/<board>.xml          (user authors / edits)
        │
        │  python3 scripts/gen_board_config.py boards/<board>.xml
        ▼
┌───────────────────────────────────────────────────────┐
│  gen_board_config.py                                  │
│  ├── Parses XML with ElementTree                      │
│  ├── Selects VendorCodegen backend (STM / Infineon …) │
│  ├── Translates XML attributes → HAL constants        │
│  └── Emits two files ──────────────────────────────── │
└───────────────────────────────────────────────────────┘
        │                           │
        ▼                           ▼
include/board/              boards/<board>/
board_device_ids.h          board_config.c
  #define UART_DEBUG 0        _uart_table[]
  #define BOARD_UART_COUNT 2  _gpio_table[]
  …                           board_get_config()
                              board_uart_register_rx_cb()
                              …
        │                           │
        └──────────┬────────────────┘
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

Board XML files live in `boards/<board_name>.xml`. The filename (without `.xml`) is the board name used by `CONFIG_BOARD`.

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
├── generate_device_ids_h(root, outpath)
├── generate_board_config_c(root, cg, outdir, xml_path)
│   ├── _emit_uart_table(root, cg)
│   ├── _emit_iic_table(root, cg)
│   ├── _emit_spi_table(root, cg)
│   ├── _emit_gpio_table(root, cg)
│   ├── _emit_gpio_clk_enable(root, cg)
│   ├── _emit_callback_tables(root, cg)
│   └── _emit_board_api(root, cg)
└── main()
    ├── Parse arguments / XML
    ├── Detect vendor → instantiate VendorCodegen
    └── Call generate_device_ids_h + generate_board_config_c
```

### VendorCodegen Class Hierarchy

`VendorCodegen` is an abstract base that defines the translation interface. Each vendor subclass implements it using vendor-specific constant names:

```python
class VendorCodegen:
    def gpio_port(self, letter: str) -> str: ...     # "A" → "GPIOA" / "XMC_GPIO_PORT_A"
    def gpio_pin(self, num: str) -> str: ...          # "9" → "GPIO_PIN_9" / "9"
    def gpio_speed(self, speed: str) -> str: ...
    def gpio_pull(self, pull: str) -> str: ...
    def gpio_af_mode(self, is_od=False) -> str: ...
    def gpio_out_mode(self, is_od=False) -> str: ...
    def gpio_in_mode(self) -> str: ...
    def gpio_it_mode(self, edge: str) -> str: ...
    def gpio_active_state(self, state: str) -> str: ...
    def af_constant(self, af_num: str, instance: str) -> str: ...
    def uart_wordlen(self, bits: str) -> str: ...
    def uart_stopbits(self, n: str) -> str: ...
    def uart_parity(self, p: str) -> str: ...
    def uart_mode(self, m: str) -> str: ...
    def i2c_addr_mode(self, m: str) -> str: ...
    def i2c_dual_addr(self, m: str) -> str: ...
    def spi_mode/direction/datasize/polarity/phase/nss/prescaler/bitorder(…): ...
    def periph_clk_fn(self, periph_type, instance): ...  # returns (fn_name, fn_body)
    def gpio_clk_enable_body(self, ports: set) -> str: ...
    def device_includes(self) -> list: ...               # vendor-specific #includes
```

Vendor is selected automatically from the XML `vendor` attribute:

```python
vendor_map = {
    'STM':       STM32Codegen,
    'INFINEON':  InfineonCodegen,
    'MICROCHIP': MicrochipCodegen,
}
cg = vendor_map[vendor.upper()]()
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

### board_device_ids.h

`include/board/board_device_ids.h` — **do not edit**, regenerated on every `make board-gen`.

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

These `BOARD_*_COUNT` constants drive static array sizes in the driver layer. `include/config/mcu_config.h` re-exports them as `NO_OF_UART`, `NO_OF_IIC`, `NO_OF_SPI`, `NO_OF_GPIO` for backward compatibility with existing driver code.

### board_config.c

`boards/<board_name>/board_config.c` — **do not edit**. Contains:

1. **Peripheral clock-enable wrappers** — one static function per peripheral instance
2. **Peripheral descriptor tables** — `_uart_table[]`, `_iic_table[]`, `_spi_table[]`, `_gpio_table[]`; each entry is a `board_uart_desc_t` / `board_iic_desc_t` / `board_spi_desc_t` / `board_gpio_desc_t` with all HAL constants filled in
3. **HAL module guards** — each table is wrapped in `#ifdef HAL_xxx_MODULE_ENABLED … #else #define _XXX_TABLE NULL #endif` so disabling a HAL module at Kconfig time zeros the corresponding driver array safely
4. **Top-level `g_board_config`** — the one `board_config_t` instance pointed to by `board_get_config()`
5. **`board_get_config()`** — returns `&g_board_config`
6. **`board_find_uart/iic/spi()`** — linear search helpers returning a descriptor by instance pointer
7. **`board_gpio_clk_enable()`** — if/else chain enabling the RCC clock for a given GPIO port
8. **Mutable callback tables** — one `board_uart_cbs_t _uart_cbs[BOARD_UART_COUNT]` (and similar for iic/spi/gpio), zero-initialised
9. **Callback registration and getter functions** — `board_uart_register_rx_cb()`, `board_get_uart_cbs()`, etc.

Example snippet from the UART table:

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

The top-level `Makefile` handles code generation automatically:

```makefile
CONFIG_BOARD ?= stm32f411_devboard
BOARD_XML    := boards/$(CONFIG_BOARD).xml
BOARD_BSP_C  := boards/$(CONFIG_BOARD)/board_config.c
BOARD_BSP_H  := include/board/board_device_ids.h

# Auto-generate BSP from XML before compiling
$(BOARD_BSP_C) $(BOARD_BSP_H): $(BOARD_XML)
    @echo "### Generating BSP from $< ..."
    @python3 scripts/gen_board_config.py $<

all: $(BOARD_BSP_C) $(BOARD_BSP_H) $(BUILD)/kernel.elf
```

Available `make` targets related to board configuration:

| Target | Effect |
|--------|--------|
| `make all` | Generates BSP (if XML is newer), then builds firmware |
| `make board-gen` | Forces BSP regeneration from XML |
| `make CONFIG_BOARD=my_board all` | Select a different board |
| `make clean` | Removes build artifacts and generated BSP files |

The `boards/Makefile` sub-directory Makefile adds the generated `board_config.c` to the link:

```makefile
obj-y += boards/$(CONFIG_BOARD)/board_config.o
```

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

/* Called from ISR when a byte arrives on UART_DEBUG */
static void my_rx_handler(uint8_t dev_id, uint8_t byte)
{
    /* process byte — keep this short, ISR context */
}

/* Called when a transmit completes */
static void my_tx_done(uint8_t dev_id)
{
    /* signal a task if needed */
}

/* Called on UART error */
static void my_error(uint8_t dev_id, uint32_t error_code)
{
    /* log or recover */
}

void app_setup_callbacks(void)
{
    board_uart_register_rx_cb(UART_DEBUG, my_rx_handler);
    board_uart_register_tx_done_cb(UART_DEBUG, my_tx_done);
    board_uart_register_error_cb(UART_DEBUG, my_error);
}
```

### I2C callbacks

```c
board_iic_register_done_cb(I2C_SENSOR_BUS, my_iic_done);
board_iic_register_error_cb(I2C_SENSOR_BUS, my_iic_error);
```

### SPI callbacks

```c
board_spi_register_done_cb(SPI_FLASH_BUS, my_spi_done);
```

### GPIO interrupt callbacks

```c
/* Called from EXTI IRQ handler when BTN_USER fires */
static void btn_irq(uint8_t dev_id)
{
    /* debounce and signal task */
}

board_gpio_register_irq_cb(BTN_USER, btn_irq);
```

### Callback dispatch chain (UART RX example)

```
HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  └─ drv_uart_rx_isr_dispatch(dev_id, rx_byte)
       ├─ pushes byte into ring buffer (IPC queue for os_read())
       └─ board_get_uart_cbs(dev_id)->on_rx_byte(dev_id, byte)
            └─ application my_rx_handler()
```

---

## Adding a New Board

1. **Create the XML file** — copy `boards/stm32f411_devboard.xml` to `boards/<my_board>.xml` and edit to match your hardware.

2. **Generate the BSP:**

   ```bash
   make CONFIG_BOARD=my_board board-gen
   ```

   This creates:
   - `boards/my_board/board_config.c`
   - `include/board/board_device_ids.h`

3. **Build:**

   ```bash
   make CONFIG_BOARD=my_board all
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

1. **Subclass `VendorCodegen`** in `gen_board_config.py`:

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

3. **Add a vendor HAL backend** in `drivers/hal/myvendor/` implementing `drv_uart_hal_ops_t`, `drv_iic_hal_ops_t`, `drv_spi_hal_ops_t`, `drv_gpio_hal_ops_t` — see `DRIVERS.md` for the ops vtable interface.

4. **Guard the HAL includes** in management service source files with `#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_MYVENDOR)` blocks, following the existing pattern in `uart_mgmt.c`, `iic_mgmt.c`, etc.

5. **Set `vendor="MYVENDOR"`** in your board XML and run `make board-gen`.
