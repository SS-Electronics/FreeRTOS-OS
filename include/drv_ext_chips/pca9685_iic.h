/**
 * @file    pca9685_iic.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   PCA9685 16-channel PWM controller driver
 *
 * @details
 * Thin driver over the iic_app sync API.  Every function takes an explicit
 * bus_id and dev_addr so multiple PCA9685 devices on different I2C buses can
 * coexist without recompilation.
 *
 * Register map (PCA9685 datasheet, rev 4)
 * ──────────────────────────────────────────────────────────────────────────
 *  0x00  MODE1         — oscillator, restart, AI, sub-address enables
 *  0x01  MODE2         — output driver, OE polarity, change-on-ACK
 *  0x06  LED0_ON_L     — first channel (4 bytes per channel, 16 channels)
 *  0xFA  ALL_LED_ON_L  — write to all 16 channels simultaneously
 *  0xFE  PRE_SCALE     — PWM frequency prescaler (only writable in SLEEP)
 *
 * PWM frequency formula (25 MHz internal oscillator)
 *   prescale = round(25_000_000 / (4096 * freq_hz)) - 1    [3 … 255]
 *   freq_hz  → 1526 Hz max (prescale 3), 24 Hz min (prescale 255)
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_PCA9685_IIC_H_
#define FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_PCA9685_IIC_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>


#ifdef __cplusplus
extern "C" {
#endif

/* ── Default I2C timeout ─────────────────────────────────────────────────── */
#define PCA9685_IIC_TIMEOUT_MS      100U

/* ── Register addresses ──────────────────────────────────────────────────── */
#define PCA9685_REG_MODE1           0x00U
#define PCA9685_REG_MODE2           0x01U
#define PCA9685_REG_LED0_ON_L       0x06U
#define PCA9685_REG_ALL_LED_ON_L    0xFAU
#define PCA9685_REG_ALL_LED_OFF_L   0xFCU
#define PCA9685_REG_PRE_SCALE       0xFEU

/* ── MODE1 bit positions ─────────────────────────────────────────────────── */
#define PCA9685_MODE1_RESTART       7U  /**< Restart PWM channels            */
#define PCA9685_MODE1_AI            5U  /**< Auto-increment register pointer  */
#define PCA9685_MODE1_SLEEP         4U  /**< Low-power mode (oscillator off)  */

/* ── MODE2 bit positions ─────────────────────────────────────────────────── */
#define PCA9685_MODE2_OUTDRV        2U  /**< 1 = totem-pole, 0 = open-drain  */

/* ── PWM prescaler clamp ─────────────────────────────────────────────────── */
#define PCA9685_PRESCALE_MIN        3U   /**< ~1526 Hz */
#define PCA9685_PRESCALE_MAX        255U /**< ~24 Hz   */

/* ── Number of PWM channels ─────────────────────────────────────────────── */
#define PCA9685_NUM_CHANNELS        16U

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief  Initialize the PCA9685: software reset, enable auto-increment.
 *
 * @param  bus_id    I2C bus index (matches board_iic_desc_t dev_id).
 * @param  dev_addr  7-bit I2C device address (A0–A5 pins set bits 1–6).
 * @return OS_ERR_NONE on success, OS_ERR_OP on I2C failure.
 */
int32_t pca9685_init(uint8_t bus_id, uint16_t dev_addr);

/**
 * @brief  Set the PWM output frequency for all channels.
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @param  freq_hz   Desired frequency in Hz [24 … 1526].
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_set_pwm_freq(uint8_t bus_id, uint16_t dev_addr, uint32_t freq_hz);

/**
 * @brief  Set the ON and OFF counts for one PWM channel.
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @param  channel   Channel index [0 … 15].
 * @param  on_tick   Tick count at which the output goes HIGH [0 … 4095].
 * @param  off_tick  Tick count at which the output goes LOW  [0 … 4095].
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_set_pwm(uint8_t bus_id, uint16_t dev_addr,
                         uint8_t channel, uint16_t on_tick, uint16_t off_tick);

/**
 * @brief  Drive a channel as a digital output (fully ON or fully OFF).
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @param  channel   Channel index [0 … 15].
 * @param  value     0 = off, non-zero = on.
 * @param  invert    1 = invert the logic level.
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_set_pin(uint8_t bus_id, uint16_t dev_addr,
                         uint8_t channel, uint8_t value, uint8_t invert);

/**
 * @brief  Trigger a software reset via the MODE1 RESTART bit.
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_sw_reset(uint8_t bus_id, uint16_t dev_addr);

/**
 * @brief  Write one or more consecutive registers.
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @param  reg       Starting register address.
 * @param  data      Pointer to bytes to write.
 * @param  len       Number of bytes.
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_write_reg(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg, const uint8_t *data, uint8_t len);

/**
 * @brief  Read one or more consecutive registers.
 *
 * @param  bus_id    I2C bus index.
 * @param  dev_addr  7-bit device address.
 * @param  reg       Starting register address.
 * @param  data      Buffer to receive bytes.
 * @param  len       Number of bytes to read.
 * @return OS_ERR_NONE on success.
 */
int32_t pca9685_read_reg(uint8_t bus_id, uint16_t dev_addr,
                          uint8_t reg, uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_PCA9685_IIC_H_ */
