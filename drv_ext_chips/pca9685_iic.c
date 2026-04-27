/**
 * @file    pca9685_iic.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   PCA9685 16-channel PWM controller driver
 *
 * @details
 * All I2C transfers go through iic_app (iic_sync_transmit / iic_sync_receive)
 * which serializes access through iic_mgmt_thread.
 *
 * PCA9685 internal oscillator: 25 MHz
 * PWM resolution: 12-bit (0 … 4095 ticks per cycle)
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <drv_ext_chips/pca9685_iic.h>
#include <drv_app/iic_app.h>
#include <drivers/drv_time.h>
#include <board/board_config.h>

#if (BOARD_IIC_COUNT > 0)

/* ── Private helpers ─────────────────────────────────────────────────────── */

/* Modify a single bit in a register (read-modify-write).
 * RESTART bit (7) is always written as 0 to avoid accidental restart. */
__SECTION_OS
static int32_t _set_bit(uint8_t bus_id, uint16_t dev_addr,
                         uint8_t reg, uint8_t bit_pos, uint8_t val)
{
    uint8_t  reg_val;
    int32_t  ret;

    ret = pca9685_read_reg(bus_id, dev_addr, reg, &reg_val, 1);
    if (ret != OS_ERR_NONE)
        return ret;

    reg_val &= (uint8_t)(~((1u << PCA9685_MODE1_RESTART) | (1u << bit_pos)));
    reg_val |= (uint8_t)((val & 1u) << bit_pos);

    return pca9685_write_reg(bus_id, dev_addr, reg, &reg_val, 1);
}

/* ── Public API ──────────────────────────────────────────────────────────── */
__SECTION_OS
int32_t pca9685_write_reg(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg, const uint8_t *data, uint8_t len)
{
    return iic_sync_transmit(bus_id, dev_addr, reg, 1,
                              data, (uint16_t)len, PCA9685_IIC_TIMEOUT_MS);
}

__SECTION_OS
int32_t pca9685_read_reg(uint8_t bus_id, uint16_t dev_addr,
                          uint8_t reg, uint8_t *data, uint8_t len)
{
    return iic_sync_receive(bus_id, dev_addr, reg, 1,
                             data, (uint16_t)len, PCA9685_IIC_TIMEOUT_MS);
}

__SECTION_OS
int32_t pca9685_sw_reset(uint8_t bus_id, uint16_t dev_addr)
{
    uint8_t val = 0x06U;
    return pca9685_write_reg(bus_id, dev_addr, PCA9685_REG_MODE1, &val, 1);
}

__SECTION_OS
int32_t pca9685_init(uint8_t bus_id, uint16_t dev_addr)
{
    int32_t ret;
    uint8_t val;

    /* Trigger restart to clear any previous state */
    val = (1u << PCA9685_MODE1_RESTART);
    ret = pca9685_write_reg(bus_id, dev_addr, PCA9685_REG_MODE1, &val, 1);
    if (ret != OS_ERR_NONE)
        return ret;

    drv_time_delay_ms(10U);

    /* Enable auto-increment so multi-byte channel writes work correctly */
    return _set_bit(bus_id, dev_addr,
                    PCA9685_REG_MODE1, PCA9685_MODE1_AI, 1);
}

__SECTION_OS
int32_t pca9685_set_pwm_freq(uint8_t bus_id, uint16_t dev_addr, uint32_t freq_hz)
{
    int32_t ret;
    uint8_t prescale;
    uint32_t denom;

    if (freq_hz == 0)
        return OS_ERR_OP;

    /* prescale = round(25_000_000 / (4096 * freq_hz)) - 1 */
    denom   = 4096UL * freq_hz;
    prescale = (uint8_t)((25000000UL + denom / 2U) / denom - 1U);

    if (prescale < PCA9685_PRESCALE_MIN)  prescale = PCA9685_PRESCALE_MIN;
    if (prescale > PCA9685_PRESCALE_MAX)  prescale = PCA9685_PRESCALE_MAX;

    /* PRE_SCALE is only writable while oscillator is in SLEEP mode */
    ret = _set_bit(bus_id, dev_addr, PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP, 1);
    if (ret != OS_ERR_NONE)
        return ret;

    drv_time_delay_ms(1U);

    ret = pca9685_write_reg(bus_id, dev_addr,
                             PCA9685_REG_PRE_SCALE, &prescale, 1);
    if (ret != OS_ERR_NONE)
        return ret;

    /* Wake up — clear SLEEP, then trigger RESTART to sync all channels */
    ret = _set_bit(bus_id, dev_addr, PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP, 0);
    if (ret != OS_ERR_NONE)
        return ret;

    drv_time_delay_ms(1U);

    return _set_bit(bus_id, dev_addr,
                    PCA9685_REG_MODE1, PCA9685_MODE1_RESTART, 1);
}

__SECTION_OS
int32_t pca9685_set_pwm(uint8_t bus_id, uint16_t dev_addr,
                         uint8_t channel, uint16_t on_tick, uint16_t off_tick)
{
    uint8_t  data[4];
    uint8_t  reg;

    if (channel >= PCA9685_NUM_CHANNELS)
        return OS_ERR_OP;

    reg = (uint8_t)(PCA9685_REG_LED0_ON_L + 4U * channel);

    data[0] = (uint8_t)(on_tick  & 0xFFU);
    data[1] = (uint8_t)((on_tick  >> 8) & 0x0FU);
    data[2] = (uint8_t)(off_tick & 0xFFU);
    data[3] = (uint8_t)((off_tick >> 8) & 0x0FU);

    return pca9685_write_reg(bus_id, dev_addr, reg, data, 4);
}

__SECTION_OS
int32_t pca9685_set_pin(uint8_t bus_id, uint16_t dev_addr,
                         uint8_t channel, uint8_t value, uint8_t invert)
{
    /* Use full-ON (on_tick bit 12 set) or full-OFF (off_tick bit 12 set)
     * to drive a channel as a digital output without jitter. */
    if (invert)
        value = (uint8_t)(value ? 0U : 1U);

    if (value)
        return pca9685_set_pwm(bus_id, dev_addr, channel, 0x1000U, 0x0000U);
    else
        return pca9685_set_pwm(bus_id, dev_addr, channel, 0x0000U, 0x1000U);
}

#endif /* BOARD_IIC_COUNT > 0 */
