/**
 * @file        DS3502_IIC.h
 * @brief       DS3502_IIC.h
 * @ingroup     drv_ext_chips
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      External Chips
 * @info        Drivers for off-MCU peripherals (sensors, GPIO expanders, EEPROM, DACs).
 * @dependency  Driver layer (I2C / SPI)
 *
 * @details
 * DS3502_IIC.h
 *
 *  Created on: Sep 28, 2023
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

#ifndef BSW_DRIVERS_MODULE_DS3202_IIC_DS3502_IIC_H_
#define BSW_DRIVERS_MODULE_DS3202_IIC_DS3502_IIC_H_

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



#define REG_CR		(0x02)
#define REG_CTRL	(0x00)



#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_DS3502 == 1)

/* Proc iic mgmt core */
kernel_status_type		drv_ds3502_iic_init(uint8_t device_address);

kernel_status_type		drv_ds3502_iic_write_pot(uint8_t device_address, uint8_t pot_val);

kernel_status_type 		drv_ds3502_iic_write_register(uint8_t device_address, uint8_t* data , uint8_t length);
uint8_t 				drv_ds3502_iic_read_register(uint8_t device_address, uint8_t* buff, uint8_t length);

#else // If there is no inclusion give the warning
#warning "DS3502 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* BSW_DRIVERS_MODULE_DS3202_IIC_DS3502_IIC_H_ */
