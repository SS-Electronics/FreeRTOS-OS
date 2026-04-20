/*
 * hal_iic_stm32.c — STM32 HAL I2C ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_I2C_MODULE_ENABLED)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_iic_stm32.h>

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────── */

static int32_t stm32_iic_hw_init(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    I2C_HandleTypeDef *hi2c = &h->hw.hi2c;

    /* Clock speed is owned by the generic handle */
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

    HAL_StatusTypeDef ret = HAL_I2C_DeInit(&h->hw.hi2c);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);
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
    {
        ret = HAL_I2C_Mem_Write(&h->hw.hi2c,
                                shifted,
                                reg_addr,
                                I2C_MEMADD_SIZE_8BIT,
                                (uint8_t *)data,
                                len,
                                timeout_ms);
    }
    else
    {
        ret = HAL_I2C_Master_Transmit(&h->hw.hi2c,
                                      shifted,
                                      (uint8_t *)data,
                                      len,
                                      timeout_ms);
    }

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
    {
        ret = HAL_I2C_Mem_Read(&h->hw.hi2c,
                               shifted,
                               reg_addr,
                               I2C_MEMADD_SIZE_8BIT,
                               data,
                               len,
                               timeout_ms);
    }
    else
    {
        ret = HAL_I2C_Master_Receive(&h->hw.hi2c,
                                     shifted,
                                     data,
                                     len,
                                     timeout_ms);
    }

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
                                                   3,
                                                   timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── Static ops table ─────────────────────────────────────────────────── */

static const drv_iic_hal_ops_t _stm32_iic_ops = {
    .hw_init        = stm32_iic_hw_init,
    .hw_deinit      = stm32_iic_hw_deinit,
    .transmit       = stm32_iic_transmit,
    .receive        = stm32_iic_receive,
    .is_device_ready = stm32_iic_is_device_ready,
};

/* ── Public API ───────────────────────────────────────────────────────── */

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

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_I2C_MODULE_ENABLED */
