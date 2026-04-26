/**
 * @file    drv_uart.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic UART driver interface, handle definition, and HAL abstraction
 *
 * @details
 * This header defines the generic UART driver interface used across the
 * FreeRTOS-OS project. It provides:
 *
 * - UART handle structure (drv_uart_handle_t)
 * - Vendor-specific hardware context abstraction (drv_hw_uart_ctx_t)
 * - HAL operations table (drv_uart_hal_ops_t)
 * - Public APIs for UART communication
 *
 * The design follows a hardware abstraction model where all MCU-specific
 * implementations are encapsulated in a HAL layer. The driver interacts
 * with hardware exclusively through the HAL operations table.
 *
 * Supported platforms are selected via CONFIG_DEVICE_VARIANT and include:
 * - STM32 (HAL-based)
 * - Infineon (register-level abstraction)
 *
 * @dependencies
 * def_std.h, def_err.h, board/mcu_config.h
 *
 * @note
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_DRIVERS_COM_DRV_UART_H_
#define INCLUDE_DRIVERS_COM_DRV_UART_H_

#include <def_attributes.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */
#include <board/board_handles.h>


/* ── Forward declaration ───────────────────────────────────────────────── */
/**
 * @brief UART driver handle forward declaration
 */
typedef struct drv_uart_handle drv_uart_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
/**
 * @brief Vendor-specific UART hardware context
 *
 * @details
 * This structure abstracts the underlying hardware representation
 * depending on the selected MCU platform.
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

/**
 * @brief STM32 UART hardware context
 */
typedef struct 
{
    UART_HandleTypeDef huart;
    
} drv_hw_uart_ctx_t;

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

/**
 * @brief Infineon UART hardware context
 */
typedef struct {
    uint32_t channel_base;
    uint32_t baudrate;
} drv_hw_uart_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check board_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
/**
 * @brief UART HAL operations table
 *
 * @details
 * Contains function pointers for hardware-specific operations.
 * Each platform must implement this interface to integrate with
 * the generic UART driver.
 */
typedef struct drv_uart_hal_ops {

    /**
     * @brief Initialize UART hardware
     * @param h UART handle
     * @return OS_ERR_NONE on success
     */
    int32_t (*hw_init)(drv_uart_handle_t *h);

    /**
     * @brief Deinitialize UART hardware
     * @param h UART handle
     * @return OS_ERR_NONE on success
     */
    int32_t (*hw_deinit)(drv_uart_handle_t *h);

    /**
     * @brief Blocking transmit
     * @param h UART handle
     * @param data TX buffer
     * @param len Number of bytes
     * @param timeout_ms Timeout in ms
     * @return OS_ERR_NONE on success
     */
    int32_t (*transmit)(drv_uart_handle_t *h,
                        const uint8_t *data,
                        uint16_t len,
                        uint32_t timeout_ms);

    /**
     * @brief Blocking receive
     * @param h UART handle
     * @param data RX buffer
     * @param len Number of bytes
     * @param timeout_ms Timeout in ms
     * @return OS_ERR_NONE on success
     */
    int32_t (*receive)(drv_uart_handle_t *h,
                       uint8_t *data,
                       uint16_t len,
                       uint32_t timeout_ms);

    /**
     * @brief Start interrupt-driven TX
     *
     * @details
     * Typically enables TXE interrupt to begin draining the TX ring buffer.
     *
     * @param h UART handle
     */
    void (*tx_start)(drv_uart_handle_t *h);

} drv_uart_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
/**
 * @brief UART driver handle structure
 *
 * @details
 * Maintains runtime state of a UART device, including configuration,
 * HAL bindings, and error status.
 */
struct drv_uart_handle {
    uint8_t                   dev_id;       /**< Device index */
    uint8_t                   initialized; /**< Initialization state */
    drv_hw_uart_ctx_t         hw;           /**< Hardware context */
    const drv_uart_hal_ops_t *ops;          /**< HAL operations */
    uint32_t                  baudrate;     /**< Configured baudrate */
    uint32_t                  timeout_ms;   /**< Blocking timeout */
    int32_t                   last_err;     /**< Last error code */
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
/**
 * @brief Initialize UART handle with default values
 */
#define DRV_UART_HANDLE_INIT(_id, _baud, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .baudrate = (_baud), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

#if (BOARD_UART_COUNT > 0)

/**
 * @brief Register UART device and initialize hardware
 */
int32_t drv_uart_register(uint8_t dev_id,
                          const drv_uart_hal_ops_t *ops,
                          uint32_t baudrate,
                          uint32_t timeout_ms);

/**
 * @brief Get UART handle
 */
drv_uart_handle_t *drv_uart_get_handle(uint8_t dev_id);

/**
 * @brief Start TX interrupt
 */
int32_t drv_uart_tx_kick(uint8_t dev_id);

/**
 * @brief Get next TX byte (ISR context)
 */
int32_t drv_uart_tx_get_next_byte(uint8_t dev_id, uint8_t *byte);

/**
 * @brief Blocking transmit API
 */
int32_t drv_serial_transmit(uint8_t dev_id,
                            const uint8_t *data,
                            uint16_t len);

/**
 * @brief Blocking receive API
 */
int32_t drv_serial_receive(uint8_t dev_id,
                           uint8_t *data,
                           uint16_t len);

#endif /* BOARD_UART_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_COM_DRV_UART_H_ */