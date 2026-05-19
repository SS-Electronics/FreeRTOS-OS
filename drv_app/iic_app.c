/**
 * @file        iic_app.c
 * @brief       iic_app.c — Application-level I2C sync / async operations
 * @ingroup     drv_app
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver App
 * @info        Application-facing helpers that wrap driver init for board-listed peripherals.
 * @dependency  Driver layer, board BSP
 *
 * @details
 * iic_app.c — Application-level I2C sync / async operations
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <drv_app/iic_app.h>
#include <services/iic_mgmt.h>
#include <def_err.h>

#if (BOARD_IIC_COUNT > 0)

/* ── Synchronous ──────────────────────────────────────────────────────── */

int32_t iic_sync_transmit(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           const uint8_t *data, uint16_t len,
                           uint32_t timeout_ms)
{
    return iic_mgmt_sync_transmit(bus_id, dev_addr, reg_addr, use_reg,
                                  data, len, timeout_ms);
}

int32_t iic_sync_receive(uint8_t bus_id, uint16_t dev_addr,
                          uint8_t reg_addr, uint8_t use_reg,
                          uint8_t *data, uint16_t len,
                          uint32_t timeout_ms)
{
    return iic_mgmt_sync_receive(bus_id, dev_addr, reg_addr, use_reg,
                                 data, len, timeout_ms);
}

int32_t iic_sync_probe(uint8_t bus_id, uint16_t dev_addr, uint32_t timeout_ms)
{
    return iic_mgmt_sync_probe(bus_id, dev_addr, timeout_ms);
}

/* ── Asynchronous ────────────────────────────────────────────────────── */

int32_t iic_async_transmit(uint8_t bus_id, uint16_t dev_addr,
                            uint8_t reg_addr, uint8_t use_reg,
                            const uint8_t *data, uint16_t len)
{
    return iic_mgmt_async_transmit(bus_id, dev_addr, reg_addr, use_reg,
                                   data, len);
}

int32_t iic_async_receive(uint8_t bus_id, uint16_t dev_addr,
                           uint8_t reg_addr, uint8_t use_reg,
                           uint8_t *data, uint16_t len)
{
    return iic_mgmt_async_receive(bus_id, dev_addr, reg_addr, use_reg,
                                  data, len);
}

#else /* BOARD_IIC_COUNT == 0 — provide no-op stubs */

int32_t iic_sync_transmit(uint8_t b, uint16_t d, uint8_t r, uint8_t u,
                           const uint8_t *p, uint16_t l, uint32_t t)
{ (void)b;(void)d;(void)r;(void)u;(void)p;(void)l;(void)t; return OS_ERR_OP; }

int32_t iic_sync_receive(uint8_t b, uint16_t d, uint8_t r, uint8_t u,
                          uint8_t *p, uint16_t l, uint32_t t)
{ (void)b;(void)d;(void)r;(void)u;(void)p;(void)l;(void)t; return OS_ERR_OP; }

int32_t iic_sync_probe(uint8_t b, uint16_t d, uint32_t t)
{ (void)b;(void)d;(void)t; return OS_ERR_OP; }

int32_t iic_async_transmit(uint8_t b, uint16_t d, uint8_t r, uint8_t u,
                            const uint8_t *p, uint16_t l)
{ (void)b;(void)d;(void)r;(void)u;(void)p;(void)l; return OS_ERR_OP; }

int32_t iic_async_receive(uint8_t b, uint16_t d, uint8_t r, uint8_t u,
                           uint8_t *p, uint16_t l)
{ (void)b;(void)d;(void)r;(void)u;(void)p;(void)l; return OS_ERR_OP; }

#endif /* BOARD_IIC_COUNT > 0 */
