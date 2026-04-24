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

#ifndef OS_DRIVERS_DEVICE_COM_CAN_DRV_CAN_H_
#define OS_DRIVERS_DEVICE_COM_CAN_DRV_CAN_H_

/*Configurations */
#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
/* kernel include for event usage */
#include <bsw/kernel/kernel.h>
#include <bsw/kernel/COMM_STACK/canopen-stack/src/core/co_core.h>
#include <bsw/services/os_shell_mgmt/os_shell_mgmt.h>
#include <bsw/services/diagnostics_mgmt/diagnostics_mgmt.h>
#include <bsw/utils/error_codes.h>
/*IPC include */
#include <bsw/ipc/events/signal.h>
#include <bsw/ipc/mqueue/mqueue.h>
/* Global data include  */
#include <bsw/ipc/global/global_var.h>
/* Low level driver include */
#include <bsw/drivers/device/driver_port.h>
#include <app/bsw_config/os_conf/can_network_config.h>
/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

#if(PERIPHERAL_CAN_1_EN == 1)
extern const CO_IF_CAN_DRV co_drv_can_1;
#endif

#if(PERIPHERAL_CAN_2_EN == 1)
extern const CO_IF_CAN_DRV co_drv_can_2;
#endif

#if(PERIPHERAL_CAN_3_EN == 1)
extern const CO_IF_CAN_DRV co_drv_can_3;
#endif

#if (INC_DRIVER_COMM_CAN == 1)

drv_can_handle_type* 	drv_can_get_handle(void);
drv_status_type 		drv_can_init(uint8_t device_id);
drv_status_type 		drv_can_transmit(uint8_t dev_id, can_pdu_struct_type * tx_pdu_ref);
drv_status_type 		drv_can_get_tx_buffer_pending_status(uint8_t dev_id);


#else // If there is no inclusion give the warning
#warning "CAN Driver Not included!!"
#endif

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* OS_DRIVERS_DEVICE_COM_CAN_DRV_CAN_H_ */
