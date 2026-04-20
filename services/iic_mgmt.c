/*
 * iic_mgmt.c — I2C management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <services/iic_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_handle.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/com/hal/stm32/hal_iic_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <drivers/com/hal/infineon/hal_iic_infineon.h>
#endif

#if (INC_SERVICE_IIC_MGMT == 1)

static QueueHandle_t _mgmt_queue = NULL;

static void iic_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_IIC_MANAGEMENT);

    /* Register all I2C buses described in the board configuration */
    {
        const drv_iic_hal_ops_t *ops =
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
            hal_iic_stm32_get_ops();
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
            hal_iic_infineon_get_ops();
#else
            NULL;
#endif
        if (ops != NULL)
        {
            const board_config_t *bc = board_get_config();
            for (uint8_t i = 0; i < bc->iic_count; i++)
            {
                const board_iic_desc_t *d = &bc->iic_table[i];
                drv_iic_handle_t       *h = drv_iic_get_handle(d->dev_id);

                /*
                 * Populate the vendor hw context (instance, addr_mode) BEFORE
                 * drv_iic_register() calls hw_init(), which reads these fields.
                 */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
                hal_iic_stm32_set_config(h, d->instance,
                                         d->addr_mode, d->dual_addr);
#endif
                int32_t err = drv_iic_register(d->dev_id, ops,
                                               d->clock_hz, IIC_ACK_TIMEOUT_MS);
                (void)err;
            }
        }
    }

    iic_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        drv_iic_handle_t *h = drv_iic_get_handle(msg.bus_id);
        if (h == NULL || h->ops == NULL)
            goto notify;

        int32_t result = OS_ERR_OP;

        switch (msg.cmd)
        {
            case IIC_MGMT_CMD_TRANSMIT:
                result = h->ops->transmit(h,
                                          msg.dev_addr,
                                          msg.reg_addr,
                                          msg.use_reg,
                                          msg.data,
                                          msg.len,
                                          h->timeout_ms);
                break;

            case IIC_MGMT_CMD_RECEIVE:
                result = h->ops->receive(h,
                                         msg.dev_addr,
                                         msg.reg_addr,
                                         msg.use_reg,
                                         msg.data,
                                         msg.len,
                                         h->timeout_ms);
                break;

            case IIC_MGMT_CMD_PROBE:
                result = h->ops->is_device_ready(h, msg.dev_addr, h->timeout_ms);
                break;

            case IIC_MGMT_CMD_REINIT:
                h->ops->hw_deinit(h);
                result = h->ops->hw_init(h);
                break;

            default:
                break;
        }

        if (msg.result_code != NULL)
            *msg.result_code = result;

notify:
        if (msg.result_notify != NULL)
            xTaskNotifyGive(msg.result_notify);
    }
}

int32_t iic_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(IIC_MGMT_QUEUE_DEPTH, sizeof(iic_mgmt_msg_t));
    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(iic_mgmt_thread,
                                   "iic_mgmt",
                                   PROC_SERVICE_IIC_MGMT_STACK_SIZE,
                                   PROC_SERVICE_IIC_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

QueueHandle_t iic_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

int32_t iic_mgmt_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                                 uint8_t reg_addr, uint8_t use_reg,
                                 const uint8_t *data, uint16_t len)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    iic_mgmt_msg_t msg = {
        .cmd           = IIC_MGMT_CMD_TRANSMIT,
        .bus_id        = bus_id,
        .dev_addr      = dev_addr,
        .reg_addr      = reg_addr,
        .use_reg       = use_reg,
        .data          = (uint8_t *)data,
        .len           = len,
        .result_notify = NULL,
        .result_code   = NULL,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

int32_t iic_mgmt_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    iic_mgmt_msg_t msg = {
        .cmd           = IIC_MGMT_CMD_TRANSMIT,
        .bus_id        = bus_id,
        .dev_addr      = dev_addr,
        .reg_addr      = reg_addr,
        .use_reg       = use_reg,
        .data          = (uint8_t *)data,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

int32_t iic_mgmt_sync_probe(uint8_t bus_id, uint16_t dev_addr,
                             uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    iic_mgmt_msg_t msg = {
        .cmd           = IIC_MGMT_CMD_PROBE,
        .bus_id        = bus_id,
        .dev_addr      = dev_addr,
        .reg_addr      = 0,
        .use_reg       = 0,
        .data          = NULL,
        .len           = 0,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

int32_t iic_mgmt_async_receive(uint8_t bus_id, uint16_t dev_addr,
                                uint8_t reg_addr, uint8_t use_reg,
                                uint8_t *data, uint16_t len)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    iic_mgmt_msg_t msg = {
        .cmd           = IIC_MGMT_CMD_RECEIVE,
        .bus_id        = bus_id,
        .dev_addr      = dev_addr,
        .reg_addr      = reg_addr,
        .use_reg       = use_reg,
        .data          = data,
        .len           = len,
        .result_notify = NULL,
        .result_code   = NULL,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

int32_t iic_mgmt_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                               uint8_t reg_addr, uint8_t use_reg,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    iic_mgmt_msg_t msg = {
        .cmd           = IIC_MGMT_CMD_RECEIVE,
        .bus_id        = bus_id,
        .dev_addr      = dev_addr,
        .reg_addr      = reg_addr,
        .use_reg       = use_reg,
        .data          = data,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

#endif /* INC_SERVICE_IIC_MGMT == 1 */
