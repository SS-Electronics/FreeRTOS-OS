/*
File:        drv_iic.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:      drivers
Info:        Generic I2C driver implementation with HAL abstraction             
Dependency:  drivers/com/drv_iic.h, board/mcu_config.h, conf_os.h, def_err.h

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file    drv_iic.c
 * @brief   Generic I2C driver with HAL abstraction
 *
 * @details
 * This file implements a vendor-agnostic I2C driver for the FreeRTOS-OS
 * project. All hardware-specific operations are delegated to a HAL
 * operations table (drv_iic_hal_ops_t), which is registered at runtime
 * by the management layer (iic_mgmt.c).
 *
 * The driver provides:
 * - Device registration and initialization
 * - Blocking transmit and receive APIs
 * - Device presence probing
 *
 * Each I2C instance is represented by a drv_iic_handle_t and mapped
 * to a hardware peripheral via the HAL layer.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <drivers/drv_iic.h>
#include <board/board_config.h>   /* pulls board_device_ids.h → BOARD_IIC_COUNT */
#include <board/board_handles.h>

#if (BOARD_IIC_COUNT > 0)

/* ── Handle storage ───────────────────────────────────────────────────── */

/**
 * @brief Internal I2C handle storage
 */
__SECTION_OS_DATA __USED
static drv_iic_handle_t _iic_handles[BOARD_IIC_COUNT];

/* ── Registration API ─────────────────────────────────────────────────── */

/**
 * @brief Register an I2C device and initialize hardware
 *
 * @param dev_id      I2C bus index
 * @param ops         HAL operations table
 * @param clock_hz    Bus clock frequency
 * @param timeout_ms  Transfer timeout
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (initialization phase)
 */
__SECTION_OS __USED
int32_t drv_iic_register(uint8_t dev_id,
                         const drv_iic_hal_ops_t *ops,
                         uint32_t clock_hz,
                         uint32_t timeout_ms)
{
    if (dev_id >= BOARD_IIC_COUNT || ops == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->clock_hz    = clock_hz;
    h->timeout_ms  = timeout_ms;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    return h->ops->hw_init(h);
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

/**
 * @brief Get I2C handle
 *
 * @param dev_id I2C bus index
 * @return Pointer to handle or NULL
 *
 * @context Task/ISR
 */
__SECTION_OS __USED
drv_iic_handle_t *drv_iic_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_IIC_COUNT)
        return NULL;
    return &_iic_handles[dev_id];
}

/* ── Public driver API ────────────────────────────────────────────────── */

/**
 * @brief Write data to an I2C device register
 *
 * @param dev_id    I2C bus index
 * @param dev_addr  7-bit device address (not shifted)
 * @param reg_addr  Register address
 * @param data      Pointer to data buffer
 * @param len       Number of bytes to write
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (blocking call)
 */
__SECTION_OS __USED
int32_t drv_iic_transmit(uint8_t dev_id, uint16_t dev_addr,
                         uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_IIC_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->transmit(h, dev_addr, reg_addr, 1, data, len, h->timeout_ms);
}

/**
 * @brief Read data from an I2C device register
 *
 * @param dev_id    I2C bus index
 * @param dev_addr  7-bit device address
 * @param reg_addr  Register address
 * @param data      Buffer to store received data
 * @param len       Number of bytes to read
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @context Task (blocking call)
 */
__SECTION_OS __USED
int32_t drv_iic_receive(uint8_t dev_id, uint16_t dev_addr,
                        uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev_id >= BOARD_IIC_COUNT || data == NULL)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->receive(h, dev_addr, reg_addr, 1, data, len, h->timeout_ms);
}

/**
 * @brief Check if an I2C device is ready (ACK response)
 *
 * @param dev_id    I2C bus index
 * @param dev_addr  7-bit device address
 *
 * @return OS_ERR_NONE if device responds, OS_ERR_OP otherwise
 *
 * @context Task
 */
__SECTION_OS __USED
int32_t drv_iic_device_ready(uint8_t dev_id, uint16_t dev_addr)
{
    if (dev_id >= BOARD_IIC_COUNT)
        return OS_ERR_OP;

    drv_iic_handle_t *h = &_iic_handles[dev_id];

    if (!h->initialized || h->ops == NULL)
        return OS_ERR_OP;

    return h->ops->is_device_ready(h, dev_addr, h->timeout_ms);
}

#endif /* BOARD_IIC_COUNT > 0 */