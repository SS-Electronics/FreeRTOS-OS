# Driver Architecture — FreeRTOS-OS

This document describes the complete driver subsystem: its three-layer architecture, all public APIs, the vendor HAL abstraction, and the management service threads that run each peripheral class as a FreeRTOS task.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Directory Layout](#directory-layout)
- [Layer 1 — Generic Handle (`drv_handle.h`)](#layer-1--generic-handle-drv_handleh)
  - [Handle Structs](#handle-structs)
  - [HAL Ops Vtables](#hal-ops-vtables)
  - [Vendor Hardware Context](#vendor-hardware-context)
  - [Registration API](#registration-api)
- [Layer 2 — Vendor HAL Backends](#layer-2--vendor-hal-backends)
  - [STM32 HAL Backend](#stm32-hal-backend)
    - [UART — hal_uart_stm32](#uart--hal_uart_stm32)
    - [I2C — hal_iic_stm32](#i2c--hal_iic_stm32)
    - [SPI — hal_spi_stm32](#spi--hal_spi_stm32)
    - [GPIO — hal_gpio_stm32](#gpio--hal_gpio_stm32)
  - [Infineon Baremetal Backend (Stubs)](#infineon-baremetal-backend-stubs)
- [Layer 3 — Generic Driver API](#layer-3--generic-driver-api)
  - [UART Driver](#uart-driver-drv_uartc)
  - [I2C Driver](#i2c-driver-drv_iicc)
  - [SPI Driver](#spi-driver-drv_spic)
  - [GPIO Driver](#gpio-driver-drv_gpioc)
  - [Timer / Time-base Driver](#timer--time-base-driver-drv_timec)
  - [CPU Utility Driver](#cpu-utility-driver-drv_cpuc)
- [Management Service Threads](#management-service-threads)
  - [UART Management — uart_mgmt](#uart-management--uart_mgmt)
  - [I2C Management — iic_mgmt](#i2c-management--iic_mgmt)
  - [SPI Management — spi_mgmt](#spi-management--spi_mgmt)
  - [GPIO Management — gpio_mgmt](#gpio-management--gpio_mgmt)
- [External Chip Drivers](#external-chip-drivers)
- [Adding a New Vendor](#adding-a-new-vendor)
- [Adding a New Peripheral Driver](#adding-a-new-peripheral-driver)
- [Include Path Reference](#include-path-reference)
- [Kconfig Flags That Affect Drivers](#kconfig-flags-that-affect-drivers)

---

## Architecture Overview

The driver subsystem has three strict layers. Higher layers never call lower-layer functions directly — all communication goes through the HAL ops vtable bound at registration time.

```
┌────────────────────────────────────────────────────────────┐
│              Application / Protocol Stack                  │
│          drv_serial_transmit()  drv_iic_transmit()  …      │
└─────────────────────────┬──────────────────────────────────┘
                          │  Generic driver API
┌─────────────────────────▼──────────────────────────────────┐
│           Management Service Threads  (services/)          │
│   uart_mgmt · iic_mgmt · spi_mgmt · gpio_mgmt             │
│   — own the handle arrays, run as FreeRTOS tasks           │
│   — dispatch TX/RX via FreeRTOS queue messages             │
└──────────┬──────────────────────────────┬──────────────────┘
           │  drv_uart_handle_t.ops->…    │
┌──────────▼──────────────────────────────▼──────────────────┐
│              Generic Driver Layer  (drivers/)              │
│   drv_uart.c  drv_iic.c  drv_spi.c  drv_gpio.c            │
│   — vendor-agnostic                                        │
│   — owns static handle arrays                             │
│   — dispatches every operation through ops vtable         │
└──────────┬──────────────────────────────────────────────────┘
           │  const drv_uart_hal_ops_t *ops  (function pointers)
┌──────────▼──────────────────────────────────────────────────┐
│              Vendor HAL Backend  (drivers/hal/)             │
│                                                            │
│  ┌──────────────────────┐   ┌────────────────────────────┐ │
│  │   STM32 HAL          │   │  Infineon XMC (baremetal)  │ │
│  │  hal_uart_stm32.c    │   │  hal_uart_infineon.c       │ │
│  │  hal_iic_stm32.c     │   │  hal_iic_infineon.c        │ │
│  │  hal_spi_stm32.c     │   │  (stubs — TODO registers)  │ │
│  │  hal_gpio_stm32.c    │   └────────────────────────────┘ │
│  └──────────────────────┘                                  │
│   calls STM32 HAL_UART_Transmit() / HAL_I2C_Init() / …    │
└─────────────────────────────────────────────────────────────┘
```

**The vendor is selected at compile time** via `CONFIG_DEVICE_VARIANT` in [`include/config/mcu_config.h`](include/config/mcu_config.h):

```c
#define MCU_VAR_STM         2   /* STM32 — uses STM32 HAL */
#define MCU_VAR_INFINEON    3   /* Infineon XMC — baremetal register access */

#define CONFIG_DEVICE_VARIANT   MCU_VAR_STM
```

Switching vendors requires only changing `CONFIG_DEVICE_VARIANT` — no changes to the generic driver layer or application code.

---

## Directory Layout

```
FreeRTOS-OS/
│
├── include/
│   ├── drivers/
│   │   ├── drv_handle.h                   ← Central: generic handles + HAL ops vtables
│   │   │
│   │   ├── com/                           ← Communication peripherals
│   │   │   ├── drv_uart.h                 ← UART public API
│   │   │   ├── drv_iic.h                  ← I2C public API
│   │   │   ├── drv_spi.h                  ← SPI public API
│   │   │   ├── drv_can.h                  ← CAN public API
│   │   │   └── hal/
│   │   │       ├── stm32/
│   │   │       │   ├── hal_uart_stm32.h   ← STM32 UART HAL ops declaration
│   │   │       │   ├── hal_iic_stm32.h    ← STM32 I2C HAL ops declaration
│   │   │       │   └── hal_spi_stm32.h    ← STM32 SPI HAL ops declaration
│   │   │       └── infineon/
│   │   │           ├── hal_uart_infineon.h
│   │   │           └── hal_iic_infineon.h
│   │   │
│   │   ├── cpu/                           ← CPU / digital I/O
│   │   │   ├── drv_gpio.h                 ← GPIO public API
│   │   │   ├── drv_cpu.h                  ← CPU utility API
│   │   │   └── hal/
│   │   │       └── stm32/
│   │   │           └── hal_gpio_stm32.h   ← STM32 GPIO HAL ops declaration
│   │   │
│   │   ├── timer/
│   │   │   └── drv_time.h                 ← Timer / tick / encoder API
│   │   │
│   │   └── ext_chips/                     ← External device drivers (IC-level)
│   │       ├── PCA9685_IIC/               ← 16-channel PWM controller
│   │       ├── MCP45HVX1/                 ← Digital potentiometer
│   │       ├── M95M02_SPI/                ← SPI EEPROM
│   │       ├── MCP3427_IIC/               ← 16-bit ADC
│   │       ├── MCP23017_IIC/              ← 16-bit GPIO expander
│   │       ├── MCP4441_IIC/               ← Quad digital potentiometer
│   │       ├── INA230_IIC/                ← Power monitor
│   │       └── DS3202_IIC/                ← Temperature / voltage regulator
│   │
│   └── services/                          ← Management thread public headers
│       ├── uart_mgmt.h
│       ├── iic_mgmt.h
│       ├── spi_mgmt.h
│       └── gpio_mgmt.h
│
├── drivers/
│   ├── Makefile
│   ├── drv_uart.c                         ← Generic UART driver
│   ├── drv_iic.c                          ← Generic I2C driver
│   ├── drv_spi.c                          ← Generic SPI driver
│   ├── drv_gpio.c                         ← Generic GPIO driver
│   ├── drv_time.c                         ← Timer / time-base driver
│   ├── drv_cpu.c                          ← CPU utility driver
│   └── hal/
│       ├── stm32/
│       │   ├── hal_uart_stm32.c           ← STM32 UART ops implementation
│       │   ├── hal_iic_stm32.c            ← STM32 I2C ops implementation
│       │   ├── hal_spi_stm32.c            ← STM32 SPI ops implementation
│       │   └── hal_gpio_stm32.c           ← STM32 GPIO ops implementation
│       └── infineon/
│           ├── hal_uart_infineon.c        ← Infineon UART stubs
│           └── hal_iic_infineon.c         ← Infineon I2C stubs
│
└── services/
    ├── Makefile
    ├── uart_mgmt.c                        ← UART FreeRTOS management task
    ├── iic_mgmt.c                         ← I2C FreeRTOS management task
    ├── spi_mgmt.c                         ← SPI FreeRTOS management task
    └── gpio_mgmt.c                        ← GPIO FreeRTOS management task
```

---

## Layer 1 — Generic Handle (`drv_handle.h`)

**File:** [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h)

This is the single most important file in the driver subsystem. Everything else depends on it. It defines:

1. The **forward-declared handle types** (`drv_uart_handle_t`, etc.)
2. The **vendor hardware context** types (selected by `CONFIG_DEVICE_VARIANT`)
3. The **HAL ops vtables** (function pointer structs)
4. The **concrete handle structs** that embed context + vtable pointer
5. The **registration and accessor API** declarations

### Handle Structs

Each peripheral class has one generic handle struct. The generic driver layer owns an array of these; upper layers access them only through the public driver API.

```c
/* UART */
struct drv_uart_handle {
    uint8_t                   dev_id;       // OS-level device index (0-based)
    uint8_t                   initialized;  // 1 after hw_init() succeeds
    drv_hw_uart_ctx_t         hw;           // vendor hardware context (opaque)
    const drv_uart_hal_ops_t *ops;          // bound HAL ops vtable
    uint32_t                  baudrate;     // configured baud rate
    uint32_t                  timeout_ms;   // default blocking timeout
    int32_t                   last_err;     // last error code (OS_ERR_*)
};

/* I2C */
struct drv_iic_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_iic_ctx_t         hw;
    const drv_iic_hal_ops_t *ops;
    uint32_t                 clock_hz;     // I2C bus clock (e.g. 400000)
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/* SPI */
struct drv_spi_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_spi_ctx_t         hw;
    const drv_spi_hal_ops_t *ops;
    uint32_t                 clock_hz;     // SPI clock speed
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/* GPIO */
struct drv_gpio_handle {
    uint8_t                   dev_id;      // logical GPIO line ID
    uint8_t                   initialized;
    drv_hw_gpio_ctx_t         hw;
    const drv_gpio_hal_ops_t *ops;
    int32_t                   last_err;
};
```

### HAL Ops Vtables

Each peripheral class has one vtable (function pointer struct). The generic driver layer only calls through this table — it never calls `HAL_UART_Transmit()` or any vendor function directly.

```c
/* UART vtable */
typedef struct drv_uart_hal_ops {
    int32_t (*hw_init)    (drv_uart_handle_t *h);
    int32_t (*hw_deinit)  (drv_uart_handle_t *h);
    int32_t (*transmit)   (drv_uart_handle_t *h, const uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);
    int32_t (*receive)    (drv_uart_handle_t *h, uint8_t *data,
                           uint16_t len, uint32_t timeout_ms);
    int32_t (*start_rx_it)(drv_uart_handle_t *h, uint8_t *rx_byte);
    void    (*rx_isr_cb)  (drv_uart_handle_t *h, uint8_t rx_byte, void *rb);
} drv_uart_hal_ops_t;

/* I2C vtable */
typedef struct drv_iic_hal_ops {
    int32_t (*hw_init)        (drv_iic_handle_t *h);
    int32_t (*hw_deinit)      (drv_iic_handle_t *h);
    int32_t (*transmit)       (drv_iic_handle_t *h, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               const uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
    int32_t (*receive)        (drv_iic_handle_t *h, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
    int32_t (*is_device_ready)(drv_iic_handle_t *h, uint16_t dev_addr,
                               uint32_t timeout_ms);
} drv_iic_hal_ops_t;

/* SPI vtable */
typedef struct drv_spi_hal_ops {
    int32_t (*hw_init)  (drv_spi_handle_t *h);
    int32_t (*hw_deinit)(drv_spi_handle_t *h);
    int32_t (*transmit) (drv_spi_handle_t *h, const uint8_t *data,
                         uint16_t len, uint32_t timeout_ms);
    int32_t (*receive)  (drv_spi_handle_t *h, uint8_t *data,
                         uint16_t len, uint32_t timeout_ms);
    int32_t (*transfer) (drv_spi_handle_t *h, const uint8_t *tx,
                         uint8_t *rx, uint16_t len, uint32_t timeout_ms);
} drv_spi_hal_ops_t;

/* GPIO vtable */
typedef struct drv_gpio_hal_ops {
    int32_t (*hw_init)  (drv_gpio_handle_t *h);
    int32_t (*hw_deinit)(drv_gpio_handle_t *h);
    void    (*set)      (drv_gpio_handle_t *h);
    void    (*clear)    (drv_gpio_handle_t *h);
    void    (*toggle)   (drv_gpio_handle_t *h);
    uint8_t (*read)     (drv_gpio_handle_t *h);
    void    (*write)    (drv_gpio_handle_t *h, uint8_t state);
} drv_gpio_hal_ops_t;
```

### Vendor Hardware Context

The `hw` field inside each handle embeds the vendor-specific hardware state. `CONFIG_DEVICE_VARIANT` selects which concrete type is used — the generic layer treats it as opaque.

| `CONFIG_DEVICE_VARIANT` | `drv_hw_uart_ctx_t` | `drv_hw_iic_ctx_t` | `drv_hw_spi_ctx_t` | `drv_hw_gpio_ctx_t` |
|---|---|---|---|---|
| `MCU_VAR_STM` | `{ UART_HandleTypeDef huart; }` | `{ I2C_HandleTypeDef hi2c; }` | `{ SPI_HandleTypeDef hspi; }` | `{ GPIO_TypeDef *port; uint16_t pin; uint8_t active_state; }` |
| `MCU_VAR_INFINEON` | `{ uint32_t channel_base; uint32_t baudrate; }` | `{ uint32_t channel_base; uint32_t clock_hz; }` | `{ uint32_t channel_base; uint32_t clock_hz; }` | `{ uint32_t port_base; uint32_t pin; uint8_t active_state; }` |

### Registration API

```c
/* Bind an ops vtable and call hw_init() — called by the mgmt thread at startup */
int32_t drv_uart_register(uint8_t dev_id, const drv_uart_hal_ops_t *ops,
                           uint32_t baudrate, uint32_t timeout_ms);
int32_t drv_iic_register (uint8_t dev_id, const drv_iic_hal_ops_t  *ops,
                           uint32_t clock_hz, uint32_t timeout_ms);
int32_t drv_spi_register (uint8_t dev_id, const drv_spi_hal_ops_t  *ops,
                           uint32_t clock_hz, uint32_t timeout_ms);
int32_t drv_gpio_register(uint8_t dev_id, const drv_gpio_hal_ops_t *ops);

/* Retrieve a handle by device index — returns NULL if out of range */
drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id);
drv_iic_handle_t  *drv_iic_get_handle (uint8_t dev_id);
drv_spi_handle_t  *drv_spi_get_handle (uint8_t dev_id);
drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);
```

All return `OS_ERR_NONE` (0) on success or a negative `OS_ERR_*` code on failure.

---

## Layer 2 — Vendor HAL Backends

The HAL backend files contain all vendor-specific code. The `.h` files live in `include/drivers/` under the appropriate category subdirectory. The `.c` files live in `drivers/hal/<vendor>/`.

### STM32 HAL Backend

Compiled only when `CONFIG_DEVICE_VARIANT == MCU_VAR_STM`. Each file wraps the corresponding STM32 HAL function and converts `HAL_StatusTypeDef` to `OS_ERR_*`.

---

#### UART — hal_uart_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_uart_stm32.h`](include/drivers/com/hal/stm32/hal_uart_stm32.h) |
| Source | [`drivers/hal/stm32/hal_uart_stm32.c`](drivers/hal/stm32/hal_uart_stm32.c) |

**API:**

```c
/* Returns pointer to the static STM32 UART ops table.
   Pass to drv_uart_register() to bind STM32 HAL. */
const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void);

/* Populate the hardware context before registering.
   instance: USART1, USART2, USART6, …
   word_len: UART_WORDLENGTH_8B / UART_WORDLENGTH_9B
   stop_bits: UART_STOPBITS_1 / UART_STOPBITS_2
   parity:   UART_PARITY_NONE / _EVEN / _ODD
   mode:     UART_MODE_TX_RX / _TX / _RX               */
void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef *instance,
                               uint32_t word_len, uint32_t stop_bits,
                               uint32_t parity,   uint32_t mode);
```

**Underlying STM32 HAL calls made by the ops table:**

| Op | STM32 HAL call |
|---|---|
| `hw_init` | `HAL_UART_Init()` |
| `hw_deinit` | `HAL_UART_DeInit()` |
| `transmit` | `HAL_UART_Transmit()` |
| `receive` | `HAL_UART_Receive()` |
| `start_rx_it` | `UART_Start_Receive_IT()` |
| `rx_isr_cb` | `HAL_UART_Receive_IT()` + `ringbuffer_putchar()` |

**Interrupt integration:** `HAL_UART_RxCpltCallback()` is implemented in [`drivers/drv_uart.c`](drivers/drv_uart.c). It walks the handle array to identify the source device and calls `drv_uart_rx_isr_dispatch()` which routes the byte into the IPC ring buffer.

**Usage example:**

```c
#include <drivers/com/hal/stm32/hal_uart_stm32.h>

/* 1. Configure hardware context (called before registration) */
drv_uart_handle_t *h = drv_uart_get_handle(UART_1);
hal_uart_stm32_set_config(h, USART1,
                          UART_WORDLENGTH_8B, UART_STOPBITS_1,
                          UART_PARITY_NONE,   UART_MODE_TX_RX);

/* 2. Register — binds ops table and calls hw_init + start_rx_it */
drv_uart_register(UART_1, hal_uart_stm32_get_ops(), 115200, 10);
```

---

#### I2C — hal_iic_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_iic_stm32.h`](include/drivers/com/hal/stm32/hal_iic_stm32.h) |
| Source | [`drivers/hal/stm32/hal_iic_stm32.c`](drivers/hal/stm32/hal_iic_stm32.c) |

**API:**

```c
const drv_iic_hal_ops_t *hal_iic_stm32_get_ops(void);

/* instance: I2C1, I2C2, I2C3
   addr_mode: I2C_ADDRESSINGMODE_7BIT / _10BIT
   dual_addr: I2C_DUALADDRESS_DISABLE / _ENABLE   */
void hal_iic_stm32_set_config(drv_iic_handle_t *h,
                              I2C_TypeDef *instance,
                              uint32_t addr_mode, uint32_t dual_addr);
```

**Underlying STM32 HAL calls:**

| Op | `use_reg == 0` | `use_reg == 1` |
|---|---|---|
| `transmit` | `HAL_I2C_Master_Transmit()` | `HAL_I2C_Mem_Write()` |
| `receive` | `HAL_I2C_Master_Receive()` | `HAL_I2C_Mem_Read()` |
| `is_device_ready` | `HAL_I2C_IsDeviceReady()` (3 retries) | — |

Device addresses are **7-bit** (not shifted); the HAL backend applies the `<< 1` shift internally.

---

#### SPI — hal_spi_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/com/hal/stm32/hal_spi_stm32.h`](include/drivers/com/hal/stm32/hal_spi_stm32.h) |
| Source | [`drivers/hal/stm32/hal_spi_stm32.c`](drivers/hal/stm32/hal_spi_stm32.c) |

**API:**

```c
const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void);

/* Configure all SPI hardware parameters before registering.
   instance:        SPI1 … SPI5
   mode:            SPI_MODE_MASTER / _SLAVE
   direction:       SPI_DIRECTION_2LINES / _1LINE
   data_size:       SPI_DATASIZE_8BIT / _16BIT
   clk_polarity:    SPI_POLARITY_LOW / _HIGH
   clk_phase:       SPI_PHASE_1EDGE / _2EDGE
   nss:             SPI_NSS_SOFT / _HARD_INPUT / _HARD_OUTPUT
   baud_prescaler:  SPI_BAUDRATEPRESCALER_2 … _256
   bit_order:       SPI_FIRSTBIT_MSB / _LSB              */
void hal_spi_stm32_set_config(drv_spi_handle_t *h,
                               SPI_TypeDef *instance,
                               uint32_t mode, uint32_t direction,
                               uint32_t data_size, uint32_t clk_polarity,
                               uint32_t clk_phase, uint32_t nss,
                               uint32_t baud_prescaler, uint32_t bit_order);
```

**Underlying STM32 HAL calls:**

| Op | STM32 HAL call |
|---|---|
| `hw_init` | `HAL_SPI_Init()` |
| `transmit` | `HAL_SPI_Transmit()` |
| `receive` | `HAL_SPI_Receive()` |
| `transfer` | `HAL_SPI_TransmitReceive()` (full-duplex) |

> **Note:** Chip-select (CS/NSS) toggling is the caller's responsibility. The SPI driver does not assert or deassert any CS GPIO pin.

---

#### GPIO — hal_gpio_stm32

| File | Path |
|---|---|
| Header | [`include/drivers/cpu/hal/stm32/hal_gpio_stm32.h`](include/drivers/cpu/hal/stm32/hal_gpio_stm32.h) |
| Source | [`drivers/hal/stm32/hal_gpio_stm32.c`](drivers/hal/stm32/hal_gpio_stm32.c) |

**API:**

```c
const drv_gpio_hal_ops_t *hal_gpio_stm32_get_ops(void);

/* port:         GPIOA … GPIOE
   pin:          GPIO_PIN_0 … GPIO_PIN_15
   mode:         GPIO_MODE_OUTPUT_PP / _INPUT / _AF_PP / …
   pull:         GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN
   speed:        GPIO_SPEED_FREQ_LOW … _VERY_HIGH
   active_state: GPIO_PIN_SET (active-high) or GPIO_PIN_RESET (active-low) */
void hal_gpio_stm32_set_config(drv_gpio_handle_t *h,
                                GPIO_TypeDef *port, uint16_t pin,
                                uint32_t mode, uint32_t pull,
                                uint32_t speed, uint8_t active_state);
```

> **Clock enable:** The GPIO peripheral clock must be enabled before `hw_init()` is called (e.g. `__HAL_RCC_GPIOA_CLK_ENABLE()`). This is done in `stm32f4xx_hal_msp.c` or a board-specific `board_init()` function — not inside the generic driver.

---

### Infineon Baremetal Backend (Stubs)

Compiled only when `CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON`. All function bodies contain `TODO` comments with the register sequences to implement for Infineon XMC4xxx / XMC1xxx targets.

| Header | Source | Peripheral |
|---|---|---|
| [`include/drivers/com/hal/infineon/hal_uart_infineon.h`](include/drivers/com/hal/infineon/hal_uart_infineon.h) | [`drivers/hal/infineon/hal_uart_infineon.c`](drivers/hal/infineon/hal_uart_infineon.c) | USIC-UART |
| [`include/drivers/com/hal/infineon/hal_iic_infineon.h`](include/drivers/com/hal/infineon/hal_iic_infineon.h) | [`drivers/hal/infineon/hal_iic_infineon.c`](drivers/hal/infineon/hal_iic_infineon.c) | USIC-I2C |

The Infineon hardware context carries only the USIC channel base address and clock/baud settings. The actual register offsets and bit-field positions must be filled in from the XMC Reference Manual (Chapter 16 — Universal Serial Interface Channel).

---

## Layer 3 — Generic Driver API

These are the functions called by application code and by the management service threads. None of them reference any vendor type.

### UART Driver (`drv_uart.c`)

**Header:** [`include/drivers/com/drv_uart.h`](include/drivers/com/drv_uart.h)
**Source:** [`drivers/drv_uart.c`](drivers/drv_uart.c)

```c
/* Register a UART device (called by uart_mgmt at startup) */
int32_t drv_uart_register(uint8_t dev_id,
                           const drv_uart_hal_ops_t *ops,
                           uint32_t baudrate, uint32_t timeout_ms);

/* Blocking transmit — returns OS_ERR_NONE or OS_ERR_OP */
int32_t drv_serial_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);

/* Blocking receive */
int32_t drv_serial_receive(uint8_t dev_id, uint8_t *data, uint16_t len);

/* ISR dispatcher — call from HAL_UART_RxCpltCallback for the matching dev_id */
void drv_uart_rx_isr_dispatch(uint8_t dev_id, uint8_t rx_byte);

/* Handle accessor — returns NULL if dev_id is out of range */
drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id);
```

**RX data flow:**

```
UART peripheral → HAL_UART_RxCpltCallback()
                → drv_uart_rx_isr_dispatch(dev_id, byte)
                → ops->rx_isr_cb(h, byte, rb)
                → ringbuffer_putchar(rb, byte)    [IPC ring buffer]
                → uart_mgmt thread reads from ring buffer and delivers to app
```

**Peripheral count:** Controlled by `NO_OF_UART` in `mcu_config.h` (default: 1). Handle array is statically sized.

---

### I2C Driver (`drv_iic.c`)

**Header:** [`include/drivers/com/drv_iic.h`](include/drivers/com/drv_iic.h)
**Source:** [`drivers/drv_iic.c`](drivers/drv_iic.c)

```c
int32_t drv_iic_register(uint8_t dev_id, const drv_iic_hal_ops_t *ops,
                          uint32_t clock_hz, uint32_t timeout_ms);

/* Write len bytes to register reg_addr of device at dev_addr (7-bit) */
int32_t drv_iic_transmit(uint8_t dev_id, uint16_t dev_addr,
                          uint8_t reg_addr, const uint8_t *data, uint16_t len);

/* Read len bytes from register reg_addr */
int32_t drv_iic_receive(uint8_t dev_id, uint16_t dev_addr,
                         uint8_t reg_addr, uint8_t *data, uint16_t len);

/* Ping device — returns OS_ERR_NONE if it ACKs within timeout */
int32_t drv_iic_device_ready(uint8_t dev_id, uint16_t dev_addr);

drv_iic_handle_t *drv_iic_get_handle(uint8_t dev_id);
```

**Peripheral count:** `NO_OF_IIC` (default: 0, enable via `make menuconfig`).

---

### SPI Driver (`drv_spi.c`)

**Header:** [`include/drivers/com/drv_spi.h`](include/drivers/com/drv_spi.h)
**Source:** [`drivers/drv_spi.c`](drivers/drv_spi.c)

```c
int32_t drv_spi_register(uint8_t dev_id, const drv_spi_hal_ops_t *ops,
                          uint32_t clock_hz, uint32_t timeout_ms);

int32_t drv_spi_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len);
int32_t drv_spi_receive (uint8_t dev_id, uint8_t *data, uint16_t len);

/* Full-duplex: tx and rx buffers must both be non-NULL and len bytes long */
int32_t drv_spi_transfer(uint8_t dev_id, const uint8_t *tx,
                          uint8_t *rx, uint16_t len);

drv_spi_handle_t *drv_spi_get_handle(uint8_t dev_id);
```

**Peripheral count:** `CONFIG_MCU_NO_OF_SPI_PERIPHERAL` (default: 0 when `HAL_SPI_MODULE_ENABLED` is not set in Kconfig).

---

### GPIO Driver (`drv_gpio.c`)

**Header:** [`include/drivers/cpu/drv_gpio.h`](include/drivers/cpu/drv_gpio.h)
**Source:** [`drivers/drv_gpio.c`](drivers/drv_gpio.c)

GPIO lines are registered individually by board-specific code before use. Up to `DRV_GPIO_MAX_LINES` (default 32) logical lines are supported.

```c
/* Register a logical GPIO line — calls hw_init() immediately */
int32_t drv_gpio_register(uint8_t dev_id, const drv_gpio_hal_ops_t *ops);

/* Pin operations */
int32_t drv_gpio_set_pin   (uint8_t dev_id);              // drive high
int32_t drv_gpio_clear_pin (uint8_t dev_id);              // drive low
int32_t drv_gpio_toggle_pin(uint8_t dev_id);
int32_t drv_gpio_write_pin (uint8_t dev_id, uint8_t state);
uint8_t drv_gpio_read_pin  (uint8_t dev_id);              // returns 0 or 1

drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);
```

---

### Timer / Time-base Driver (`drv_time.c`)

**Header:** [`include/drivers/timer/drv_time.h`](include/drivers/timer/drv_time.h)
**Source:** [`drivers/drv_time.c`](drivers/drv_time.c)

Provides OS-agnostic tick access and quadrature encoder tick counters.

```c
/* Returns HAL_GetTick() — milliseconds since reset */
uint32_t drv_time_get_ticks(void);

/* Blocking millisecond delay (wraps HAL_Delay) */
void drv_time_delay_ms(uint32_t ms);

/* Quadrature encoder tick counters (when NO_OF_TIMER > 0) */
uint32_t drv_get_tim_2_encoder_ticks(void);
uint32_t drv_get_tim_3_encoder_ticks(void);
void     drv_set_tim_3_encoder_ticks(uint32_t tim_val);
```

---

### CPU Utility Driver (`drv_cpu.c`)

**Header:** [`include/drivers/cpu/drv_cpu.h`](include/drivers/cpu/drv_cpu.h)
**Source:** [`drivers/drv_cpu.c`](drivers/drv_cpu.c)

```c
/* Configure NVIC interrupt priorities for all used peripherals */
void drv_cpu_interrupt_prio_set(void);

/* Software reset via NVIC_SystemReset() */
void reset_mcu(void);
```

---

## Management Service Threads

Each peripheral class has a dedicated FreeRTOS service thread in `services/`. These threads:

1. **Own** the driver registration — they call `drv_xxx_register()` after a startup delay so that all OS subsystems are ready.
2. **Serialise** hardware access — application tasks post messages to the management queue instead of calling the driver API directly, preventing concurrent bus access.
3. **Recover** from errors — on `last_err != OS_ERR_NONE` the thread attempts a `hw_deinit` + `hw_init` reinit cycle before returning an error.

All management threads are activated via `INC_SERVICE_*` flags in [`include/config/os_config.h`](include/config/os_config.h).

---

### UART Management — `uart_mgmt`

**Header:** [`include/services/uart_mgmt.h`](include/services/uart_mgmt.h)
**Source:** [`services/uart_mgmt.c`](services/uart_mgmt.c)
**Config flag:** `INC_SERVICE_UART_MGMT` in `os_config.h`

#### Thread lifecycle

```
os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT)  ← wait for OS ready
    │
    ├─ drv_uart_register(id, hal_uart_stm32_get_ops(), UART_1_BAUD, 10)
    │   for each id in 0..NO_OF_UART-1
    │
    └─ loop: xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY)
                │
                ├─ UART_MGMT_CMD_TRANSMIT → ops->transmit()
                │   on error: hw_deinit() + hw_init()
                ├─ UART_MGMT_CMD_REINIT  → hw_deinit() + hw_init()
                └─ UART_MGMT_CMD_DEINIT  → hw_deinit()
```

#### Message structure

```c
typedef enum {
    UART_MGMT_CMD_TRANSMIT,   // async TX request
    UART_MGMT_CMD_REINIT,     // force peripheral reinit
    UART_MGMT_CMD_DEINIT,     // graceful shutdown
} uart_mgmt_cmd_t;

typedef struct {
    uart_mgmt_cmd_t  cmd;
    uint8_t          dev_id;
    const uint8_t   *data;    // buffer must remain valid until TX completes
    uint16_t         len;
} uart_mgmt_msg_t;
```

#### Public API

```c
/* Start the management thread — call before vTaskStartScheduler() */
int32_t uart_mgmt_start(void);

/* Post an async TX (non-blocking, returns OS_ERR_OP if queue full) */
int32_t uart_mgmt_async_transmit(uint8_t dev_id,
                                  const uint8_t *data, uint16_t len);

/* Get the raw queue handle for direct posting */
QueueHandle_t uart_mgmt_get_queue(void);
```

**Stack / priority** (configurable in `os_config.h`):

| Parameter | Macro | Default |
|---|---|---|
| Stack depth (words) | `PROC_SERVICE_SERIAL_MGMT_STACK_SIZE` | 512 |
| Priority | `PROC_SERVICE_SERIAL_MGMT_PRIORITY` | 1 |
| Startup delay | `TIME_OFFSET_SERIAL_MANAGEMENT` | 4000 ms |

---

### I2C Management — `iic_mgmt`

**Header:** [`include/services/iic_mgmt.h`](include/services/iic_mgmt.h)
**Source:** [`services/iic_mgmt.c`](services/iic_mgmt.c)
**Config flag:** `INC_SERVICE_IIC_MGMT` in `os_config.h`

Adds a **synchronous receive** path via FreeRTOS task notification — the caller blocks until the management thread completes the I2C read, then the result is written back to `*result_code`.

#### Message structure

```c
typedef enum {
    IIC_MGMT_CMD_TRANSMIT,
    IIC_MGMT_CMD_RECEIVE,
    IIC_MGMT_CMD_PROBE,        // send address, check ACK
    IIC_MGMT_CMD_REINIT,
} iic_mgmt_cmd_t;

typedef struct {
    iic_mgmt_cmd_t   cmd;
    uint8_t          bus_id;
    uint16_t         dev_addr;        // 7-bit address
    uint8_t          reg_addr;
    uint8_t          use_reg;         // 1 → use reg_addr prefix
    uint8_t         *data;
    uint16_t         len;
    TaskHandle_t     result_notify;   // non-NULL for sync operations
    int32_t         *result_code;     // written by management thread
} iic_mgmt_msg_t;
```

#### Public API

```c
int32_t iic_mgmt_start(void);
QueueHandle_t iic_mgmt_get_queue(void);

/* Non-blocking write (fire and forget) */
int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len);

/* Blocking read — caller suspends until management thread completes */
int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms);
```

**Stack / priority:**

| Parameter | Macro | Default |
|---|---|---|
| Stack depth | `PROC_SERVICE_IIC_MGMT_STACK_SIZE` | 1024 |
| Priority | `PROC_SERVICE_IIC_MGMT_PRIORITY` | 1 |
| Startup delay | `TIME_OFFSET_IIC_MANAGEMENT` | 6500 ms |

---

### SPI Management — `spi_mgmt`

**Header:** [`include/services/spi_mgmt.h`](include/services/spi_mgmt.h)
**Source:** [`services/spi_mgmt.c`](services/spi_mgmt.c)

Like the I2C management thread but for SPI. Supports both a fire-and-forget async transmit and a blocking full-duplex transfer with task-notification synchronisation.

```c
int32_t spi_mgmt_start(void);
QueueHandle_t spi_mgmt_get_queue(void);

int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len);

/* Blocking full-duplex transfer */
int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms);
```

> CS pin must be asserted by the caller before posting the message and deasserted after `spi_mgmt_sync_transfer()` returns.

---

### GPIO Management — `gpio_mgmt`

**Header:** [`include/services/gpio_mgmt.h`](include/services/gpio_mgmt.h)
**Source:** [`services/gpio_mgmt.c`](services/gpio_mgmt.c)

Handles asynchronous and **delayed** GPIO commands (e.g. "assert reset for 10 ms then release").

```c
int32_t gpio_mgmt_start(void);
QueueHandle_t gpio_mgmt_get_queue(void);

/* Post a GPIO command.
   delay_ms = 0 → execute immediately.
   delay_ms > 0 → management thread sleeps delay_ms then executes. */
int32_t gpio_mgmt_post(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                        uint8_t state, uint32_t delay_ms);
```

```c
typedef enum {
    GPIO_MGMT_CMD_SET,
    GPIO_MGMT_CMD_CLEAR,
    GPIO_MGMT_CMD_TOGGLE,
    GPIO_MGMT_CMD_WRITE,    // uses msg.state field
    GPIO_MGMT_CMD_REINIT,
} gpio_mgmt_cmd_t;
```

**Example — timed reset pulse:**

```c
/* Assert active-low reset, hold 10 ms, then release */
gpio_mgmt_post(GPIO_ID_RESET, GPIO_MGMT_CMD_CLEAR, 0, 0);    // assert immediately
gpio_mgmt_post(GPIO_ID_RESET, GPIO_MGMT_CMD_SET,   0, 10);   // release after 10 ms
```

---

## External Chip Drivers

Device-specific drivers sit above the generic driver layer in `include/drivers/ext_chips/`. They use `drv_iic_transmit()` / `drv_iic_receive()` or `drv_spi_transfer()` and expose a chip-specific API.

| Driver | Protocol | IC | Purpose |
|---|---|---|---|
| `PCA9685_IIC` | I2C | NXP PCA9685 | 16-channel PWM controller |
| `MCP45HVX1` | I2C | Microchip MCP45HVX1 | High-voltage digital potentiometer |
| `M95M02_SPI` | SPI | ST M95M02 | 2Mbit SPI EEPROM |
| `MCP3427_IIC` | I2C | Microchip MCP3427 | 16-bit delta-sigma ADC |
| `MCP23017_IIC` | I2C | Microchip MCP23017 | 16-bit GPIO expander |
| `MCP4441_IIC` | I2C | Microchip MCP4441 | Quad 8-bit digital potentiometer |
| `INA230_IIC` | I2C | TI INA230 | Power monitor (voltage + current) |
| `DS3202_IIC` | I2C | Maxim DS3502 | 7-bit digital potentiometer |

All ext_chip drivers return `kernel_status_type` and take a device I2C address as first argument, so multiple instances of the same IC can coexist on the same bus.

---

## Adding a New Vendor

1. **Add a new `MCU_VAR_*` constant** in [`include/config/mcu_config.h`](include/config/mcu_config.h).

2. **Extend the `drv_hw_xxx_ctx_t` unions** in [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h) with a new `#elif` branch for the new vendor's hardware register types.

3. **Create HAL backend files** under `drivers/hal/<vendor>/`:
   - Implement all vtable function pointers.
   - Return `OS_ERR_NONE` / `OS_ERR_OP` from every function.

4. **Place headers** in the correct `include/drivers/` subdirectory:
   - Communication peripherals → `include/drivers/com/hal/<vendor>/`
   - CPU / GPIO peripherals → `include/drivers/cpu/hal/<vendor>/`

5. **Update `drivers/Makefile`** with a new `ifeq` block for the new MCU config.

6. **Update the management threads** in `services/*.c` to `#include` and `hal_xxx_<vendor>_get_ops()` under a new `#elif` branch.

---

## Adding a New Peripheral Driver

1. **Create a vtable struct** (`drv_xxx_hal_ops_t`) in [`include/drivers/drv_handle.h`](include/drivers/drv_handle.h).

2. **Add a vendor hardware context** type and forward-declare the handle.

3. **Implement the generic driver** in `drivers/drv_xxx.c` — static handle array, `drv_xxx_register()`, `drv_xxx_get_handle()`, and the public API functions.

4. **Add a public API header** in the correct `include/drivers/<category>/` subfolder.

5. **Implement vendor HAL backends** in `drivers/hal/<vendor>/hal_xxx_<vendor>.{h,c}`.

6. **Create a management thread** in `services/xxx_mgmt.{h,c}` — add the header to `include/services/`.

7. **Update Makefiles:**
   - `drivers/Makefile` — add `drivers/drv_xxx.o` and the HAL `.o` entries.
   - `services/Makefile` — add `services/xxx_mgmt.o` under the appropriate Kconfig guard.
   - `Makefile` — `SUBDIRS` already includes both `drivers` and `services`.

---

## Include Path Reference

The top-level Makefile sets these `-I` flags (via sub-directory Makefiles):

| Path | Covers |
|---|---|
| `-I$(CURDIR)/include` | All project headers (`<drivers/drv_handle.h>`, `<services/uart_mgmt.h>`, …) |
| `-I$(CURDIR)/include/config` | `<autoconf.h>`, `<mcu_config.h>`, `<os_config.h>`, `<board.h>` |
| `-I$(CURDIR)` | Cross-directory driver-layer includes (`<drivers/com/hal/stm32/…>`) |
| `-I$(CURDIR)/arch/devices/STM/stm32f4xx-hal-driver/Inc` | STM32 HAL headers |
| `-I$(CURDIR)/arch/devices/STM/STM32F4xx` | `<device.h>`, `<stm32f4xx.h>` |
| `-I$(CURDIR)/arch/arm/CMSIS_6` | ARM CMSIS-Core |

The VS Code IntelliSense configuration mirrors these paths in [`.vscode/c_cpp_properties.json`](.vscode/c_cpp_properties.json).

---

## Kconfig Flags That Affect Drivers

Run `make menuconfig` to configure these interactively.

| Kconfig symbol | C macro | Effect |
|---|---|---|
| `CONFIG_NO_OF_UART` | `NO_OF_UART` | UART handle array size and IPC queue count |
| `CONFIG_NO_OF_IIC` | `NO_OF_IIC` | I2C handle array size |
| `CONFIG_HAL_SPI_MODULE_ENABLED` | `CONFIG_MCU_NO_OF_SPI_PERIPHERAL` | Enables SPI HAL module and SPI mgmt thread |
| `CONFIG_HAL_TIM_MODULE_ENABLED` | — | Enables timer driver and encoder APIs |
| `CONFIG_INC_SERVICE_UART_MGMT` | `INC_SERVICE_UART_MGMT` | Compiles `uart_mgmt.c` and starts the thread |
| `CONFIG_INC_SERVICE_IIC_MGMT` | `INC_SERVICE_IIC_MGMT` | Compiles `iic_mgmt.c` and starts the thread |
| `CONFIG_HAL_IWDG_MODULE_ENABLED` | `CONFIG_MCU_WDG_EN` | Enables watchdog driver support |

---

*Part of the FreeRTOS-OS project — Author: Subhajit Roy <subhajitroy005@gmail.com>*
