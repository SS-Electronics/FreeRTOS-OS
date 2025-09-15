/*
 * INA230_IIC.h
 *
 *  Created on: Jan 11, 2024
 *      Author: subhajit-roy
 */

#ifndef BSW_DRIVERS_MODULE_INA230_IIC_INA230_IIC_H_
#define BSW_DRIVERS_MODULE_INA230_IIC_INA230_IIC_H_

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
#define INA230_CONFIG_REG			0x00
/*! Shunt voltage register. */
#define INA230_SHUNTV_REG			0x01
/*! Bus voltage register. */
#define INA230_VBUS_REG				0x02
/*! Power register. */
#define INA230_POWER_REG			0x03
/*! Current register. */
#define INA230_CURRENT_REG			0x04
/*! Calibration register. */
#define INA230_CALIB_REG			0x05
/*! Alert mask / enable register. */
#define INA230_MASKEN_REG			0x06
/*! Alert limit register. */
#define INA230_ALERT_LIMIT_REG		0x07
/*! Die identifier register. */
#define INA230_DIEID_REG			0xFF

/*! Average config bit position from the config register. */
#define INA230_AVG_BITPOS			9
/*! VBUS conversion time config bit position from the
 * config register. */
#define INA230_VBUS_CT_BITPOS		6
/*! VSHUNT conversion time config bit position from the
 * config register. */
#define INA230_VSH_CT_BITPOS		3

/*! 1 LSB in VBUS register = 1.25mV. */
#define INA230_VBUS_LSB				(1.25f)

typedef enum _ina230_avg_t
{
	INA230_AVG_1 = 0,
	INA230_AVG_4,
	INA230_AVG_16,
	INA230_AVG_64,
	INA230_AVG_128,
	INA230_AVG_256,
	INA230_AVG_512,
	INA230_AVG_1024,
	INA230_AVG_MAX
}ina230_avg_t;

typedef enum _ina230_conv_time_t
{
	INA230_CT_140US = 0,
	INA230_CT_204US,
	INA230_CT_332US,
	INA230_CT_588US,
	INA230_CT_1_1MS,
	INA230_CT_2_1MS,
	INA230_CT_4_1MS,
	INA230_CT_8_2MS,
	INA230_CT_MAX
}ina230_conv_time_t;



#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_INA230 == 1)

/* Proc iic mgmt core */
kernel_status_type		drv_ina230_iic_init(uint8_t device_address);
kernel_status_type		drv_ina230_iic_init(uint8_t device_address);
float					drv_ina230_iic_read_volt(uint8_t device_address);
float					drv_ina230_iic_read_current(uint8_t device_address);

kernel_status_type 		drv_ina230_iic_write_register(uint8_t device_address, uint8_t reg_address, uint8_t* data , uint8_t length);
kernel_status_type 		drv_ina230_iic_read_register(uint8_t device_address, uint8_t reg_address, uint8_t* buff, uint8_t length);
#else // If there is no inclusion give the warning
#warning "INA230 Module Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* BSW_DRIVERS_MODULE_INA230_IIC_INA230_IIC_H_ */
