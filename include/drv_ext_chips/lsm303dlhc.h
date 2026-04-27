/**
 * @file    lsm303dlhc.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   LSM303DLHC 3-axis accelerometer + magnetometer driver
 *
 * @details
 * The LSM303DLHC exposes two independent I2C slave addresses on the same bus:
 *   - Accelerometer: 0x19  (SA0 pin tied HIGH on DLHC variant)
 *   - Magnetometer:  0x1E
 *
 * Default configuration applied by lsm303dlhc_acc_init() / lsm303dlhc_mag_init():
 *   ACC  — ±2 g, 100 Hz ODR, high-resolution (12-bit)
 *   MAG  — ±1.3 gauss, 15 Hz ODR, continuous measurement
 *
 * Raw data format (accelerometer, HR mode)
 * ─────────────────────────────────────────
 *   The DLHC stores 12-bit left-justified values in a 16-bit pair:
 *     OUT_X_H_A[7:0] : OUT_X_L_A[7:0]  →  16-bit word (shift right 4 for mg)
 *   Sensitivity: 1 mg / LSB  (±2 g HR) after the >>4 shift.
 *   To read 6 bytes in one transfer, set bit 7 of the register address
 *   to enable address auto-increment (LSM303DLHC datasheet, §5.1.1).
 *
 * Magnetometer note
 * ─────────────────
 *   MAG output order: X_H, X_L, Z_H, Z_L, Y_H, Y_L  (Z before Y — datasheet §8.1)
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_LSM303DLHC_H_
#define FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_LSM303DLHC_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── I2C addresses (7-bit) ───────────────────────────────────────────────── */
#define LSM303DLHC_ACC_ADDR         0x19U
#define LSM303DLHC_MAG_ADDR         0x1EU

/* ── I2C timeout ─────────────────────────────────────────────────────────── */
#define LSM303DLHC_IIC_TIMEOUT_MS   50U

/* ── Accelerometer registers ─────────────────────────────────────────────── */
#define LSM303_ACC_CTRL_REG1        0x20U  /**< ODR, power, axis enable        */
#define LSM303_ACC_CTRL_REG2        0x21U  /**< HPF config                     */
#define LSM303_ACC_CTRL_REG3        0x22U  /**< INT1 source select             */
#define LSM303_ACC_CTRL_REG4        0x23U  /**< Full-scale, HR, BDU            */
#define LSM303_ACC_CTRL_REG5        0x24U  /**< FIFO enable, latch INT         */
#define LSM303_ACC_STATUS_REG       0x27U  /**< Data-ready flags               */
#define LSM303_ACC_OUT_X_L          0x28U  /**< X low byte (use 0xA8 for burst)*/
#define LSM303_ACC_OUT_X_H          0x29U
#define LSM303_ACC_OUT_Y_L          0x2AU
#define LSM303_ACC_OUT_Y_H          0x2BU
#define LSM303_ACC_OUT_Z_L          0x2CU
#define LSM303_ACC_OUT_Z_H          0x2DU
#define LSM303_ACC_WHO_AM_I         0x0FU  /**< Should return 0x33             */

/* Auto-increment sub-address for burst reads (bit 7 = 1) */
#define LSM303_ACC_OUT_BURST        (0x80U | LSM303_ACC_OUT_X_L)  /* 0xA8 */

/* ── ACC CTRL_REG1_A bit fields ──────────────────────────────────────────── */
/* ODR[3:0] — bits [7:4] */
#define LSM303_ACC_ODR_PWRDN        0x00U  /**< Power-down                     */
#define LSM303_ACC_ODR_1HZ          0x10U  /**< 1 Hz                           */
#define LSM303_ACC_ODR_10HZ         0x20U  /**< 10 Hz                          */
#define LSM303_ACC_ODR_25HZ         0x30U  /**< 25 Hz                          */
#define LSM303_ACC_ODR_50HZ         0x40U  /**< 50 Hz                          */
#define LSM303_ACC_ODR_100HZ        0x50U  /**< 100 Hz  (default)              */
#define LSM303_ACC_ODR_200HZ        0x60U  /**< 200 Hz                         */
#define LSM303_ACC_ODR_400HZ        0x70U  /**< 400 Hz                         */
#define LSM303_ACC_AXES_EN          0x07U  /**< X | Y | Z enable               */

/* ── ACC CTRL_REG4_A bit fields ──────────────────────────────────────────── */
#define LSM303_ACC_FS_2G            0x00U  /**< ±2 g full scale  (default)     */
#define LSM303_ACC_FS_4G            0x10U  /**< ±4 g                           */
#define LSM303_ACC_FS_8G            0x20U  /**< ±8 g                           */
#define LSM303_ACC_FS_16G           0x30U  /**< ±16 g                          */
#define LSM303_ACC_HR               0x08U  /**< High-resolution (12-bit)       */

/* ── ACC sensitivity (mg/LSB after >>4, HR mode) ────────────────────────── */
#define LSM303_ACC_SENS_2G_HR       1      /**< 1 mg/LSB  ±2 g  HR            */
#define LSM303_ACC_SENS_4G_HR       2      /**< 2 mg/LSB  ±4 g  HR            */
#define LSM303_ACC_SENS_8G_HR       4      /**< 4 mg/LSB  ±8 g  HR            */
#define LSM303_ACC_SENS_16G_HR      12     /**< 12 mg/LSB ±16 g HR            */

/* ── Magnetometer registers ──────────────────────────────────────────────── */
#define LSM303_MAG_CRA_REG          0x00U  /**< Temp enable, output data rate  */
#define LSM303_MAG_CRB_REG          0x01U  /**< Gain                           */
#define LSM303_MAG_MR_REG           0x02U  /**< Mode register                  */
#define LSM303_MAG_OUT_X_H          0x03U  /**< X high byte                    */
#define LSM303_MAG_OUT_X_L          0x04U  /**< X low byte                     */
#define LSM303_MAG_OUT_Z_H          0x05U  /**< Z high byte  (Z before Y!)     */
#define LSM303_MAG_OUT_Z_L          0x06U
#define LSM303_MAG_OUT_Y_H          0x07U  /**< Y high byte                    */
#define LSM303_MAG_OUT_Y_L          0x08U
#define LSM303_MAG_SR_REG           0x09U  /**< Status (DRDY flag)             */
#define LSM303_MAG_IRA_REG          0x0AU  /**< ID register A (0x48)           */

/* ── MAG CRA_REG_M: output data rate DO[2:0] = bits [4:2] ───────────────── */
#define LSM303_MAG_ODR_0_75HZ       0x00U
#define LSM303_MAG_ODR_1_5HZ        0x04U
#define LSM303_MAG_ODR_3HZ          0x08U
#define LSM303_MAG_ODR_7_5HZ        0x0CU
#define LSM303_MAG_ODR_15HZ         0x10U  /**< 15 Hz  (default)               */
#define LSM303_MAG_ODR_30HZ         0x14U
#define LSM303_MAG_ODR_75HZ         0x18U

/* ── MAG CRB_REG_M: gain GN[2:0] = bits [7:5] ───────────────────────────── */
#define LSM303_MAG_GAIN_1_3G        0x20U  /**< ±1.3 gauss  (default)         */
#define LSM303_MAG_GAIN_1_9G        0x40U  /**< ±1.9 gauss                     */
#define LSM303_MAG_GAIN_2_5G        0x60U  /**< ±2.5 gauss                     */
#define LSM303_MAG_GAIN_4_0G        0x80U  /**< ±4.0 gauss                     */

/* ── MAG MR_REG_M: measurement mode MD[1:0] = bits [1:0] ───────────────── */
#define LSM303_MAG_MODE_CONTINUOUS  0x00U  /**< Continuous (default)           */
#define LSM303_MAG_MODE_SINGLE      0x01U
#define LSM303_MAG_MODE_IDLE        0x02U

/* ── MAG sensitivity (milligauss/LSB, ±1.3 gauss gain) ──────────────────── */
#define LSM303_MAG_SENS_XY_1_3G     909    /**< XY: ~0.909 mG/LSB (×1000)     */
#define LSM303_MAG_SENS_Z_1_3G      1020   /**< Z:  ~1.020 mG/LSB (×1000)     */

/* ── Data structures ─────────────────────────────────────────────────────── */

typedef struct {
    int16_t x;  /**< Raw 16-bit ACC output (left-justified; >>4 for HR value) */
    int16_t y;
    int16_t z;
} lsm303dlhc_acc_raw_t;

typedef struct {
    int16_t x;  /**< Raw 16-bit MAG output                                    */
    int16_t y;
    int16_t z;
} lsm303dlhc_mag_raw_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief  Initialize the accelerometer.
 *         Config: 100 Hz ODR, ±2 g, high-resolution, all axes enabled.
 *
 * @param  bus_id   I2C bus index.
 * @return OS_ERR_NONE on success.
 */
int32_t lsm303dlhc_acc_init(uint8_t bus_id);

/**
 * @brief  Initialize the magnetometer.
 *         Config: 15 Hz ODR, ±1.3 gauss gain, continuous mode.
 *
 * @param  bus_id   I2C bus index.
 * @return OS_ERR_NONE on success.
 */
int32_t lsm303dlhc_mag_init(uint8_t bus_id);

/**
 * @brief  Read raw 16-bit accelerometer data (all 3 axes, one burst transfer).
 *
 * @param  bus_id   I2C bus index.
 * @param  out      Pointer to struct receiving X, Y, Z raw values.
 * @return OS_ERR_NONE on success.
 */
int32_t lsm303dlhc_acc_read(uint8_t bus_id, lsm303dlhc_acc_raw_t *out);

/**
 * @brief  Read raw 16-bit magnetometer data (all 3 axes, one burst transfer).
 *         Note: hardware output order is X, Z, Y — this function reorders to X, Y, Z.
 *
 * @param  bus_id   I2C bus index.
 * @param  out      Pointer to struct receiving X, Y, Z raw values.
 * @return OS_ERR_NONE on success.
 */
int32_t lsm303dlhc_mag_read(uint8_t bus_id, lsm303dlhc_mag_raw_t *out);

/**
 * @brief  Write one register on the accelerometer sub-device.
 */
int32_t lsm303dlhc_acc_write_reg(uint8_t bus_id, uint8_t reg,
                                   uint8_t val);

/**
 * @brief  Read one register from the accelerometer sub-device.
 */
int32_t lsm303dlhc_acc_read_reg(uint8_t bus_id, uint8_t reg,
                                  uint8_t *val);

/**
 * @brief  Write one register on the magnetometer sub-device.
 */
int32_t lsm303dlhc_mag_write_reg(uint8_t bus_id, uint8_t reg,
                                   uint8_t val);

/**
 * @brief  Read one register from the magnetometer sub-device.
 */
int32_t lsm303dlhc_mag_read_reg(uint8_t bus_id, uint8_t reg,
                                  uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_DRV_EXT_CHIPS_LSM303DLHC_H_ */
