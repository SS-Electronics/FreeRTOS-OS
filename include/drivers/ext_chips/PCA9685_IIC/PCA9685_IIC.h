/*
 * pca9685_iic.h
 *
 *  Created on: Sep 26, 2023
 *      Author: subhajit-roy
 */

#ifndef BSW_DRIVERS_MODULE_PCA9685_IIC_PCA9685_IIC_H_
#define BSW_DRIVERS_MODULE_PCA9685_IIC_PCA9685_IIC_H_

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



/* PCC 9685 related macros */
#define REG_MODE_1_RESTART_BIT_POS	(7)
#define REG_MODE_1_AI_BIT_POS		(5)
#define REG_MODE_1_SLEEP_BIT_POS	(4)
#define	REH_MODE_2_OUTDRV_BIT_POS	(2)

#define	REG_MODE_0					(0)
#define REG_MODE_1					(1)
#define REG_SUB_ADDR_3				(0x04)
#define REG_LED0_ON_L 				(0x06)
#define REG_LED8_ON_L 				(8)
#define REG_PRESCALE				(0xFE)
#define REG_ALL_LED_ON				(0xFA)
#define REG_ALL_LED_OFF				(0xFC)


/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_PCA9685 == 1)

/* Proc iic mgmt core */
kernel_status_type 		drv_pca9685_iic_init(uint8_t dev_address);
kernel_status_type 		drv_pca9685_iic_set_pwm_freq(uint8_t dev_address, uint32_t frequency);
kernel_status_type 		drv_pca9685_iic_sw_reset(uint8_t dev_address);
kernel_status_type 		drv_pca9685_iic_set_bit(uint8_t dev_address, uint8_t reg_address, uint8_t bit_pos, uint8_t val);
kernel_status_type 		drv_pca9685_iic_set_pwm(uint8_t dev_address, uint8_t channel, uint16_t on_time, uint16_t off_time);
kernel_status_type      drv_pca9685_iic_set_pin(uint8_t dev_address, uint8_t channel, uint8_t value , uint8_t invert);

kernel_status_type 		drv_pca9685_iic_write_register(uint8_t dev_address,
												  	  uint8_t reg_address,
													  uint8_t* data ,
													  uint8_t length);
uint8_t 				drv_pca9685_iic_read_register(uint8_t dev_address, uint8_t reg_address);

#else // If there is no inclusion give the warning
#warning "PCA9685 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* BSW_DRIVERS_MODULE_PCA9685_IIC_PCA9685_IIC_H_ */
