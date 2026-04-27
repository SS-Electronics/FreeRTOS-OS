/**
 * @file    lsm303dlhc.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   LSM303DLHC accelerometer + magnetometer driver
 *
 * @details
 * All I2C transfers go through iic_app (iic_sync_transmit / iic_sync_receive).
 *
 * Accelerometer burst read: sub-address 0xA8 (0x80 | OUT_X_L_A) enables
 * address auto-increment so all 6 bytes are fetched in one transfer.
 *
 * Magnetometer: auto-increment is always active; read starts at OUT_X_H_M.
 * Hardware register order is X, Z, Y — reordered to X, Y, Z in the output struct.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <drv_ext_chips/lsm303dlhc.h>
#include <drv_app/iic_app.h>
#include <board/board_config.h>

#if (BOARD_IIC_COUNT > 0)

/* ── Low-level register access ───────────────────────────────────────────── */

int32_t lsm303dlhc_acc_write_reg(uint8_t bus_id, uint8_t reg, uint8_t val)
{
    return iic_sync_transmit(bus_id, LSM303DLHC_ACC_ADDR, reg, 1,
                              &val, 1, LSM303DLHC_IIC_TIMEOUT_MS);
}

int32_t lsm303dlhc_acc_read_reg(uint8_t bus_id, uint8_t reg, uint8_t *val)
{
    return iic_sync_receive(bus_id, LSM303DLHC_ACC_ADDR, reg, 1,
                             val, 1, LSM303DLHC_IIC_TIMEOUT_MS);
}

int32_t lsm303dlhc_mag_write_reg(uint8_t bus_id, uint8_t reg, uint8_t val)
{
    return iic_sync_transmit(bus_id, LSM303DLHC_MAG_ADDR, reg, 1,
                              &val, 1, LSM303DLHC_IIC_TIMEOUT_MS);
}

int32_t lsm303dlhc_mag_read_reg(uint8_t bus_id, uint8_t reg, uint8_t *val)
{
    return iic_sync_receive(bus_id, LSM303DLHC_MAG_ADDR, reg, 1,
                             val, 1, LSM303DLHC_IIC_TIMEOUT_MS);
}

/* ── Init ────────────────────────────────────────────────────────────────── */

int32_t lsm303dlhc_acc_init(uint8_t bus_id)
{
    int32_t ret;

    /* CTRL_REG1_A: 100 Hz ODR, all axes enabled
     *   ODR = 0101b (100 Hz) → bits [7:4] = 0x50
     *   Zen | Yen | Xen     → bits [2:0] = 0x07
     *   Result: 0x57 */
    ret = lsm303dlhc_acc_write_reg(bus_id, LSM303_ACC_CTRL_REG1,
                                    LSM303_ACC_ODR_100HZ | LSM303_ACC_AXES_EN);
    if (ret != OS_ERR_NONE)
        return ret;

    /* CTRL_REG4_A: ±2 g, high-resolution, LSB @ lower address
     *   FS = 00b (±2 g) → bits [5:4] = 0x00
     *   HR = 1          → bit  [3]   = 0x08
     *   Result: 0x08 */
    return lsm303dlhc_acc_write_reg(bus_id, LSM303_ACC_CTRL_REG4,
                                     LSM303_ACC_FS_2G | LSM303_ACC_HR);
}

int32_t lsm303dlhc_mag_init(uint8_t bus_id)
{
    int32_t ret;

    /* CRA_REG_M: 15 Hz output data rate
     *   DO[2:0] = 100b → bits [4:2] = 0x10 */
    ret = lsm303dlhc_mag_write_reg(bus_id, LSM303_MAG_CRA_REG,
                                    LSM303_MAG_ODR_15HZ);
    if (ret != OS_ERR_NONE)
        return ret;

    /* CRB_REG_M: ±1.3 gauss gain
     *   GN[2:0] = 001b → bits [7:5] = 0x20 */
    ret = lsm303dlhc_mag_write_reg(bus_id, LSM303_MAG_CRB_REG,
                                    LSM303_MAG_GAIN_1_3G);
    if (ret != OS_ERR_NONE)
        return ret;

    /* MR_REG_M: continuous measurement mode */
    return lsm303dlhc_mag_write_reg(bus_id, LSM303_MAG_MR_REG,
                                     LSM303_MAG_MODE_CONTINUOUS);
}

/* ── Data reads ──────────────────────────────────────────────────────────── */

int32_t lsm303dlhc_acc_read(uint8_t bus_id, lsm303dlhc_acc_raw_t *out)
{
    uint8_t buf[6];
    int32_t ret;

    if (out == NULL)
        return OS_ERR_OP;

    /* Burst read 6 bytes starting at OUT_X_L_A.
     * Bit 7 of sub-address must be set to enable address auto-increment. */
    ret = iic_sync_receive(bus_id, LSM303DLHC_ACC_ADDR,
                            LSM303_ACC_OUT_BURST, 1,
                            buf, 6, LSM303DLHC_IIC_TIMEOUT_MS);
    if (ret != OS_ERR_NONE)
        return ret;

    out->x = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    out->y = (int16_t)((uint16_t)buf[3] << 8 | buf[2]);
    out->z = (int16_t)((uint16_t)buf[5] << 8 | buf[4]);

    return OS_ERR_NONE;
}

int32_t lsm303dlhc_mag_read(uint8_t bus_id, lsm303dlhc_mag_raw_t *out)
{
    uint8_t buf[6];
    int32_t ret;

    if (out == NULL)
        return OS_ERR_OP;

    /* Burst read 6 bytes starting at OUT_X_H_M.
     * Hardware order: X_H, X_L, Z_H, Z_L, Y_H, Y_L */
    ret = iic_sync_receive(bus_id, LSM303DLHC_MAG_ADDR,
                            LSM303_MAG_OUT_X_H, 1,
                            buf, 6, LSM303DLHC_IIC_TIMEOUT_MS);
    if (ret != OS_ERR_NONE)
        return ret;

    out->x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    out->z = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);  /* Z before Y in HW */
    out->y = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);

    return OS_ERR_NONE;
}

#endif /* BOARD_IIC_COUNT > 0 */
