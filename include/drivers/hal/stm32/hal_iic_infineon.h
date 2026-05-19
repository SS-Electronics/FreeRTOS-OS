/**
 * @file        hal_iic_infineon.h
 * @brief       hal_iic_infineon.h — Infineon XMC series baremetal I2C ops declaration
 * @ingroup     drivers
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver Layer
 * @info        Vendor-agnostic driver vtables; concrete backends live under drivers/hal/<vendor>/.
 * @dependency  HAL backend (selected by CONFIG_DEVICE_VARIANT)
 *
 * @details
 * hal_iic_infineon.h — Infineon XMC series baremetal I2C ops declaration
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

#ifndef DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_
#define DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

#include <drivers/drv_iic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the Infineon I2C HAL ops table. */
const drv_iic_hal_ops_t *hal_iic_infineon_get_ops(void);

/**
 * @brief  Set the hardware context for an Infineon I2C channel.
 *
 * @param  h             Generic I2C handle.
 * @param  channel_base  USIC I2C channel base address.
 */
void hal_iic_infineon_set_config(drv_iic_handle_t *h,
                                 uint32_t          channel_base);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON */

#endif /* DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_ */
