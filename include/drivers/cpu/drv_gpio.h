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

#ifndef OS_DRIVERS_DEVICE_GPIO_DRV_GPIO_H_
#define OS_DRIVERS_DEVICE_GPIO_DRV_GPIO_H_

/*Configurations */
#include <app/bsw_config/arch_conf/board/board.h>
#include <bsw/utils/type_def/typedef_global.h>
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

drv_status_type		drv_gpio_int();
drv_status_type		drv_gpio_set_pin(drv_gpio_port_list_type port , uint32_t pin);
drv_status_type		drv_gpio_clear_pin(drv_gpio_port_list_type port , uint32_t pin);
drv_status_type 	drv_gpio_set_pin_state(drv_gpio_port_list_type port , uint32_t pin, uint8_t pin_state);
uint8_t				drv_read_pin(drv_gpio_port_list_type port , uint32_t pin);


#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* OS_DRIVERS_DEVICE_GPIO_DRV_GPIO_H_ */
