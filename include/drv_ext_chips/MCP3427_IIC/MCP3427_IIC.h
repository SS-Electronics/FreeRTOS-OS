/**
 * @file        MCP3427_IIC.h
 * @brief       MCP3427_IIC.h
 * @ingroup     drv_ext_chips
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      External Chips
 * @info        Drivers for off-MCU peripherals (sensors, GPIO expanders, EEPROM, DACs).
 * @dependency  Driver layer (I2C / SPI)
 *
 * @details
 * MCP3427_IIC.h
 *
 *  Created on: Feb15, 2024
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

#ifndef BSW_DRIVERS_MODULE_MCP3427_IIC_H_
#define BSW_DRIVERS_MODULE_MCP3427_IIC_H_

#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
#include <app/bsw_config/os_conf/os_config.h>
/* Kernel Include */
#include <bsw/kernel/kernel.h>
/* IPC */
#include <bsw/ipc/events/signal.h>
#include <bsw/ipc/mqueue/mqueue.h>
/* Respective drivers */
#include <bsw/drivers/device/com/iic/drv_iic.h>
#include <bsw/drivers/device/time/drv_time.h>
#include <bsw/services/os_shell_mgmt/os_shell_mgmt.h>

/* Device register map. */
/*! Device configuration register. */
#define CONFIG_REG          0xB8

#define  LSB_16BIT          0.0000625
#define  LSB_12BIT          0.001

enum _MCP3427_channels
{
	MCP3427_CH_1,
	MCP3427_CH_2
};

#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_MCP3427 == 1)

/* Proc iic mgmt core */
kernel_status_type		drv_mcp3427_iic_init(uint8_t device_address);
float					drv_mcp3427_iic_read_volt(uint8_t device_address, uint8_t channel);

kernel_status_type 		drv_mcp3427_iic_write_register(uint8_t device_address, uint8_t* data , uint8_t length);
kernel_status_type 		drv_mcp3427_iic_read_register(uint8_t device_address, uint8_t* buff, uint8_t length);
#else // If there is no inclusion give the warning
#warning "MCP3427 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* BSW_DRIVERS_MODULE_INA230_IIC_INA230_IIC_H_ */
