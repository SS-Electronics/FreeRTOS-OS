/**
 * @file    spi_mgmt.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  services
 * @brief   SPI management service thread for serialized SPI access
 *
 * @details
 * This module implements a dedicated SPI management service thread that
 * serializes all SPI bus transactions across the system. It ensures safe
 * concurrent access to SPI peripherals from multiple FreeRTOS tasks.
 *
 * All SPI operations are executed through a message queue and processed
 * by a single management thread, preventing race conditions and bus
 * contention.
 *
 * The service supports interrupt-driven SPI transfers using HAL callbacks
 * where available, and falls back to blocking APIs when interrupts are
 * not supported.
 *
 * IRQ-based execution flow:
 * @code
 * Management thread
 *      ↓
 * h->notify_task = currentTask()
 *      ↓
 * SPI transfer_it() (HAL driver)
 *      ↓
 * ISR callback (HAL_SPI_*CpltCallback)
 *      ↓
 * irq_notify_from_isr(SPI_DONE)
 *      ↓
 * vTaskNotifyGiveFromISR()
 *      ↓
 * ulTaskNotifyTake() resumes thread
 *      ↓
 * h->last_err collected
 * @endcode
 *
 * Thread lifecycle:
 * - spi_mgmt_start() creates queue and service thread
 * - Thread delays startup until system stabilizes
 * - All SPI buses are registered using board configuration
 * - IRQ subscriptions are installed for TX/RX/ERROR events
 * - Thread enters infinite message processing loop
 *
 * Supported commands:
 * - TRANSMIT   : SPI write-only transfer
 * - RECEIVE    : SPI read-only transfer
 * - TRANSFER   : Full-duplex SPI transfer
 * - REINIT     : Hardware reinitialization
 *
 * @dependencies
 * def_attributes.h, def_compiler.h, def_std.h, def_err.h,
 * conf_os.h, services/spi_mgmt.h, os/kernel.h,
 * drivers/drv_spi.h, board/board_config.h, irq/irq_notify.h
 *
 * @note
 * - This module is part of FreeRTOS-OS Project
 * - Runs as a dedicated system service thread
 * - Uses FreeRTOS queue for request serialization
 * - Designed for both blocking and interrupt-driven SPI HAL layers
 *
 * @warning
 * - Buffers passed in messages must remain valid until completion
 * - Queue overflow results in command loss
 * - ISR callbacks must remain lightweight (notify only)
 *
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <services/spi_mgmt.h>
#include <os/kernel.h>
#include <drivers/drv_spi.h>
#include <board/board_config.h>
#include <irq/irq_notify.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/hal/stm32/hal_spi_stm32.h>
#endif

#if (BOARD_SPI_COUNT > 0)

/* ────────────────────────────────────────────────────────────────────────── */
/** @brief SPI management request queue handle */
__SECTION_OS_DATA __USED
static QueueHandle_t _mgmt_queue = NULL;

/* ────────────────────────────────────────────────────────────────────────── */
/**
 * @brief SPI transfer completion ISR callback
 *
 * @details Called from ISR context when SPI TX/RX/TRANSFER completes.
 * Notifies the waiting task via FreeRTOS task notification.
 */
__SECTION_OS __USED
static void _spi_done_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;

    drv_spi_handle_t *h = (drv_spi_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/**
 * @brief SPI error ISR callback
 *
 * @details Called when SPI hardware error occurs. Only signals task.
 */
__SECTION_OS __USED
static void _spi_err_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;

    drv_spi_handle_t *h = (drv_spi_handle_t *)arg;
    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/* ────────────────────────────────────────────────────────────────────────── */
/**
 * @brief SPI management service thread
 *
 * @details
 * Initializes SPI peripherals from board configuration and processes
 * queued SPI requests in an infinite loop.
 */
__SECTION_OS __USED
static void spi_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_SPI_MANAGEMENT);

    /* ── Register SPI peripherals ───────────────────────────────────────── */
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
                hal_spi_stm32_set_config(
                    h,
                    d->instance,
                    d->mode,
                    d->direction,
                    d->data_size,
                    d->clk_polarity,
                    d->clk_phase,
                    d->nss,
                    d->baud_prescaler,
                    d->bit_order
                );
#endif

                (void)drv_spi_register(d->dev_id, ops, 0, 10);

                irq_register(IRQ_ID_SPI_TX_DONE(d->dev_id),   _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_RX_DONE(d->dev_id),   _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_TXRX_DONE(d->dev_id), _spi_done_cb, h);
                irq_register(IRQ_ID_SPI_ERROR(d->dev_id),     _spi_err_cb,  h);
            }
        }
    }

    /* ── Main message loop ─────────────────────────────────────────────── */
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
                        ulTaskNotifyTake(pdTRUE,
                                         pdMS_TO_TICKS(h->timeout_ms));

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
                        ulTaskNotifyTake(pdTRUE,
                                         pdMS_TO_TICKS(h->timeout_ms));

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
                    result = h->ops->transfer_it(h, msg.tx_data,
                                                  msg.rx_data, msg.len);

                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE,
                                         pdMS_TO_TICKS(h->timeout_ms));

                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->transfer(h, msg.tx_data,
                                              msg.rx_data, msg.len,
                                              h->timeout_ms);
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

/* ────────────────────────────────────────────────────────────────────────── */
/** @brief Start SPI management service */
__SECTION_OS __USED
int32_t spi_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(SPI_MGMT_QUEUE_DEPTH,
                               sizeof(spi_mgmt_msg_t));

    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(spi_mgmt_thread,
                                   "MGMT_SPI",
                                   PROC_SERVICE_SPI_MGMT_STACK_SIZE,
                                   PROC_SERVICE_SPI_MGMT_PRIORITY,
                                   NULL);

    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

/** @brief Get SPI management queue */
__SECTION_OS __USED
QueueHandle_t spi_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

/** @brief Synchronous SPI transfer */
__SECTION_OS __USED
int32_t spi_mgmt_sync_transfer(uint8_t bus_id,
                                const uint8_t *tx,
                                uint8_t *rx,
                                uint16_t len,
                                uint32_t timeout_ms)
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

    if (xQueueSend(_mgmt_queue, &msg,
                   pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

/** @brief Async transmit */
__SECTION_OS __USED
int32_t spi_mgmt_async_transmit(uint8_t bus_id,
                                 const uint8_t *data,
                                 uint16_t len)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_TRANSMIT,
        .bus_id        = bus_id,
        .tx_data       = data,
        .rx_data       = NULL,
        .len           = len,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
           ? OS_ERR_NONE : OS_ERR_OP;
}

/** @brief Synchronous SPI transmit-only */
__SECTION_OS __USED
int32_t spi_mgmt_sync_transmit(uint8_t bus_id,
                                const uint8_t *data,
                                uint16_t len,
                                uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_TRANSMIT,
        .bus_id        = bus_id,
        .tx_data       = data,
        .rx_data       = NULL,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

/** @brief Synchronous SPI receive-only */
__SECTION_OS __USED
int32_t spi_mgmt_sync_receive(uint8_t bus_id,
                               uint8_t *data,
                               uint16_t len,
                               uint32_t timeout_ms)
{
    if (_mgmt_queue == NULL || data == NULL)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd           = SPI_MGMT_CMD_RECEIVE,
        .bus_id        = bus_id,
        .tx_data       = NULL,
        .rx_data       = data,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(_mgmt_queue, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

/** @brief Async full-duplex SPI transfer */
__SECTION_OS __USED
int32_t spi_mgmt_async_transfer(uint8_t bus_id,
                                 const uint8_t *tx,
                                 uint8_t *rx,
                                 uint16_t len)
{
    if (_mgmt_queue == NULL)
        return OS_ERR_OP;

    spi_mgmt_msg_t msg = {
        .cmd     = SPI_MGMT_CMD_TRANSFER,
        .bus_id  = bus_id,
        .tx_data = tx,
        .rx_data = rx,
        .len     = len,
    };

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
           ? OS_ERR_NONE : OS_ERR_OP;
}

#endif /* BOARD_SPI_COUNT > 0 */