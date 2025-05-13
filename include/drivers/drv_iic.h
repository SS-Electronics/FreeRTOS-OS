/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS Basic Software
#
# RTOS Basic Software is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS Basic Software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#ifndef OS_DRIVERS_DEVICE_COM_IIC_DRV_IIC_H_
#define OS_DRIVERS_DEVICE_COM_IIC_DRV_IIC_H_
/*Configurations */
#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
/*Services*/
#include <bsw/services/diagnostics_mgmt/diagnostics_mgmt.h>
#include <bsw/utils/error_codes.h>
/* kernel include for event usage */
#include <bsw/kernel/kernel.h>
/*IPC include */
#include <bsw/ipc/events/signal.h>
#include <bsw/ipc/mqueue/mqueue.h>
/* Global data include  */
#include <bsw/ipc/global/global_var.h>
/* Low level driver include */
#include <bsw/drivers/device/driver_port.h>

/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_COMM_IIC == 1)

/* Entry Point of the OS and APP */
drv_iic_handle_type*	drv_iic_get_handle(void);
drv_status_type 		drv_iic_init(void);
drv_status_type			drv_iic_transmit(uint8_t dev_id, uint16_t device_address, uint8_t* data, uint16_t len);
drv_status_type			drv_iic_receive(uint8_t dev_id, uint16_t device_address, uint8_t* data, uint16_t len);
drv_status_type			drv_iic_device_ready(uint8_t dev_id, uint16_t device_address);

#else // If there is no inclusion give the warning
#warning "IIC Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/




#endif /* OS_DRIVERS_DEVICE_COM_IIC_DRV_IIC_H_ */
