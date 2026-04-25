/**
 * @file    gpio_mgmt.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   GPIO management service thread (FreeRTOS-based service layer)
 *
 * @details
 * This module implements a centralized GPIO management service running as a
 * dedicated RTOS thread. It is responsible for:
 *
 *  - Initializing all GPIOs defined in board configuration
 *  - Registering GPIO HAL backend operations
 *  - Applying initial states for output pins
 *  - Processing asynchronous GPIO control requests via queue
 *
 * Architecture:
 *
 *   Board Config
 *        │
 *        ▼
 *   gpio_mgmt_thread (service task)
 *        │
 *        ├── GPIO hardware initialization
 *        ├── HAL ops binding (STM32 / other MCUs)
 *        └── Queue-driven GPIO command execution
 *
 * Supported commands:
 *   - SET
 *   - CLEAR
 *   - TOGGLE
 *   - WRITE
 *   - REINIT
 *
 * This design decouples application logic from direct GPIO manipulation,
 * improving modularity and testability.
 *
 * @note
 * This module is enabled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <services/gpio_mgmt.h>
#include <os/kernel.h>
#include <drivers/drv_gpio.h>
#include <board/mcu_config.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/hal/stm32/hal_gpio_stm32.h>
#endif

/* ────────────────────────────────────────────────────────────────────────── */
/*                          Internal Service Queue                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief GPIO management command queue handle
 */
__SECTION_OS_DATA __USED
static QueueHandle_t _mgmt_queue = NULL;

/* ────────────────────────────────────────────────────────────────────────── */
/*                        GPIO Management Thread                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief GPIO management service thread entry function
 *
 * @param arg Unused parameter (RTOS thread interface requirement)
 *
 * @details
 * This thread performs:
 *  1. Delayed startup for system stabilization
 *  2. GPIO hardware initialization from board config
 *  3. HAL ops registration (platform-specific backend)
 *  4. Continuous processing of GPIO command queue
 */
__SECTION_OS __USED
static void gpio_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_GPIO_MANAGEMENT);

    /* ── GPIO hardware initialization from board configuration ─────────── */
    {
        const drv_gpio_hal_ops_t *ops =
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
            hal_gpio_stm32_get_ops();
#else
            NULL;
#endif

        if (ops != NULL)
        {
            const board_config_t *bc = board_get_config();

            for (uint8_t i = 0; i < bc->gpio_count; i++)
            {
                const board_gpio_desc_t *d = &bc->gpio_table[i];
                drv_gpio_handle_t       *h = drv_gpio_get_handle(d->dev_id);

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
                board_clk_enable();

                hal_gpio_stm32_set_config(
                    h,
                    d->port,
                    d->pin,
                    d->mode,
                    d->pull,
                    d->speed,
                    d->active_state
                );
#endif

                (void)drv_gpio_register(d->dev_id, ops);

                /* Apply initial state for output pins */
                if (h->initialized && d->initial_state && h->ops != NULL)
                    h->ops->set(h);
            }
        }
    }

    /* ── GPIO command processing loop ──────────────────────────────────── */

    gpio_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        if (msg.delay_ms > 0)
            os_thread_delay(msg.delay_ms);

        drv_gpio_handle_t *h = drv_gpio_get_handle(msg.gpio_id);

        if (h == NULL || h->ops == NULL || !h->initialized)
            continue;

        switch (msg.cmd)
        {
            case GPIO_MGMT_CMD_SET:
                h->ops->set(h);
                break;

            case GPIO_MGMT_CMD_CLEAR:
                h->ops->clear(h);
                break;

            case GPIO_MGMT_CMD_TOGGLE:
                h->ops->toggle(h);
                break;

            case GPIO_MGMT_CMD_WRITE:
                h->ops->write(h, msg.state);
                break;

            case GPIO_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h);
                h->ops->hw_init(h);
                break;

            default:
                break;
        }
    }
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                           Public API                                     */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Start GPIO management service
 *
 * @return OS_ERR_NONE on success
 * @return OS_ERR_MEM_OF if queue allocation fails
 * @return OS_ERR_OP if thread creation fails
 */
__SECTION_OS __USED
int32_t gpio_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(GPIO_MGMT_QUEUE_DEPTH,
                               sizeof(gpio_mgmt_msg_t));

    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(
        gpio_mgmt_thread,
        "MGMT_GPIO",
        PROC_SERVICE_GPIO_MGMT_STACK_SIZE,
        PROC_SERVICE_GPIO_MGMT_PRIORITY,
        NULL
    );

    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief Get GPIO management queue handle
 *
 * @return QueueHandle_t queue reference or NULL if not initialized
 */
__SECTION_OS __USED
QueueHandle_t gpio_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

/**
 * @brief Post GPIO management command
 *
 * @param gpio_id   GPIO identifier
 * @param cmd       GPIO operation command
 * @param state     Optional state (used for WRITE command)
 * @param delay_ms  Execution delay before applying command
 *
 * @return OS_ERR_NONE on success
 * @return OS_ERR_OP if queue is not available or send fails
 */
__SECTION_OS __USED
int32_t gpio_mgmt_post(uint8_t gpio_id,
                       gpio_mgmt_cmd_t cmd,
                       uint8_t state,
                       uint32_t delay_ms)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    gpio_mgmt_msg_t msg = {
        .cmd      = cmd,
        .gpio_id  = gpio_id,
        .state    = state,
        .delay_ms = delay_ms,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
        ? OS_ERR_NONE
        : OS_ERR_OP;
}