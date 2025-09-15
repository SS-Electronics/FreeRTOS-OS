/*
 * MCP3427_IIC.h
 *
 *  Created on: Feb15, 2024
 *      Author: koripalli yaswanth
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
