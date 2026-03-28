/*
 * drv_handle.h — Generic driver handle definitions
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Design
 * ──────
 *  Each peripheral class (UART, I2C, SPI, GPIO) has one generic handle struct:
 *
 *    drv_uart_handle_t
 *    drv_iic_handle_t
 *    drv_spi_handle_t
 *    drv_gpio_handle_t
 *
 *  Each handle embeds:
 *    • A vendor-specific hardware context (drv_hw_uart_ctx_t etc.) that is
 *      selected at compile time via CONFIG_DEVICE_VARIANT.  For STM32 this
 *      wraps the HAL handle struct directly; for Infineon it holds register
 *      base addresses.  The generic driver layer never touches this field
 *      directly — only the HAL implementation files do.
 *
 *    • A const pointer to a HAL-ops table (drv_uart_hal_ops_t etc.) that is
 *      populated at registration time.  All driver operations go through this
 *      table so the generic layer is vendor-agnostic.
 *
 *  Management threads (drivers/mgmt/xxx_mgmt.c) own the handle arrays and
 *  expose typed accessor macros so upper layers never manipulate handles
 *  directly.
 */

#ifndef INCLUDE_DRIVERS_DRV_HANDLE_H_
#define INCLUDE_DRIVERS_DRV_HANDLE_H_

#include <def_std.h>
#include <def_err.h>
#include <config/mcu_config.h>  /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */

/* ═══════════════════════════════════════════════════════════════════════════
 * 1.  Forward declarations (break circular dependency with hal_ops structs)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct drv_uart_handle  drv_uart_handle_t;
typedef struct drv_iic_handle   drv_iic_handle_t;
typedef struct drv_spi_handle   drv_spi_handle_t;
typedef struct drv_gpio_handle  drv_gpio_handle_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 2.  Vendor-specific hardware context types
 *     Conditional on CONFIG_DEVICE_VARIANT set in mcu_config.h.
 *     Only the matching HAL implementation file should access the fields
 *     inside these structs.
 * ═══════════════════════════════════════════════════════════════════════════ */

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
/*
 * STM32 — wrap the official STM32 HAL handle structs directly.
 * stm32f4xx_hal.h is pulled in transitively via device.h on STM32 targets.
 */
#include <device.h>  /* pulls stm32f4xx_hal.h for the current STM32 family */

typedef struct {
    UART_HandleTypeDef  huart;
} drv_hw_uart_ctx_t;

#ifdef HAL_I2C_MODULE_ENABLED
typedef struct {
    I2C_HandleTypeDef   hi2c;
} drv_hw_iic_ctx_t;
#else
typedef struct {
    uint32_t            _reserved;
} drv_hw_iic_ctx_t;
#endif

#ifdef HAL_SPI_MODULE_ENABLED
typedef struct {
    SPI_HandleTypeDef   hspi;
} drv_hw_spi_ctx_t;
#else
typedef struct {
    uint32_t            _reserved;
} drv_hw_spi_ctx_t;
#endif

typedef struct {
    GPIO_TypeDef       *port;
    uint16_t            pin;
    uint32_t            mode;           /* GPIO_MODE_OUTPUT_PP, GPIO_MODE_AF_PP, … */
    uint32_t            pull;           /* GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN */
    uint32_t            speed;          /* GPIO_SPEED_FREQ_LOW … _VERY_HIGH */
    uint8_t             active_state;   /* GPIO_PIN_SET or GPIO_PIN_RESET */
} drv_hw_gpio_ctx_t;

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
/*
 * Infineon XMC series — baremetal register-mapped peripheral access.
 * The Infineon SDK is not a dependency of this file; the context struct
 * carries only the minimal information the Infineon HAL layer needs.
 * Vendor HAL files (hal_xxx_infineon.c) cast these fields appropriately.
 */

typedef struct {
    uint32_t    channel_base;   /* USIC channel base address  */
    uint32_t    baudrate;
} drv_hw_uart_ctx_t;

typedef struct {
    uint32_t    channel_base;   /* USIC I2C channel base address */
    uint32_t    clock_hz;
} drv_hw_iic_ctx_t;

typedef struct {
    uint32_t    channel_base;   /* USIC SPI channel base address */
    uint32_t    clock_hz;
} drv_hw_spi_ctx_t;

typedef struct {
    uint32_t    port_base;      /* GPIO port base address */
    uint32_t    pin;
    uint32_t    mode;
    uint32_t    pull;
    uint32_t    speed;
    uint8_t     active_state;
} drv_hw_gpio_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif /* CONFIG_DEVICE_VARIANT */


/* ═══════════════════════════════════════════════════════════════════════════
 * 3.  HAL operations tables  (function-pointer vtables)
 *     Each vendor HAL implementation file defines one const instance of the
 *     matching table and exposes a getter function (e.g. hal_uart_stm32_get_ops).
 *     The management layer binds the ops pointer at registration time.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── UART ops ─────────────────────────────────────────────────────────── */
typedef struct drv_uart_hal_ops {
    /**
     * @brief  Configure the hardware peripheral for the settings stored in
     *         the handle's @c hw context.  Called once before first use.
     * @return OS_ERR_NONE on success, negative OS_ERR_* on failure.
     */
    int32_t (*hw_init)(drv_uart_handle_t *h);

    /** @brief Undo hw_init — disable clock, release GPIO, etc. */
    int32_t (*hw_deinit)(drv_uart_handle_t *h);

    /**
     * @brief  Blocking transmit.
     * @param  data        Pointer to TX buffer.
     * @param  len         Number of bytes to send.
     * @param  timeout_ms  Timeout in milliseconds.
     */
    int32_t (*transmit)(drv_uart_handle_t *h,
                        const uint8_t *data,
                        uint16_t       len,
                        uint32_t       timeout_ms);

    /**
     * @brief  Blocking receive.
     * @param  data        Pointer to RX buffer.
     * @param  len         Number of bytes to receive.
     * @param  timeout_ms  Timeout in milliseconds.
     */
    int32_t (*receive)(drv_uart_handle_t *h,
                       uint8_t  *data,
                       uint16_t  len,
                       uint32_t  timeout_ms);

    /**
     * @brief  Arm the interrupt-driven single-byte receive.
     *         The ISR callback feeds bytes into the RX ring buffer.
     * @param  rx_byte  Pointer to the staging byte used by the ISR.
     */
    int32_t (*start_rx_it)(drv_uart_handle_t *h, uint8_t *rx_byte);

    /**
     * @brief  ISR-context receive-complete callback.
     *         Called from HAL_UART_RxCpltCallback; must be ISR-safe.
     * @param  rx_byte  The byte that was just received.
     * @param  rb       Ring-buffer to push the byte into (may be NULL).
     */
    void    (*rx_isr_cb)(drv_uart_handle_t *h,
                         uint8_t            rx_byte,
                         void              *rb);
} drv_uart_hal_ops_t;


/* ── I2C / IIC ops ────────────────────────────────────────────────────── */
typedef struct drv_iic_hal_ops {
    int32_t (*hw_init)(drv_iic_handle_t *h);
    int32_t (*hw_deinit)(drv_iic_handle_t *h);

    /**
     * @brief  Write @p len bytes to @p dev_addr.
     * @param  dev_addr    7-bit I2C device address (not shifted).
     * @param  reg_addr    Register address (0 if device doesn't use registers).
     * @param  use_reg     Non-zero to send reg_addr before data.
     */
    int32_t (*transmit)(drv_iic_handle_t *h,
                        uint16_t  dev_addr,
                        uint8_t   reg_addr,
                        uint8_t   use_reg,
                        const uint8_t *data,
                        uint16_t  len,
                        uint32_t  timeout_ms);

    /** @brief Read @p len bytes from @p dev_addr. */
    int32_t (*receive)(drv_iic_handle_t *h,
                       uint16_t  dev_addr,
                       uint8_t   reg_addr,
                       uint8_t   use_reg,
                       uint8_t  *data,
                       uint16_t  len,
                       uint32_t  timeout_ms);

    /** @brief Ping @p dev_addr — returns OS_ERR_NONE if device ACKs. */
    int32_t (*is_device_ready)(drv_iic_handle_t *h,
                               uint16_t dev_addr,
                               uint32_t timeout_ms);
} drv_iic_hal_ops_t;


/* ── SPI ops ──────────────────────────────────────────────────────────── */
typedef struct drv_spi_hal_ops {
    int32_t (*hw_init)(drv_spi_handle_t *h);
    int32_t (*hw_deinit)(drv_spi_handle_t *h);

    /** @brief Transmit-only — ignores MISO. */
    int32_t (*transmit)(drv_spi_handle_t *h,
                        const uint8_t *data,
                        uint16_t       len,
                        uint32_t       timeout_ms);

    /** @brief Receive-only — drives MOSI low. */
    int32_t (*receive)(drv_spi_handle_t *h,
                       uint8_t  *data,
                       uint16_t  len,
                       uint32_t  timeout_ms);

    /** @brief Full-duplex transfer — TX and RX simultaneously. */
    int32_t (*transfer)(drv_spi_handle_t *h,
                        const uint8_t *tx,
                        uint8_t       *rx,
                        uint16_t       len,
                        uint32_t       timeout_ms);
} drv_spi_hal_ops_t;


/* ── GPIO ops ─────────────────────────────────────────────────────────── */
typedef struct drv_gpio_hal_ops {
    int32_t (*hw_init)(drv_gpio_handle_t *h);
    int32_t (*hw_deinit)(drv_gpio_handle_t *h);
    void    (*set)(drv_gpio_handle_t *h);
    void    (*clear)(drv_gpio_handle_t *h);
    void    (*toggle)(drv_gpio_handle_t *h);
    uint8_t (*read)(drv_gpio_handle_t *h);
    void    (*write)(drv_gpio_handle_t *h, uint8_t state);
} drv_gpio_hal_ops_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 4.  Generic handle structs
 * ═══════════════════════════════════════════════════════════════════════════ */

/** @brief Generic UART/USART peripheral handle. */
struct drv_uart_handle {
    uint8_t                   dev_id;        /**< OS-level device index (0-based) */
    uint8_t                   initialized;   /**< 1 after hw_init succeeds         */
    drv_hw_uart_ctx_t         hw;            /**< Vendor hardware context          */
    const drv_uart_hal_ops_t *ops;           /**< Bound HAL ops table              */
    uint32_t                  baudrate;      /**< Configured baud rate             */
    uint32_t                  timeout_ms;    /**< Default blocking timeout         */
    int32_t                   last_err;      /**< Last error code (OS_ERR_*)       */
};

/** @brief Generic I2C peripheral handle. */
struct drv_iic_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_iic_ctx_t         hw;
    const drv_iic_hal_ops_t *ops;
    uint32_t                 clock_hz;       /**< I2C bus clock (e.g. 400000)     */
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/** @brief Generic SPI peripheral handle. */
struct drv_spi_handle {
    uint8_t                  dev_id;
    uint8_t                  initialized;
    drv_hw_spi_ctx_t         hw;
    const drv_spi_hal_ops_t *ops;
    uint32_t                 clock_hz;       /**< SPI clock speed                 */
    uint32_t                 timeout_ms;
    int32_t                  last_err;
};

/** @brief Generic GPIO pin handle. */
struct drv_gpio_handle {
    uint8_t                   dev_id;        /**< Logical GPIO line ID            */
    uint8_t                   initialized;
    drv_hw_gpio_ctx_t         hw;
    const drv_gpio_hal_ops_t *ops;
    int32_t                   last_err;
};


/* ═══════════════════════════════════════════════════════════════════════════
 * 5.  Handle initialiser macros (zero-initialise + set ids/config)
 * ═══════════════════════════════════════════════════════════════════════════ */

#define DRV_UART_HANDLE_INIT(_id, _baud, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .baudrate = (_baud), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

#define DRV_IIC_HANDLE_INIT(_id, _clk, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .clock_hz = (_clk), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

#define DRV_SPI_HANDLE_INIT(_id, _clk, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .clock_hz = (_clk), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

#define DRV_GPIO_HANDLE_INIT(_id) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, .last_err = OS_ERR_NONE }


/* ═══════════════════════════════════════════════════════════════════════════
 * 6.  Generic driver registration API
 *     Upper layers call these to bind an ops table and trigger hw_init.
 *     Implemented in the respective drv_xxx.c files.
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifdef __cplusplus
extern "C" {
#endif

int32_t drv_uart_register(uint8_t dev_id, const drv_uart_hal_ops_t *ops, uint32_t baudrate, uint32_t timeout_ms);
int32_t drv_iic_register (uint8_t dev_id, const drv_iic_hal_ops_t  *ops, uint32_t clock_hz,  uint32_t timeout_ms);
int32_t drv_spi_register (uint8_t dev_id, const drv_spi_hal_ops_t  *ops, uint32_t clock_hz,  uint32_t timeout_ms);
int32_t drv_gpio_register(uint8_t dev_id, const drv_gpio_hal_ops_t *ops);

drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id);
drv_iic_handle_t  *drv_iic_get_handle (uint8_t dev_id);
drv_spi_handle_t  *drv_spi_get_handle (uint8_t dev_id);
drv_gpio_handle_t *drv_gpio_get_handle(uint8_t dev_id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DRV_HANDLE_H_ */
