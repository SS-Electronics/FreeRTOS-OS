/*
 * gpio_mgmt.c — GPIO management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <services/gpio_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_gpio.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/cpu/hal/stm32/hal_gpio_stm32.h>
#endif

static QueueHandle_t _mgmt_queue = NULL;

static void gpio_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_GPIO_MANAGEMENT);

    /* Register all GPIO lines described in the board configuration.
     * For each line: enable the GPIO clock, configure the pin parameters
     * in the hw context, then call drv_gpio_register() which calls hw_init(). */
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
                hal_gpio_stm32_set_config(h,
                                          d->port,
                                          d->pin,
                                          d->mode,
                                          d->pull,
                                          d->speed,
                                          d->active_state);
#endif
                int32_t err = drv_gpio_register(d->dev_id, ops);
                (void)err;

                /* Apply initial state for output pins */
                if (h->initialized && d->initial_state && h->ops != NULL)
                    h->ops->set(h);
            }
        }
    }

    gpio_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        /* Apply optional delay before executing */
        if (msg.delay_ms > 0)
            os_thread_delay(msg.delay_ms);

        drv_gpio_handle_t *h = drv_gpio_get_handle(msg.gpio_id);
        if (h == NULL || h->ops == NULL || !h->initialized)
            continue;

        switch (msg.cmd)
        {
            case GPIO_MGMT_CMD_SET:    h->ops->set(h);              break;
            case GPIO_MGMT_CMD_CLEAR:  h->ops->clear(h);            break;
            case GPIO_MGMT_CMD_TOGGLE: h->ops->toggle(h);           break;
            case GPIO_MGMT_CMD_WRITE:  h->ops->write(h, msg.state); break;
            case GPIO_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h);
                h->ops->hw_init(h);
                break;
            default: break;
        }
    }
}

int32_t gpio_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(GPIO_MGMT_QUEUE_DEPTH, sizeof(gpio_mgmt_msg_t));
    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(gpio_mgmt_thread,
                                   "gpio_mgmt",
                                   PROC_SERVICE_GPIO_MGMT_STACK_SIZE,
                                   PROC_SERVICE_GPIO_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

QueueHandle_t gpio_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

int32_t gpio_mgmt_post(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                        uint8_t state, uint32_t delay_ms)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    gpio_mgmt_msg_t msg = {
        .cmd      = cmd,
        .gpio_id  = gpio_id,
        .state    = state,
        .delay_ms = delay_ms,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}
