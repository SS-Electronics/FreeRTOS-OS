/*
 * gpio_mgmt.h — GPIO management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The GPIO management thread owns logical GPIO line handles and exposes a
 * simple message-based API for setting, clearing, toggling, and reading
 * GPIO pins from any FreeRTOS task context.
 *
 * Unlike UART/SPI/I2C, GPIO operations are very fast, so most callers will
 * call the generic gpio driver directly (drv_gpio_set / drv_gpio_clear).
 * The management thread is useful for:
 *   • Delayed GPIO operations (e.g. "assert reset for 10 ms, then release").
 *   • Scheduled blinking / heartbeat LEDs.
 *   • Centralised GPIO registration at startup.
 */

#ifndef DRIVERS_MGMT_GPIO_MGMT_H_
#define DRIVERS_MGMT_GPIO_MGMT_H_

#include <def_std.h>
#include <def_err.h>
#include <config/mcu_config.h>
#include <config/os_config.h>
#include <drivers/drv_handle.h>

#include "FreeRTOS.h"
#include "queue.h"

/* ── Message types ────────────────────────────────────────────────────── */

typedef enum {
    GPIO_MGMT_CMD_SET = 0,
    GPIO_MGMT_CMD_CLEAR,
    GPIO_MGMT_CMD_TOGGLE,
    GPIO_MGMT_CMD_WRITE,
    GPIO_MGMT_CMD_REINIT,
} gpio_mgmt_cmd_t;

typedef struct {
    gpio_mgmt_cmd_t cmd;
    uint8_t         gpio_id;    /**< Logical GPIO line ID (registered via drv_gpio_register) */
    uint8_t         state;      /**< For GPIO_MGMT_CMD_WRITE: desired output level            */
    uint32_t        delay_ms;   /**< Delay before executing the command (0 = immediate)       */
} gpio_mgmt_msg_t;

#ifndef GPIO_MGMT_QUEUE_DEPTH
#define GPIO_MGMT_QUEUE_DEPTH  16
#endif

#ifndef PROC_SERVICE_GPIO_MGMT_STACK_SIZE
#define PROC_SERVICE_GPIO_MGMT_STACK_SIZE  256
#endif
#ifndef PROC_SERVICE_GPIO_MGMT_PRIORITY
#define PROC_SERVICE_GPIO_MGMT_PRIORITY    1
#endif
#ifndef TIME_OFFSET_GPIO_MANAGEMENT
#define TIME_OFFSET_GPIO_MANAGEMENT        3000
#endif

/* ── Public API ───────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

int32_t       gpio_mgmt_start(void);
QueueHandle_t gpio_mgmt_get_queue(void);

/** @brief Post a GPIO command (non-blocking, delay_ms = 0 for immediate). */
int32_t gpio_mgmt_post(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                        uint8_t state, uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_MGMT_GPIO_MGMT_H_ */
