/**
 * @file    gpio_mgmt.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   GPIO management service thread interface
 *
 * @details
 * This module provides a message-driven GPIO management service built on top
 * of FreeRTOS.
 *
 * The GPIO management thread acts as a centralized controller for logical GPIO
 * lines registered in the system. It enables safe cross-task GPIO manipulation
 * using a queue-based command interface.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Design Philosophy
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * GPIO operations are generally fast and are often performed directly via the
 * GPIO driver (drv_gpio_* APIs). However, this service layer is required for:
 *
 *   • Deferred GPIO execution (timed activation/deactivation)
 *   • Scheduled operations (LED blink patterns, heartbeat signals)
 *   • Centralized GPIO initialization during system boot
 *   • Decoupling application logic from hardware access
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Execution Model
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   Task / ISR
 *       │
 *       ▼
 * gpio_mgmt_post()
 *       │
 *       ▼
 * FreeRTOS Queue
 *       │
 *       ▼
 * gpio_mgmt_thread (service task)
 *       │
 *       ├── Validate GPIO handle
 *       ├── Apply optional delay
 *       └── Execute GPIO operation via drv_gpio HAL
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Supported Commands
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   GPIO_MGMT_CMD_SET     → Set GPIO high (active state)
 *   GPIO_MGMT_CMD_CLEAR   → Set GPIO low
 *   GPIO_MGMT_CMD_TOGGLE  → Toggle GPIO state
 *   GPIO_MGMT_CMD_WRITE   → Write explicit logic level
 *   GPIO_MGMT_CMD_REINIT  → Reinitialize GPIO hardware context
 *
 * @note
 * This service is enabled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_MGMT_GPIO_MGMT_H_
#define DRIVERS_MGMT_GPIO_MGMT_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <board/mcu_config.h>
#include <config/conf_os.h>
#include <drivers/drv_gpio.h>

#include <os/kernel.h>
#include <os/kernel_mem.h>

/* ────────────────────────────────────────────────────────────────────────── */
/*                              GPIO Commands                               */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief GPIO management command types
 */
typedef enum {
    GPIO_MGMT_CMD_SET = 0,   /**< Set GPIO to active state                     */
    GPIO_MGMT_CMD_CLEAR,     /**< Set GPIO to inactive state                   */
    GPIO_MGMT_CMD_TOGGLE,    /**< Toggle current GPIO state                    */
    GPIO_MGMT_CMD_WRITE,     /**< Write explicit logic level (state field)     */
    GPIO_MGMT_CMD_REINIT,    /**< Reinitialize GPIO hardware configuration     */
} gpio_mgmt_cmd_t;

/**
 * @brief GPIO management message structure
 *
 * @details
 * This structure is passed through the management queue to request a GPIO
 * operation from the service thread.
 */
typedef struct {
    gpio_mgmt_cmd_t cmd;      /**< GPIO operation command                        */
    uint8_t         gpio_id;  /**< Logical GPIO ID (drv_gpio registration ID)   */
    uint8_t         state;    /**< Used for WRITE command (0/1 logic level)     */
    uint32_t        delay_ms;  /**< Delay before execution (0 = immediate)        */
} gpio_mgmt_msg_t;

/* ────────────────────────────────────────────────────────────────────────── */
/*                              Public API                                   */
/* ────────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start GPIO management service thread
 *
 * @details
 * Creates the GPIO management queue and launches the service thread that
 * processes all GPIO requests asynchronously.
 *
 * @return OS_ERR_NONE on success
 * @return OS_ERR_MEM_OF if queue allocation fails
 * @return OS_ERR_OP if thread creation fails
 */
int32_t gpio_mgmt_start(void);

/**
 * @brief Get GPIO management queue handle
 *
 * @return QueueHandle_t valid queue handle if initialized, otherwise NULL
 */
QueueHandle_t gpio_mgmt_get_queue(void);

/**
 * @brief Post a GPIO management command
 *
 * @details
 * Sends a GPIO operation request to the management thread. This function is
 * non-blocking and safe to call from any task or ISR context (depending on
 * FreeRTOS configuration of xQueueSend).
 *
 * @param gpio_id   Logical GPIO identifier registered in drv_gpio layer
 * @param cmd       GPIO operation command type
 * @param state     Output state (used only for GPIO_MGMT_CMD_WRITE)
 * @param delay_ms  Execution delay in milliseconds (0 = immediate execution)
 *
 * @return OS_ERR_NONE on success
 * @return OS_ERR_OP if queue is not available or send fails
 */
int32_t gpio_mgmt_post(uint8_t gpio_id,
                       gpio_mgmt_cmd_t cmd,
                       uint8_t state,
                       uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_GPIO_MGMT_H_ */