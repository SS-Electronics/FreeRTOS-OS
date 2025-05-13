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

#ifndef OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_
#define OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_


#include <bsw/drivers/device/driver_port.h>
/* for serial print operation */
#include <bsw/drivers/device/com/uart/drv_uart.h>
#include <app/bsw_config/arch_conf/board/board.h>

#define ITM_PRINT_BUFF_LENGTH	50

/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

drv_wdg_handle_type* 	drv_wdg_get_handle(void);
void 					drv_cpu_SVC_prio_set(void);
void 					drv_wdg_refresh(void);
drv_status_type 		drv_wdg_init(void);
drv_status_type 		drv_itm_init(void);
int						drv_itm_print(uint8_t *ptr);
void reset_mcu(void);

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/

#endif /* OS_DRIVERS_DEVICE_CPU_DRV_CPU_H_ */
