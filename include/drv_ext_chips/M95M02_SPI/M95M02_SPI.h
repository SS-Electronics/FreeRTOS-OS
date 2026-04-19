/*
 * M95M02_SPI.h
 *
 *  Created on: Sep 30, 2024
 *      Author: subhajitroy005
 */

#ifndef BSW_DRIVERS_MODULE_M95M02_SPI_M95M02_SPI_H_
#define BSW_DRIVERS_MODULE_M95M02_SPI_M95M02_SPI_H_

#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
#include <app/bsw_config/os_conf/os_config.h>
/* Kernel Include */
#include <bsw/kernel/kernel.h>
/* IPC */
#include <bsw/ipc/events/signal.h>
#include <bsw/ipc/mqueue/mqueue.h>
/* Respective drivers */
#include <bsw/drivers/device/com/spi/drv_spi.h>
#include <bsw/drivers/device/time/drv_time.h>
#include <bsw/drivers/device/driver_port.h>

#if (INC_DRIVER_M95M02 == 1)

#define M95M02_CMD_WREN			(0x06)  // Write Enable
#define M95M02_CMD_WRDI			(0x04)	// Write Disable
#define M95M02_CMD_RDSR			(0x05)	// Read status register
#define M95M02_CMD_WRSR			(0x01)	// Write ststus register
#define M95M02_CMD_READ			(0x03)  // Read form mem array
#define M95M02_CMD_WRITE		(0x02)	// write to mem array
#define M95M02_CMD_RDID			(0x83)	// Read identification page
#define M95M02_CMD_WRID			(0x82)	// Write Identification page
#define M95M02_CMD_RDLS			(0x83)	// Read identificatoion lock status
#define M95M02_CMD_LID			(0x82)	// Write Lock identification lock status


#ifdef __cplusplus
extern "C"{
#endif

/* Proc iic mgmt core */
kernel_status_type		drv_m92m02_spi_init(uint8_t device_address);

kernel_status_type		drv_m92m02_spi_get_status(uint8_t device_address, uint8_t * str_prt);

kernel_status_type 		drv_m92m02_spi_write_to_address(uint8_t device_address, uint32_t mem_addr ,
		                                                uint8_t  data, uint8_t length);
kernel_status_type 		drv_m92m02_spi_read_from_address(uint8_t device_address, uint32_t mem_addr ,
														 uint8_t * data, uint8_t length );



#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/

#else // If there is no inclusion give the warning
#warning "M95M02 Module Driver Not included!!"
#endif

#endif /* BSW_DRIVERS_MODULE_M95M02_SPI_M95M02_SPI_H_ */
