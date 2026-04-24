/**
 * @file    drv_spi.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic SPI driver interface with HAL abstraction and async support
 *
 * @details
 * This header defines the interface for the generic SPI driver used in the
 * FreeRTOS-OS project.
 *
 * It provides:
 * - SPI handle structure (drv_spi_handle_t)
 * - Vendor-specific hardware abstraction (drv_hw_spi_ctx_t)
 * - HAL operations table (drv_spi_hal_ops_t)
 * - Blocking and interrupt-driven (IT) SPI APIs
 *
 * The driver follows a hardware abstraction model where all MCU-specific
 * implementations are encapsulated in a HAL layer selected via
 * CONFIG_DEVICE_VARIANT.
 *
 * Supported platforms include:
 * - STM32 (HAL SPI-based)
 * - Infineon (register-level abstraction)
 *
 * Interrupt-driven (IT) APIs rely on ISR completion callbacks and can
 * optionally notify a FreeRTOS task via TaskHandle_t.
 *
 * Chip-select (CS) handling is not part of this driver and must be managed
 * externally by the caller.
 *
 * @dependencies
 * def_std.h, def_err.h, board/mcu_config.h,
 * FreeRTOS.h, task.h
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

#ifndef INCLUDE_DRIVERS_COM_DRV_SPI_H_
#define INCLUDE_DRIVERS_COM_DRV_SPI_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_handles.h>
#include <board/mcu_config.h>   /* CONFIG_DEVICE_VARIANT, MCU_VAR_* */
#include <os/kernel.h>


/* ── Forward declaration ───────────────────────────────────────────────── */
/**
 * @brief SPI driver handle forward declaration
 */
typedef struct drv_spi_handle drv_spi_handle_t;

/* ── Vendor hardware context ───────────────────────────────────────────── */
/**
 * @brief Vendor-specific SPI hardware context
 *
 * @details
 * Abstracts underlying SPI peripheral depending on MCU platform.
 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>

#ifdef HAL_SPI_MODULE_ENABLED
/**
 * @brief STM32 SPI hardware context
 */
typedef struct {
    SPI_HandleTypeDef hspi;
} drv_hw_spi_ctx_t;
#else
/**
 * @brief Placeholder context when SPI HAL is disabled
 */
typedef struct {
    uint32_t _reserved;
} drv_hw_spi_ctx_t;
#endif

#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

/**
 * @brief Infineon SPI hardware context
 */
typedef struct {
    uint32_t channel_base;
    uint32_t clock_hz;
} drv_hw_spi_ctx_t;

#else
#error "CONFIG_DEVICE_VARIANT not set or unsupported. Check mcu_config.h."
#endif

/* ── HAL ops table ─────────────────────────────────────────────────────── */
/**
 * @brief SPI HAL operations table
 *
 * @details
 * Provides abstraction for hardware-specific SPI operations including
 * blocking and interrupt-driven transfers.
 */
typedef struct drv_spi_hal_ops {

    /**
     * @brief Initialize SPI hardware
     */
    int32_t (*hw_init)(drv_spi_handle_t *h);

    /**
     * @brief Deinitialize SPI hardware
     */
    int32_t (*hw_deinit)(drv_spi_handle_t *h);

    /**
     * @brief Blocking transmit
     */
    int32_t (*transmit)(drv_spi_handle_t *h,
                        const uint8_t *data,
                        uint16_t len,
                        uint32_t timeout_ms);

    /**
     * @brief Blocking receive
     */
    int32_t (*receive)(drv_spi_handle_t *h,
                       uint8_t *data,
                       uint16_t len,
                       uint32_t timeout_ms);

    /**
     * @brief Blocking full-duplex transfer
     */
    int32_t (*transfer)(drv_spi_handle_t *h,
                        const uint8_t *tx,
                        uint8_t *rx,
                        uint16_t len,
                        uint32_t timeout_ms);

    /**
     * @brief Interrupt-based transmit
     *
     * @context Task → ISR completion
     */
    int32_t (*transmit_it)(drv_spi_handle_t *h,
                           const uint8_t *data,
                           uint16_t len);

    /**
     * @brief Interrupt-based receive
     *
     * @context Task → ISR completion
     */
    int32_t (*receive_it)(drv_spi_handle_t *h,
                          uint8_t *data,
                          uint16_t len);

    /**
     * @brief Interrupt-based full-duplex transfer
     *
     * @context Task → ISR completion
     */
    int32_t (*transfer_it)(drv_spi_handle_t *h,
                           const uint8_t *tx,
                           uint8_t *rx,
                           uint16_t len);

} drv_spi_hal_ops_t;

/* ── Handle struct ─────────────────────────────────────────────────────── */
/**
 * @brief SPI driver handle structure
 *
 * @details
 * Stores runtime state, configuration, HAL bindings, and RTOS notification
 * context for a SPI instance.
 */
struct drv_spi_handle {
    uint8_t                  dev_id;        /**< Device index */
    uint8_t                  initialized;  /**< Initialization state */
    drv_hw_spi_ctx_t         hw;            /**< Hardware context */
    const drv_spi_hal_ops_t *ops;           /**< HAL operations */
    uint32_t                 clock_hz;      /**< SPI clock frequency */
    uint32_t                 timeout_ms;    /**< Blocking timeout */
    int32_t                  last_err;      /**< Last error code */
    TaskHandle_t             notify_task;   /**< Task to notify on IT completion */
};

/* ── Handle initialiser macro ──────────────────────────────────────────── */
/**
 * @brief Initialize SPI handle with default values
 */
#define DRV_SPI_HANDLE_INIT(_id, _clk, _tmout) \
    { .dev_id = (_id), .initialized = 0, .ops = NULL, \
      .clock_hz = (_clk), .timeout_ms = (_tmout), .last_err = OS_ERR_NONE }

/* ── Public API ────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

#if (BOARD_SPI_COUNT > 0)

/**
 * @brief Register SPI device and initialize hardware
 *
 * @param dev_id      SPI device index
 * @param ops         HAL operations table
 * @param clock_hz    SPI clock frequency
 * @param timeout_ms  Blocking timeout
 *
 * @return OS_ERR_NONE on success
 *
 * @context Task (initialization phase)
 */
int32_t drv_spi_register(uint8_t dev_id,
                         const drv_spi_hal_ops_t *ops,
                         uint32_t clock_hz,
                         uint32_t timeout_ms);

/**
 * @brief Get SPI handle
 *
 * @param dev_id SPI device index
 * @return Pointer to handle or NULL
 *
 * @context Task/ISR
 */
drv_spi_handle_t *drv_spi_get_handle(uint8_t dev_id);

#endif /* BOARD_SPI_COUNT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_COM_DRV_SPI_H_ */