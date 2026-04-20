/*
 * hal_iic_infineon.c — Infineon XMC series baremetal I2C ops (stubs)
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/infineon/hal_iic_infineon.h>

static int32_t infineon_iic_hw_init(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    /*
     * TODO: Configure USIC channel for I2C mode at h->clock_hz.
     *   1. Enable USIC clock via SCU.
     *   2. Set operating mode to I2C.
     *   3. Configure baud rate registers.
     *   4. Set address (master mode: own address not required).
     */
    h->initialized = 1;
    h->last_err    = OS_ERR_NONE;
    return OS_ERR_NONE;
}

static int32_t infineon_iic_hw_deinit(drv_iic_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    /* TODO: Disable USIC I2C channel. */
    h->initialized = 0;
    return OS_ERR_NONE;
}

static int32_t infineon_iic_transmit(drv_iic_handle_t *h,
                                      uint16_t          dev_addr,
                                      uint8_t           reg_addr,
                                      uint8_t           use_reg,
                                      const uint8_t    *data,
                                      uint16_t          len,
                                      uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    /*
     * TODO:
     *   1. Send START + device address (write bit).
     *   2. If use_reg, send reg_addr byte.
     *   3. Send data bytes.
     *   4. Send STOP.
     */
    (void)dev_addr; (void)reg_addr; (void)use_reg;
    (void)data; (void)len; (void)timeout_ms;
    return OS_ERR_NONE;
}

static int32_t infineon_iic_receive(drv_iic_handle_t *h,
                                     uint16_t          dev_addr,
                                     uint8_t           reg_addr,
                                     uint8_t           use_reg,
                                     uint8_t          *data,
                                     uint16_t          len,
                                     uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    /*
     * TODO:
     *   1. If use_reg: send START + addr(W) + reg_addr, repeated START + addr(R).
     *   2. Else: send START + addr(R).
     *   3. Read len bytes, NACK last byte.
     *   4. Send STOP.
     */
    (void)dev_addr; (void)reg_addr; (void)use_reg;
    (void)data; (void)len; (void)timeout_ms;
    return OS_ERR_NONE;
}

static int32_t infineon_iic_is_device_ready(drv_iic_handle_t *h,
                                              uint16_t          dev_addr,
                                              uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized)
        return OS_ERR_OP;

    /* TODO: Send START + addr(W), check for ACK, then STOP. */
    (void)dev_addr; (void)timeout_ms;
    return OS_ERR_NONE;
}

static const drv_iic_hal_ops_t _infineon_iic_ops = {
    .hw_init         = infineon_iic_hw_init,
    .hw_deinit       = infineon_iic_hw_deinit,
    .transmit        = infineon_iic_transmit,
    .receive         = infineon_iic_receive,
    .is_device_ready = infineon_iic_is_device_ready,
};

const drv_iic_hal_ops_t *hal_iic_infineon_get_ops(void)
{
    return &_infineon_iic_ops;
}

void hal_iic_infineon_set_config(drv_iic_handle_t *h,
                                 uint32_t          channel_base)
{
    if (h == NULL)
        return;

    h->hw.channel_base = channel_base;
    h->hw.clock_hz     = h->clock_hz;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON */
