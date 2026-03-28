/*
 * board_config.h — Board peripheral configuration (compile-time device table)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Design
 * ──────
 * Analogous to a Linux Device Tree, but resolved entirely at compile time as
 * C constant structs.  Every board declares one const board_config_t that
 * contains tables of peripheral descriptors (UART, I2C, SPI, GPIO).
 *
 * Drivers and MSP init functions query this single source of truth instead of
 * having pin assignments and peripheral parameters scattered across multiple
 * source files.
 *
 * Porting to a new board
 * ──────────────────────
 *   1. Create  boards/<board_name>/board_config.c
 *   2. Define  g_board_config  with your peripheral tables
 *   3. Pass    CONFIG_BOARD=<board_name>  to make  (or edit boards/Makefile)
 *   4. Rebuild — no other source files need to change
 *
 * Include path
 * ────────────
 *   #include <board/board_config.h>
 */

#ifndef INCLUDE_BOARD_BOARD_CONFIG_H_
#define INCLUDE_BOARD_BOARD_CONFIG_H_

#include <def_std.h>
#include <device.h>     /* CMSIS types: GPIO_TypeDef, USART_TypeDef, IRQn_Type … */

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * 1.  Pin descriptor
 *     Carries everything HAL_GPIO_Init() needs for one physical pin.
 *     Use alternate = 0 for non-AF (output/input) pins.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    GPIO_TypeDef *port;       /**< GPIOA … GPIOE                              */
    uint16_t      pin;        /**< GPIO_PIN_0 … GPIO_PIN_15                   */
    uint32_t      mode;       /**< GPIO_MODE_AF_PP, GPIO_MODE_OUTPUT_PP, …    */
    uint32_t      pull;       /**< GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN  */
    uint32_t      speed;      /**< GPIO_SPEED_FREQ_LOW / _MEDIUM / _HIGH …   */
    uint32_t      alternate;  /**< GPIO_AF7_USART1, GPIO_AF4_I2C1, …         */
} board_pin_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 2.  Peripheral descriptors
 *
 *     Only CMSIS register types (USART_TypeDef*, I2C_TypeDef*, SPI_TypeDef*)
 *     are used here.  These come from stm32f4xx.h and are always available
 *     regardless of which HAL modules are enabled.
 *     Vendor handle types (UART_HandleTypeDef etc.) live in drv_handle.h.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── UART / USART ──────────────────────────────────────────────────────── */
typedef struct {
    uint8_t        dev_id;            /**< UART_1 … UART_6  (mcu_config.h)   */
    USART_TypeDef *instance;          /**< USART1, USART2, USART6             */
    uint32_t       baudrate;
    uint32_t       word_len;          /**< UART_WORDLENGTH_8B / _9B           */
    uint32_t       stop_bits;         /**< UART_STOPBITS_1 / _2               */
    uint32_t       parity;            /**< UART_PARITY_NONE / _EVEN / _ODD    */
    uint32_t       mode;              /**< UART_MODE_TX_RX / _TX / _RX        */
    board_pin_t    tx_pin;
    board_pin_t    rx_pin;
    IRQn_Type      irqn;              /**< USARTx_IRQn                        */
    uint32_t       irq_priority;
    void         (*periph_clk_enable)(void); /**< __HAL_RCC_USARTx_CLK_ENABLE */
} board_uart_desc_t;

/* ── I2C ───────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t      dev_id;              /**< IIC_1 … IIC_3  (mcu_config.h)     */
    I2C_TypeDef *instance;            /**< I2C1, I2C2, I2C3                   */
    uint32_t     clock_hz;            /**< 100000 (standard) or 400000 (fast) */
    uint32_t     addr_mode;           /**< I2C_ADDRESSINGMODE_7BIT / _10BIT   */
    uint32_t     dual_addr;           /**< I2C_DUALADDRESS_DISABLE / _ENABLE  */
    board_pin_t  scl_pin;
    board_pin_t  sda_pin;
    IRQn_Type    ev_irqn;             /**< I2Cx_EV_IRQn                       */
    IRQn_Type    er_irqn;             /**< I2Cx_ER_IRQn                       */
    uint32_t     irq_priority;
    void       (*periph_clk_enable)(void);
} board_iic_desc_t;

/* ── SPI ───────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t      dev_id;              /**< Logical SPI index                  */
    SPI_TypeDef *instance;            /**< SPI1, SPI2, …                      */
    uint32_t     mode;                /**< SPI_MODE_MASTER / _SLAVE           */
    uint32_t     direction;           /**< SPI_DIRECTION_2LINES / _1LINE      */
    uint32_t     data_size;           /**< SPI_DATASIZE_8BIT / _16BIT         */
    uint32_t     clk_polarity;        /**< SPI_POLARITY_LOW / _HIGH           */
    uint32_t     clk_phase;           /**< SPI_PHASE_1EDGE / _2EDGE           */
    uint32_t     nss;                 /**< SPI_NSS_SOFT / _HARD_OUTPUT        */
    uint32_t     baud_prescaler;      /**< SPI_BAUDRATEPRESCALER_2 … _256     */
    uint32_t     bit_order;           /**< SPI_FIRSTBIT_MSB / _LSB            */
    board_pin_t  sck_pin;
    board_pin_t  miso_pin;
    board_pin_t  mosi_pin;
    board_pin_t  nss_pin;             /**< Set .pin = 0 for software NSS      */
    IRQn_Type    irqn;
    uint32_t     irq_priority;
    void       (*periph_clk_enable)(void);
} board_spi_desc_t;

/* ── General-purpose GPIO line ─────────────────────────────────────────── */
typedef struct {
    uint8_t       dev_id;             /**< Logical GPIO ID → gpio_mgmt        */
    const char   *label;              /**< e.g. "LED_RED", "nRESET"           */
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      mode;
    uint32_t      pull;
    uint32_t      speed;
    uint8_t       active_state;       /**< GPIO_PIN_SET or GPIO_PIN_RESET     */
    uint8_t       initial_state;      /**< 0 = inactive, 1 = active at boot   */
} board_gpio_desc_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 3.  Top-level board configuration struct
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char              *board_name;

    const board_uart_desc_t *uart_table;
    uint8_t                  uart_count;

    const board_iic_desc_t  *iic_table;
    uint8_t                  iic_count;

    const board_spi_desc_t  *spi_table;
    uint8_t                  spi_count;

    const board_gpio_desc_t *gpio_table;
    uint8_t                  gpio_count;
} board_config_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 4.  Runtime callback types
 *
 *     Application code registers callbacks via board_uart_register_rx_cb()
 *     etc.  The generated board_config.c holds the mutable callback tables.
 *     These types must match what the code generator produces.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Called from ISR/DMA when a byte is received on a UART. */
typedef void (*board_uart_rx_cb_t)(uint8_t dev_id, uint8_t byte);

/** Called when a UART transmit operation completes. */
typedef void (*board_uart_tx_done_cb_t)(uint8_t dev_id);

/** Called when a UART error is detected. */
typedef void (*board_uart_error_cb_t)(uint8_t dev_id, uint32_t error);

typedef struct {
    board_uart_rx_cb_t      on_rx_byte;
    board_uart_tx_done_cb_t on_tx_done;
    board_uart_error_cb_t   on_error;
} board_uart_cbs_t;

/** Called when an I2C master transfer (TX or RX) completes. */
typedef void (*board_iic_done_cb_t)(uint8_t dev_id);

/** Called on I2C error. */
typedef void (*board_iic_error_cb_t)(uint8_t dev_id, uint32_t error);

typedef struct {
    board_iic_done_cb_t  on_done;
    board_iic_error_cb_t on_error;
} board_iic_cbs_t;

/** Called when an SPI transfer completes. */
typedef void (*board_spi_done_cb_t)(uint8_t dev_id);

/** Called on SPI error. */
typedef void (*board_spi_error_cb_t)(uint8_t dev_id, uint32_t error);

typedef struct {
    board_spi_done_cb_t  on_done;
    board_spi_error_cb_t on_error;
} board_spi_cbs_t;

/** Called on GPIO interrupt (rising/falling/both edge). */
typedef void (*board_gpio_irq_cb_t)(uint8_t dev_id);

typedef struct {
    board_gpio_irq_cb_t on_irq;
} board_gpio_cbs_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * 5.  Board API
 *     Implemented in boards/<board>/board_config.c.
 *     Linked as a single translation unit selected by CONFIG_BOARD.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Return a pointer to the active board configuration.
 *         The returned pointer addresses a compile-time constant struct.
 */
const board_config_t    *board_get_config(void);

/**
 * @brief  Find the UART descriptor matching @p instance.
 * @return Pointer into uart_table, or NULL if not found.
 */
const board_uart_desc_t *board_find_uart(USART_TypeDef *instance);

/**
 * @brief  Find the I2C descriptor matching @p instance.
 */
const board_iic_desc_t  *board_find_iic(I2C_TypeDef *instance);

/**
 * @brief  Find the SPI descriptor matching @p instance.
 */
const board_spi_desc_t  *board_find_spi(SPI_TypeDef *instance);

/**
 * @brief  Enable the RCC peripheral clock for the GPIO port @p port.
 *         Call once per port before HAL_GPIO_Init().
 */
void board_gpio_clk_enable(GPIO_TypeDef *port);

/* ── Callback registration ─────────────────────────────────────────────── */

/* UART */
void board_uart_register_rx_cb(uint8_t dev_id, board_uart_rx_cb_t cb);
void board_uart_register_tx_done_cb(uint8_t dev_id, board_uart_tx_done_cb_t cb);
void board_uart_register_error_cb(uint8_t dev_id, board_uart_error_cb_t cb);
const board_uart_cbs_t *board_get_uart_cbs(uint8_t dev_id);

/* I2C */
void board_iic_register_done_cb(uint8_t dev_id, board_iic_done_cb_t cb);
void board_iic_register_error_cb(uint8_t dev_id, board_iic_error_cb_t cb);
const board_iic_cbs_t *board_get_iic_cbs(uint8_t dev_id);

/* SPI */
void board_spi_register_done_cb(uint8_t dev_id, board_spi_done_cb_t cb);
const board_spi_cbs_t *board_get_spi_cbs(uint8_t dev_id);

/* GPIO */
void board_gpio_register_irq_cb(uint8_t dev_id, board_gpio_irq_cb_t cb);
const board_gpio_cbs_t *board_get_gpio_cbs(uint8_t dev_id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_BOARD_BOARD_CONFIG_H_ */
