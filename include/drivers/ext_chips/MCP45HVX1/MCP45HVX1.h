/*
 * MCP45HVX1.h
 *
 *  Created on: Jan 29, 2024
 *      Author: subhajit-roy
 */

#ifndef BSW_DRIVERS_MODULE_MCP45HVX1_MCP45HVX1_H_
#define BSW_DRIVERS_MODULE_MCP45HVX1_MCP45HVX1_H_

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

#if (INC_DRIVER_MCP45HVX1 == 1)



/* REG addresses definition */

/* Device register map definitions. */
/*! \brief Volatile wiper value register. */
#define MCP45HV_REG_WIPER_VOL		0x00
/*! \brief Volatile terminal connection register. */
#define MCP45HV_REG_TCON_VOL		0x04

/* Device access command definitions. */
/*! \brief Write data command opcode. */
#define MCP45HV_CMD_WRITE			0x00
/*! \brief Increment command opcode. */
#define MCP45HV_CMD_INCR			0x01
/*! \brief Decrement command opcode. */
#define MCP45HV_CMD_DECR			0x02
/*! \brief Read data command opcode. */
#define MCP45HV_CMD_READ			0x03




#ifdef __cplusplus
extern "C" {
#endif

/* Proc iic mgmt core */
kernel_status_type		drv_mcp45hvx1_iic_init(uint8_t device_address);

kernel_status_type 		drv_mcp45hvx1_set_pot(uint8_t device_address, uint8_t pot_val);
kernel_status_type 		drv_mcp45hvx1_iic_write_register(uint8_t device_address,uint8_t reg_address, uint8_t* data , uint8_t length);
kernel_status_type      drv_mcp45hvx1_iic_read_register(uint8_t device_address, uint8_t reg_address, uint8_t* buff, uint8_t length);

#else // If there is no inclusion give the warning
#warning "MC23017 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/







#endif /* BSW_DRIVERS_MODULE_MCP45HVX1_MCP45HVX1_H_ */
