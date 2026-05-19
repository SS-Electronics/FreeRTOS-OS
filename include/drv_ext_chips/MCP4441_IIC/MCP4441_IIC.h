/**
 * @file        MCP4441_IIC.h
 * @brief       MCP4441_IIC.h
 * @ingroup     drv_ext_chips
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      External Chips
 * @info        Drivers for off-MCU peripherals (sensors, GPIO expanders, EEPROM, DACs).
 * @dependency  Driver layer (I2C / SPI)
 *
 * @details
 * MCP4441_IIC.h
 *
 *  Created on: Feb 19, 2024
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

#ifndef BSW_DRIVERS_MODULE_MCP4441_IIC_MCP4441_IIC_H_
#define BSW_DRIVERS_MODULE_MCP4441_IIC_MCP4441_IIC_H_

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

#define TCON_REG_0 			( (0x04<<4) | 1)
#define TCON_REG_1		 	( (0x0A<<4) | 1)
#define WIPER_REG_0 		( (0x00<<4) | 1)
#define WIPER_REG_1 		( (0x01<<4) | 1)
#define WIPER_REG_2 		( (0x06<<4) | 1)
#define WIPER_REG_3 		( (0x07<<4) | 1)

enum wiper{
	WIPER_0 = 0,
	WIPER_1,
	WIPER_2,
	WIPER_3
};

#if (INC_DRIVER_MCP4441 == 1)

/* Proc iic mgmt core */
kernel_status_type		drv_mcp4441_iic_init(uint8_t device_address);
kernel_status_type 		drv_mcp4441_iic_write_pot(uint8_t device_address, uint8_t w_num, uint8_t pot_val);
kernel_status_type 		drv_mcp4441_iic_write_register(uint8_t device_address,uint8_t reg_address, uint8_t* data , uint8_t length);
kernel_status_type		drv_mcp4441_iic_read_register(uint8_t device_address,uint8_t reg_address);
#else // If there is no inclusion give the warning
#warning "MCP4441 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/





#endif /* BSW_DRIVERS_MODULE_MCP4441_IIC_MCP4441_IIC_H_ */
