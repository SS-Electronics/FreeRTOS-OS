/*
 * gpio_app.h — Application-level GPIO API
 *
 * Combines direct driver access (for fast, zero-latency operations) with
 * management-queue operations (for delayed or ISR-deferred commands).
 *
 * Use the async variants from ISR or performance-critical contexts where
 * you cannot afford to call the driver directly. Use gpio_read() for
 * immediate pin-state sampling (bypasses the management thread).
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_APP_GPIO_APP_H_
#define FREERTOS_OS_INCLUDE_DRV_APP_GPIO_APP_H_

#include <def_std.h>
#include <def_err.h>
#include <services/gpio_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Immediate (direct driver call, no queue) ────────────────────────── */

/**
 * @brief  Read current logical state of a GPIO pin.
 *
 * Calls the driver read() op directly — no management queue involved.
 * Safe to call from any task at any time after gpio_mgmt has initialised.
 *
 * @param  gpio_id  Logical GPIO line ID.
 * @return 1 if pin is at its active state, 0 otherwise, 0xFF on error.
 */
uint8_t gpio_read(uint8_t gpio_id);

/* ── Asynchronous via management queue ──────────────────────────────── */

/**
 * @brief  Set pin to its active state (non-blocking).
 */
int32_t gpio_async_set(uint8_t gpio_id);

/**
 * @brief  Clear pin to its inactive state (non-blocking).
 */
int32_t gpio_async_clear(uint8_t gpio_id);

/**
 * @brief  Toggle pin state (non-blocking).
 */
int32_t gpio_async_toggle(uint8_t gpio_id);

/**
 * @brief  Write an explicit 0/1 level to the pin (non-blocking).
 * @param  state  1 = active state, 0 = inactive state.
 */
int32_t gpio_async_write(uint8_t gpio_id, uint8_t state);

/**
 * @brief  Schedule a GPIO command with an optional pre-execution delay.
 *
 * The management thread waits @p delay_ms before executing the command.
 * Useful for generating reset pulses or timed enable sequences.
 *
 * @param  gpio_id   Logical GPIO line ID.
 * @param  cmd       Command (GPIO_MGMT_CMD_SET / CLEAR / TOGGLE / WRITE).
 * @param  state     State value (only used for GPIO_MGMT_CMD_WRITE).
 * @param  delay_ms  Delay before executing the command (0 = immediate).
 */
int32_t gpio_async_delayed(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                            uint8_t state, uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_APP_GPIO_APP_H_ */
