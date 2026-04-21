/*
 * spi_mgmt.c — SPI management service thread
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * All transfers use interrupt-driven HAL ops (transmit_it / receive_it /
 * transfer_it).  Completion is signalled from the ISR callback via the generic
 * IRQ notification framework:
 *
 *   mgmt thread                          ISR (HAL_SPI_*CpltCallback)
 *   ──────────                           ───────────────────────────
 *   h->notify_task = currentTask()
 *   ops->transfer_it(...)  ──────────►  irq_notify_from_isr(IRQ_ID_SPI_TXRX_DONE)
 *   ulTaskNotifyTake(...)  ◄──────────  vTaskNotifyGiveFromISR(h->notify_task)
 *   result = h->last_err
 */

#include <services/spi_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_handle.h>
#include <board/board_config.h>
#include <irq/irq_notify.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/com/hal/stm32/hal_spi_stm32.h>
#endif

#if (BOARD_SPI_COUNT > 0)

static QueueHandle_t _mgmt_queue = NULL;

/* ── IRQ subscriber callbacks ─────────────────────────────────────────────── */

static void _spi_done_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;
    drv_spi_handle_t *h = (drv_spi_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

static void _spi_err_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;
    drv_spi_handle_t *h = (drv_spi_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/* ── Management thread ────────────────────────────────────────────────────── */

static void spi_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_SPI_MANAGEMENT);

    /* Register all SPI buses described in the board configuration */
    {
        const drv_spi_hal_ops_t *ops =
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)
            hal_spi_stm32_get_ops();
#else
            NULL;
#endif
        if (ops != NULL)
        {
            const board_config_t *bc = board_get_config();
            for (uint8_t i = 0; i < bc->spi_count; i++)
            {
                const board_spi_desc_t *d = &bc->spi_table[i];
                drv_spi_handle_t       *h = drv_spi_get_handle(d->dev_id);

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)
                hal_spi_stm32_set_config(h,
                                         d->instance,
                                         d->mode,
                                         d->direction,
                                         d->data_size,
                                         d->clk_polarity,
                                         d->clk_phase,
                                         d->nss,
                                         d->baud_prescaler,
                                         d->bit_order);
#endif
                int32_t err = drv_spi_register(d->dev_id, ops, 0, 10);
                (void)err;

                /* Subscribe for IT completion and error notifications */
                irq_register(IRQ_ID_SPI_TX_DONE(d->dev_id),   _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_RX_DONE(d->dev_id),   _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_TXRX_DONE(d->dev_id), _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_ERROR(d->dev_id),      _spi_err_cb,  h);
            }
        }
    }

    spi_mgmt_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(_mgmt_queue, &msg, portMAX_DELAY) != pdTRUE)
            continue;

        drv_spi_handle_t *h = drv_spi_get_handle(msg.bus_id);
        if (h == NULL || h->ops == NULL)
            goto notify;

        int32_t result = OS_ERR_OP;

        switch (msg.cmd)
        {
            case SPI_MGMT_CMD_TRANSMIT:
                if (h->ops->transmit_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();
                    result = h->ops->transmit_it(h, msg.tx_data, msg.len);
                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(h->timeout_ms));
                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->transmit(h, msg.tx_data, msg.len,
                                              h->timeout_ms);
                }
                break;

            case SPI_MGMT_CMD_RECEIVE:
                if (h->ops->receive_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();
                    result = h->ops->receive_it(h, msg.rx_data, msg.len);
                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(h->timeout_ms));
                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->receive(h, msg.rx_data, msg.len,
                                             h->timeout_ms);
                }
                break;

            case SPI_MGMT_CMD_TRANSFER:
                if (h->ops->transfer_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();
                    result = h->ops->transfer_it(h, msg.tx_data, msg.rx_data,
                                                  msg.len);
                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(h->timeout_ms));
                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->transfer(h, msg.tx_data, msg.rx_data,
                                              msg.len, h->timeout_ms);
                }
                break;

            case SPI_MGMT_CMD_REINIT:
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

int32_t spi_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(SPI_MGMT_QUEUE_DEPTH, sizeof(spi_mgmt_msg_t));
    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(spi_mgmt_thread,
                                   "spi_mgmt",
                                   PROC_SERVICE_SPI_MGMT_STACK_SIZE,
                                   PROC_SERVICE_SPI_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

QueueHandle_t spi_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx, uint8_t *rx,
                                uint16_t len, uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_TRANSFER,
        .bus_id        = bus_id,
        .tx_data       = tx,
        .rx_data       = rx,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

int32_t spi_mgmt_sync_transmit(uint8_t bus_id,
                                const uint8_t *data, uint16_t len,
                                uint32_t timeout_ms)
{
    return spi_mgmt_sync_transfer(bus_id, data, NULL, len, timeout_ms);
}

int32_t spi_mgmt_sync_receive(uint8_t bus_id,
                               uint8_t *data, uint16_t len,
                               uint32_t timeout_ms)
{
    return spi_mgmt_sync_transfer(bus_id, NULL, data, len, timeout_ms);
}

int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data, uint16_t len)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_TRANSMIT,
        .bus_id        = bus_id,
        .tx_data       = data,
        .rx_data       = NULL,
        .len           = len,
        .result_notify = NULL,
        .result_code   = NULL,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

int32_t spi_mgmt_async_transfer(uint8_t bus_id,
                                 const uint8_t *tx, uint8_t *rx,
                                 uint16_t len)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_TRANSFER,
        .bus_id        = bus_id,
        .tx_data       = tx,
        .rx_data       = rx,
        .len           = len,
        .result_notify = NULL,
        .result_code   = NULL,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* BOARD_SPI_COUNT > 0 */
