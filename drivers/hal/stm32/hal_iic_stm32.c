/**
 * @file    hal_iic_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL I2C driver implementation with IRQ-based completion dispatch
 *
 * @details
 * This file provides the STM32-specific implementation of the generic I2C
 * driver interface (`drv_iic_hal_ops_t`). It supports both blocking and
 * interrupt-driven communication modes.
 *
 * The driver abstracts STM32 HAL I2C operations while integrating with the
 * system-wide IRQ framework (`drv_irq`) to dispatch completion and error
 * events without relying on HAL weak callbacks.
 *
 * Key features:
 * - Blocking I2C transmit/receive APIs
 * - Interrupt-driven I2C operations (TX/RX)
 * - Device readiness probing
 * - Direct IRQ-based completion dispatch (no HAL callbacks)
 *
 * IRQ handling model:
 * - Event IRQ → hal_iic_stm32_ev_irq_handler()
 * - Error IRQ → hal_iic_stm32_er_irq_handler()
 *
 * Completion detection is performed by comparing HAL state transitions:
 *   BUSY → READY → operation complete
 *
 * -----------------------------------------------------------------------------
 * @dependencies
 * drv_iic.h, hal_iic_stm32.h, drv_irq.h, board_config.h
 *
 * @note
 * Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 *
 * This file is part of FreeRTOS-OS Project.
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
 */

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_iic.h>
#include <drivers/hal/stm32/hal_iic_stm32.h>
#include <drivers/drv_irq.h>
#include <board/board_config.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* Internal Helpers                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Convert HAL status to OS error code
 *
 * @param s HAL status
 *
 * @retval OS_ERR_NONE Success
 * @retval OS_ERR_OP   Failure
 */
__SECTION_OS __USED
static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL Operations Implementation                                             */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize I2C hardware
 *
 * @param h I2C handle
 *
 * @retval OS_ERR_NONE Success
 * @retval OS_ERR_OP   Failure
 */
__SECTION_OS __USED
static int32_t stm32_iic_hw_init(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    I2C_HandleTypeDef      *hi2c = &h->hw.hi2c;
    const board_iic_desc_t *d    = board_find_iic(hi2c->Instance);
    if (d == NULL)
        return OS_ERR_OP;

    GPIO_InitTypeDef gpio = {
        .Mode      = d->scl_pin.mode,
        .Pull      = d->scl_pin.pull,
        .Speed     = d->scl_pin.speed,
        .Alternate = d->scl_pin.alternate,
    };

    gpio.Pin = d->scl_pin.pin;
    HAL_GPIO_Init(d->scl_pin.port, &gpio);

    gpio.Pin       = d->sda_pin.pin;
    gpio.Alternate = d->sda_pin.alternate;
    HAL_GPIO_Init(d->sda_pin.port, &gpio);

    drv_irq_enable((int32_t)d->ev_irqn, d->irq_priority);
    drv_irq_enable((int32_t)d->er_irqn, d->irq_priority);

    hi2c->Init.ClockSpeed = h->clock_hz;

    HAL_StatusTypeDef ret = HAL_I2C_Init(hi2c);
    if (ret == HAL_OK)
        h->initialized = 1;

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Deinitialize I2C hardware
 *
 * @param h I2C handle
 *
 * @retval OS_ERR_NONE Success
 * @retval OS_ERR_OP   Failure
 */
__SECTION_OS __USED
static int32_t stm32_iic_hw_deinit(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    const board_iic_desc_t *d = board_find_iic(h->hw.hi2c.Instance);

    HAL_StatusTypeDef ret = HAL_I2C_DeInit(&h->hw.hi2c);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);

    if (d != NULL)
    {
        drv_irq_disable((int32_t)d->ev_irqn);
        drv_irq_disable((int32_t)d->er_irqn);
        HAL_GPIO_DeInit(d->scl_pin.port, d->scl_pin.pin);
        HAL_GPIO_DeInit(d->sda_pin.port, d->sda_pin.pin);
    }

    return h->last_err;
}

/**
 * @brief Blocking I2C transmit
 */
__SECTION_OS __USED
static int32_t stm32_iic_transmit(drv_iic_handle_t *h,
                                  uint16_t dev_addr,
                                  uint8_t  reg_addr,
                                  uint8_t  use_reg,
                                  const uint8_t *data,
                                  uint16_t len,
                                  uint32_t timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Write(&h->hw.hi2c, shifted, reg_addr,
                               I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)data, len, timeout_ms);
    else
        ret = HAL_I2C_Master_Transmit(&h->hw.hi2c,
                                     shifted,
                                     (uint8_t *)data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Blocking I2C receive
 */
__SECTION_OS __USED
static int32_t stm32_iic_receive(drv_iic_handle_t *h,
                                 uint16_t dev_addr,
                                 uint8_t  reg_addr,
                                 uint8_t  use_reg,
                                 uint8_t *data,
                                 uint16_t len,
                                 uint32_t timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Read(&h->hw.hi2c, shifted, reg_addr,
                              I2C_MEMADD_SIZE_8BIT,
                              data, len, timeout_ms);
    else
        ret = HAL_I2C_Master_Receive(&h->hw.hi2c,
                                    shifted, data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Check if I2C device is ready
 */
__SECTION_OS __USED
static int32_t stm32_iic_is_device_ready(drv_iic_handle_t *h,
                                         uint16_t dev_addr,
                                         uint32_t timeout_ms)
{
    if (h == NULL || !h->initialized)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_I2C_IsDeviceReady(&h->hw.hi2c,
                             (uint16_t)(dev_addr << 1),
                             3, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Ops table + public accessors                                              */
/* ────────────────────────────────────────────────────────────────────────── */

static const drv_iic_hal_ops_t _stm32_iic_ops = {
    .hw_init        = stm32_iic_hw_init,
    .hw_deinit      = stm32_iic_hw_deinit,
    .transmit       = stm32_iic_transmit,
    .receive        = stm32_iic_receive,
    .is_device_ready = stm32_iic_is_device_ready,
    .transmit_it    = NULL,
    .receive_it     = NULL,
};

const drv_iic_hal_ops_t *hal_iic_stm32_get_ops(void)
{
    return &_stm32_iic_ops;
}

void hal_iic_stm32_set_config(drv_iic_handle_t *h,
                              I2C_TypeDef      *instance,
                              uint32_t          addr_mode,
                              uint32_t          dual_addr)
{
    if (!h) return;
    h->hw.hi2c.Instance              = instance;
    h->hw.hi2c.Init.AddressingMode   = addr_mode;
    h->hw.hi2c.Init.DualAddressMode  = dual_addr;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Handlers                                                             */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief I2C event IRQ handler
 *
 * @details
 * Detects completion by observing state transition to READY.
 * Dispatches TX_DONE or RX_DONE accordingly.
 */
__SECTION_OS __USED
void hal_iic_stm32_ev_irq_handler(I2C_TypeDef *instance)
{
    for (uint8_t id = 0; id < BOARD_IIC_COUNT; id++)
    {
        drv_iic_handle_t *h = drv_iic_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.hi2c.Instance != instance)
            continue;

        HAL_I2C_StateTypeDef prev = HAL_I2C_GetState(&h->hw.hi2c);
        HAL_I2C_EV_IRQHandler(&h->hw.hi2c);
        HAL_I2C_StateTypeDef next = HAL_I2C_GetState(&h->hw.hi2c);

        if (next == HAL_I2C_STATE_READY && prev != HAL_I2C_STATE_READY)
        {
            h->last_err = OS_ERR_NONE;
            BaseType_t hpt = pdFALSE;

            if (prev == HAL_I2C_STATE_BUSY_TX)
                drv_irq_dispatch_from_isr(IRQ_ID_I2C_TX_DONE(id), NULL, &hpt);
            else
                drv_irq_dispatch_from_isr(IRQ_ID_I2C_RX_DONE(id), NULL, &hpt);

            portYIELD_FROM_ISR(hpt);
        }
        return;
    }
}

/**
 * @brief I2C error IRQ handler
 *
 * @details
 * Dispatches error event after HAL error handling.
 */
__SECTION_OS __USED
void hal_iic_stm32_er_irq_handler(I2C_TypeDef *instance)
{
    for (uint8_t id = 0; id < BOARD_IIC_COUNT; id++)
    {
        drv_iic_handle_t *h = drv_iic_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.hi2c.Instance != instance)
            continue;

        HAL_I2C_ER_IRQHandler(&h->hw.hi2c);

        uint32_t err = HAL_I2C_GetError(&h->hw.hi2c);
        h->last_err  = OS_ERR_OP;

        BaseType_t hpt = pdFALSE;
        drv_irq_dispatch_from_isr(IRQ_ID_I2C_ERROR(id), &err, &hpt);
        portYIELD_FROM_ISR(hpt);
        return;
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */