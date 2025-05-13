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

#ifndef BSW_DRIVERS_DEVICE_COM_SPI_DRV_SPI_H_
#define BSW_DRIVERS_DEVICE_COM_SPI_DRV_SPI_H_

/*Configurations */
#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
/*Services*/
#include <bsw/services/os_shell_mgmt/os_shell_mgmt.h>
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
#include <bsw/drivers/device/gpio/drv_gpio.h>

/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

#if (INC_DRIVER_COMM_IIC == 1)

/* Entry Point of the OS and APP */
drv_spi_handle_type*	drv_spi_get_handle(void);
drv_status_type 		drv_spi_init(uint8_t device_id);
drv_status_type			drv_spi_transmit(spi_pdu_struct_type * tx_pdu);
drv_status_type			drv_spi_receive(spi_pdu_struct_type * rx_pdu);
drv_status_type			drv_spi_transfer(spi_pdu_struct_type * txn_pdu);


#else // If there is no inclusion give the warning
#warning "IIC Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* BSW_DRIVERS_DEVICE_COM_SPI_DRV_SPI_H_ */
