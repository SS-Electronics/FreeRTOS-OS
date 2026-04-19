/*
 * MCP23017_IIC.h
 *
 *  Created on: Sep 27, 2023
 *      Author: subhajit-roy
 */

#ifndef BSW_DRIVERS_MODULE_MCP23017_IIC_MCP23017_IIC_H_
#define BSW_DRIVERS_MODULE_MCP23017_IIC_MCP23017_IIC_H_

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

#if (INC_DRIVER_MC23017 == 1)

#define PORT_A		(1)
#define	PORT_B		(2)

#define REG_IOCON	(0x05)
#define REG_IODIR_A	(0)
#define REG_IPOLA_A (0x02)
#define REG_DEFVALA	(0x06)
#define	REG_GPPUA	(0x0C)
#define REG_GPIOA	(0x12)
#define REG_OLATA	(0x14)

#ifdef __cplusplus
extern "C"{
#endif

/* Proc iic mgmt core */
kernel_status_type		drv_mcp23017_iic_init(uint8_t device_address);

kernel_status_type 		drv_mcp23017_iic_write(uint8_t device_address, uint8_t port , uint8_t pin, uint8_t state);
kernel_status_type 		drv_mcp23017_iic_write_register(uint8_t device_address,uint8_t reg_address, uint8_t* data , uint8_t length);
uint8_t 				drv_mcp23017_iic_read_register(uint8_t device_address,uint8_t reg_address);

#else // If there is no inclusion give the warning
#warning "MC23017 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* BSW_DRIVERS_MODULE_MCP23017_IIC_MCP23017_IIC_H_ */
