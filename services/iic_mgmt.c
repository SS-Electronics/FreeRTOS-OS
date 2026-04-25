/**
 * @file    iic_mgmt.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   I2C management service thread (FreeRTOS-based serialized I2C engine)
 *
 * @details
 * This module implements a centralized I2C management service that serializes
 * all I2C transactions per bus and provides both synchronous and asynchronous
 * APIs for application tasks.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Architecture Overview
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   Application Task
 *         │
 *         ├── iic_mgmt_sync_* / iic_mgmt_async_*
 *         ▼
 *   FreeRTOS Queue (iic_mgmt_msg_t)
 *         │
 *         ▼
 *   iic_mgmt_thread (single I2C executor per system)
 *         │
 *         ├── Select HAL backend (STM32 / Infineon)
 *         ├── Dispatch I2C transaction (IT or polling mode)
 *         └── Wait for completion via RTOS notification
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Interrupt Completion Model
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   I2C Peripheral ISR (HAL layer)
 *         │
 *         ▼
 *   irq_notify_from_isr()
 *         │
 *         ▼
 *   vTaskNotifyGiveFromISR()
 *         │
 *         ▼
 *   iic_mgmt_thread resumes execution
 *
 * This ensures:
 *   - Non-blocking hardware execution
 *   - Deterministic serialization per bus
 *   - Safe cross-task synchronization
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Features
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   • Blocking (sync) I2C APIs
 *   • Non-blocking (async) I2C APIs
 *   • Register-based device access
 *   • Optional interrupt-driven transfers
 *   • Automatic fallback to polling mode if IT is unavailable
 *   • Device probe (is_device_ready)
 *   • Hardware reinitialization support
 *
 * @note
 * This module is enabled only when BOARD_IIC_COUNT > 0
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <config/conf_os.h>
#include <services/iic_mgmt.h>
#include <os/kernel.h>
#include <os/kernel_mem.h>
#include <drivers/drv_iic.h>
#include <board/board_config.h>
#include <board/mcu_config.h>
#include <irq/irq_notify.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#  include <drivers/hal/stm32/hal_iic_stm32.h>
#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)
#  include <drivers/hal/stm32/hal_iic_infineon.h>
#endif

#if (BOARD_IIC_COUNT > 0)

/* ────────────────────────────────────────────────────────────────────────── */
/*                           Internal Queue                                  */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief I2C management command queue handle
 */
__SECTION_OS_DATA __USED
static QueueHandle_t _mgmt_queue = NULL;

/* ────────────────────────────────────────────────────────────────────────── */
/*                     IRQ Notification Callbacks                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief I2C transfer completion ISR callback
 *
 * @details
 * Called from HAL I2C TX/RX complete interrupts. Wakes the waiting task.
 *
 * @param id     IRQ identifier
 * @param data   Unused
 * @param arg    drv_iic_handle_t pointer
 * @param pxHPT  FreeRTOS ISR context switch flag
 */
__SECTION_OS __USED
static void _iic_done_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;

    drv_iic_handle_t *h = (drv_iic_handle_t *)arg;

    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/**
 * @brief I2C error ISR callback
 *
 * @details
 * Signals task wake-up on bus error, arbitration loss, or timeout.
 */
__SECTION_OS __USED
static void _iic_err_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id;
    (void)data;

    drv_iic_handle_t *h = (drv_iic_handle_t *)arg;

    if (h->notify_task != NULL)
        vTaskNotifyGiveFromISR(h->notify_task, pxHPT);
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                        I2C Management Thread                              */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief I2C management service thread
 *
 * @param arg Unused RTOS parameter
 *
 * @details
 * This thread:
 *   1. Initializes all I2C buses from board configuration
 *   2. Registers HAL backend (STM32 / Infineon)
 *   3. Subscribes to IRQ completion notifications
 *   4. Processes queued I2C requests sequentially
 */
__SECTION_OS __USED
static void iic_mgmt_thread(void *arg)
{
    (void)arg;

    os_thread_delay(TIME_OFFSET_IIC_MANAGEMENT);

    /* ── Register all I2C buses from board configuration ──────────────── */

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

            (void)drv_iic_register(d->dev_id, ops,
                                    d->clock_hz,
                                    IIC_ACK_TIMEOUT_MS);

            /* IRQ subscription for async completion */
            irq_register(IRQ_ID_I2C_TX_DONE(d->dev_id), _iic_done_cb, h);
            irq_register(IRQ_ID_I2C_RX_DONE(d->dev_id), _iic_done_cb, h);
            irq_register(IRQ_ID_I2C_ERROR(d->dev_id),   _iic_err_cb,  h);
        }
    }

    /* ── Main request processing loop ─────────────────────────────────── */

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
            /* ── TRANSMIT ─────────────────────────────────────────────── */
            case IIC_MGMT_CMD_TRANSMIT:
                if (h->ops->transmit_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();

                    result = h->ops->transmit_it(
                        h, msg.dev_addr, msg.reg_addr,
                        msg.use_reg, msg.data, msg.len);

                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE,
                                         pdMS_TO_TICKS(h->timeout_ms));

                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->transmit(
                        h, msg.dev_addr, msg.reg_addr,
                        msg.use_reg, msg.data, msg.len,
                        h->timeout_ms);
                }
                break;

            /* ── RECEIVE ──────────────────────────────────────────────── */
            case IIC_MGMT_CMD_RECEIVE:
                if (h->ops->receive_it != NULL)
                {
                    h->notify_task = xTaskGetCurrentTaskHandle();

                    result = h->ops->receive_it(
                        h, msg.dev_addr, msg.reg_addr,
                        msg.use_reg, msg.data, msg.len);

                    if (result == OS_ERR_NONE)
                        ulTaskNotifyTake(pdTRUE,
                                         pdMS_TO_TICKS(h->timeout_ms));

                    result = h->last_err;
                    h->notify_task = NULL;
                }
                else
                {
                    result = h->ops->receive(
                        h, msg.dev_addr, msg.reg_addr,
                        msg.use_reg, msg.data, msg.len,
                        h->timeout_ms);
                }
                break;

            /* ── DEVICE PROBE ─────────────────────────────────────────── */
            case IIC_MGMT_CMD_PROBE:
                result = h->ops->is_device_ready(
                    h, msg.dev_addr, h->timeout_ms);
                break;

            /* ── REINITIALIZE ──────────────────────────────────────────── */
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

/* ────────────────────────────────────────────────────────────────────────── */
/*                              Public API                                  */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Start I2C management service
 */
__SECTION_OS __USED
int32_t iic_mgmt_start(void)
{
    _mgmt_queue = xQueueCreate(IIC_MGMT_QUEUE_DEPTH,
                               sizeof(iic_mgmt_msg_t));

    if (_mgmt_queue == NULL)
        return OS_ERR_MEM_OF;

    int32_t tid = os_thread_create(
        iic_mgmt_thread,
        "MGMT_IIC",
        PROC_SERVICE_IIC_MGMT_STACK_SIZE,
        PROC_SERVICE_IIC_MGMT_PRIORITY,
        NULL);

    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief Get I2C management queue handle
 */
__SECTION_OS __USED
QueueHandle_t iic_mgmt_get_queue(void)
{
    return _mgmt_queue;
}

/**
 * @brief Asynchronous I2C transmit
 */
__SECTION_OS __USED
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

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
        ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief Synchronous I2C transmit
 */
__SECTION_OS __USED
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

    if (xQueueSend(_mgmt_queue, &msg,
                   pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

/**
 * @brief Synchronous I2C device probe
 */
__SECTION_OS __USED
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

    if (xQueueSend(_mgmt_queue, &msg,
                   pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

/**
 * @brief Asynchronous I2C receive
 */
__SECTION_OS __USED
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

    return (xQueueSend(_mgmt_queue, &msg, 0) == pdTRUE)
        ? OS_ERR_NONE : OS_ERR_OP;
}

/**
 * @brief Synchronous I2C receive
 */
__SECTION_OS __USED
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

    if (xQueueSend(_mgmt_queue, &msg,
                   pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

#endif /* BOARD_IIC_COUNT > 0 */