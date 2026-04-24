/*
 * iic_mgmt.c — I2C management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The management thread serialises I2C operations onto each bus.  All
 * transfers use the interrupt-driven HAL ops (transmit_it / receive_it) so
 * the thread releases the CPU while the hardware works.  Completion is
 * signalled from the ISR callback via the generic IRQ notification framework:
 *
 *   mgmt thread                          ISR (HAL_I2C_*CpltCallback)
 *   ──────────                           ────────────────────────────
 *   h->notify_task = currentTask()
 *   ops->transmit_it(...)  ──────────►  irq_notify_from_isr(IRQ_ID_I2C_TX_DONE)
 *   ulTaskNotifyTake(...)  ◄──────────  vTaskNotifyGiveFromISR(h->notify_task)
 *   result = h->last_err
 */

#include <services/iic_mgmt.h>

#include <os/kernel.h>
#include <drivers/com/drv_iic.h>
#include <board/board_config.h>
#include <irq/irq_notify.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/com/hal/stm32/hal_iic_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <drivers/com/hal/infineon/hal_iic_infineon.h>
#endif

#if (BOARD_IIC_COUNT > 0)

static QueueHandle_t _mgmt_queue = NULL;

/* ── IRQ subscriber callbacks ─────────────────────────────────────────────── */

static void _iic_done_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;
    drv_iic_handle_t *h = (drv_iic_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

static void _iic_err_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;
    drv_iic_handle_t *h = (drv_iic_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/* ── Management thread ────────────────────────────────────────────────────── */

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

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
                hal_iic_stm32_set_config(h, d->instance,
                                         d->addr_mode, d->dual_addr);
#endif
                int32_t err = drv_iic_register(d->dev_id, ops,
                                               d->clock_hz, IIC_ACK_TIMEOUT_MS);
                (void)err;

                /* Subscribe for IT completion and error notifications */
                irq_register(IRQ_ID_I2C_TX_DONE(d->dev_id), _iic_done_cb, h);
                irq_register(IRQ_ID_I2C_RX_DONE(d->dev_id), _iic_done_cb, h);
                irq_register(IRQ_ID_I2C_ERROR(d->dev_id),   _iic_err_cb,  h);
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
                if (h->ops->transmit_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();
                    result = h->ops->transmit_it(h, msg.dev_addr, msg.reg_addr,
                                                  msg.use_reg, msg.data, msg.len);
                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(h->timeout_ms));
                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->transmit(h, msg.dev_addr, msg.reg_addr,
                                              msg.use_reg, msg.data, msg.len,
                                              h->timeout_ms);
                }
                break;

            case IIC_MGMT_CMD_RECEIVE:
                if (h->ops->receive_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();
                    result = h->ops->receive_it(h, msg.dev_addr, msg.reg_addr,
                                                 msg.use_reg, msg.data, msg.len);
                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(h->timeout_ms));
                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->receive(h, msg.dev_addr, msg.reg_addr,
                                             msg.use_reg, msg.data, msg.len,
                                             h->timeout_ms);
                }
                break;

            case IIC_MGMT_CMD_PROBE:
                /* HAL has no IT variant for IsDeviceReady — use blocking */
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

#endif /* BOARD_IIC_COUNT > 0 */
