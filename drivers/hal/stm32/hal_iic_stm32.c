/*
 * hal_iic_stm32.c — STM32 HAL I2C ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Provides both blocking ops (transmit/receive) and interrupt-driven ops
 * (transmit_it/receive_it).  Completion events are dispatched directly from
 * hal_iic_stm32_ev_irq_handler() / hal_iic_stm32_er_irq_handler() by
 * inspecting HAL_I2C_GetState() before and after calling the HAL handler —
 * no HAL weak callbacks are used.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_iic_stm32.h>
#include <drivers/drv_irq.h>
#include <board/board_config.h>

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────────── */

static int32_t stm32_iic_hw_init(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    I2C_HandleTypeDef      *hi2c = &h->hw.hi2c;
    const board_iic_desc_t *d    = board_find_iic(hi2c->Instance);
    if (d == NULL)
        return OS_ERR_OP;

    /* GPIO and NVIC setup — owned here so HAL_I2C_MspInit (called internally
     * by HAL_I2C_Init below) is a no-op and init ownership stays in this fn. */
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

static int32_t stm32_iic_transmit(drv_iic_handle_t *h,
                                   uint16_t          dev_addr,
                                   uint8_t           reg_addr,
                                   uint8_t           use_reg,
                                   const uint8_t    *data,
                                   uint16_t          len,
                                   uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Write(&h->hw.hi2c, shifted, reg_addr,
                                I2C_MEMADD_SIZE_8BIT, (uint8_t *)data,
                                len, timeout_ms);
    else
        ret = HAL_I2C_Master_Transmit(&h->hw.hi2c, shifted,
                                      (uint8_t *)data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_iic_receive(drv_iic_handle_t *h,
                                  uint16_t          dev_addr,
                                  uint8_t           reg_addr,
                                  uint8_t           use_reg,
                                  uint8_t          *data,
                                  uint16_t          len,
                                  uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Read(&h->hw.hi2c, shifted, reg_addr,
                               I2C_MEMADD_SIZE_8BIT, data, len, timeout_ms);
    else
        ret = HAL_I2C_Master_Receive(&h->hw.hi2c, shifted, data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_iic_is_device_ready(drv_iic_handle_t *h,
                                          uint16_t          dev_addr,
                                          uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(&h->hw.hi2c,
                                                   (uint16_t)(dev_addr << 1),
                                                   3, timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── IT ops ───────────────────────────────────────────────────────────────── */

static int32_t stm32_iic_transmit_it(drv_iic_handle_t *h,
                                      uint16_t          dev_addr,
                                      uint8_t           reg_addr,
                                      uint8_t           use_reg,
                                      const uint8_t    *data,
                                      uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Write_IT(&h->hw.hi2c, shifted, reg_addr,
                                    I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len);
    else
        ret = HAL_I2C_Master_Transmit_IT(&h->hw.hi2c, shifted,
                                          (uint8_t *)data, len);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_iic_receive_it(drv_iic_handle_t *h,
                                     uint16_t          dev_addr,
                                     uint8_t           reg_addr,
                                     uint8_t           use_reg,
                                     uint8_t          *data,
                                     uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret;
    uint16_t shifted = (uint16_t)(dev_addr << 1);

    if (use_reg)
        ret = HAL_I2C_Mem_Read_IT(&h->hw.hi2c, shifted, reg_addr,
                                   I2C_MEMADD_SIZE_8BIT, data, len);
    else
        ret = HAL_I2C_Master_Receive_IT(&h->hw.hi2c, shifted, data, len);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── Static ops table ─────────────────────────────────────────────────────── */

static const drv_iic_hal_ops_t _stm32_iic_ops = {
    .hw_init        = stm32_iic_hw_init,
    .hw_deinit      = stm32_iic_hw_deinit,
    .transmit       = stm32_iic_transmit,
    .receive        = stm32_iic_receive,
    .is_device_ready = stm32_iic_is_device_ready,
    .transmit_it    = stm32_iic_transmit_it,
    .receive_it     = stm32_iic_receive_it,
};

/* ── Public API ───────────────────────────────────────────────────────────── */

const drv_iic_hal_ops_t *hal_iic_stm32_get_ops(void)
{
    return &_stm32_iic_ops;
}

void hal_iic_stm32_set_config(drv_iic_handle_t *h,
                              I2C_TypeDef      *instance,
                              uint32_t          addr_mode,
                              uint32_t          dual_addr)
{
    if (h == NULL || instance == NULL)
        return;

    I2C_HandleTypeDef *hi2c = &h->hw.hi2c;

    hi2c->Instance              = instance;
    hi2c->Init.AddressingMode   = addr_mode;
    hi2c->Init.DualAddressMode  = dual_addr;
    hi2c->Init.OwnAddress1      = 0;
    hi2c->Init.OwnAddress2      = 0;
    hi2c->Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    /* ClockSpeed will be set from h->clock_hz in hw_init */
}

/* ── IRQ dispatch ─────────────────────────────────────────────────────────── */

/*
 * hal_iic_stm32_ev_irq_handler — I2C event IRQ, dispatches completion events.
 *
 * Captures HAL state before and after calling HAL_I2C_EV_IRQHandler.  When
 * the state transitions to READY the transfer is done; the previous state
 * (BUSY_TX vs BUSY_RX) selects which IRQ ID to dispatch.  Both Master and
 * Mem transfer variants share the same BUSY_TX / BUSY_RX states so no
 * separate MemTxCplt / MemRxCplt path is needed.
 */
void hal_iic_stm32_ev_irq_handler(I2C_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_IIC; id++)
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

/*
 * hal_iic_stm32_er_irq_handler — I2C error IRQ.
 *
 * Calls HAL_I2C_ER_IRQHandler (which resets the state machine to READY), then
 * reads the error code and dispatches IRQ_ID_I2C_ERROR directly — no
 * HAL_I2C_ErrorCallback weak override needed.
 */
void hal_iic_stm32_er_irq_handler(I2C_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_IIC; id++)
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


#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_I2C_MODULE_ENABLED */
