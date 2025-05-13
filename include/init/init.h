/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS OS project
#
# RTOS OS project is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS OS project is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#ifndef _INIT_H_
#define _INIT_H_

/* Device drivers include for getting the handles*/
#include <def_compiler.h>
#include <def_env_macros.h>
#include <def_hw.h>

/* Typedef of LOW level driver handles list */

/* Here we have accommodate all the handles as per of the requirements */









/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

void 		os_add_drv_uart_handle			(__TYPE_HW_UART_HANDLE_TYPE * uart_handle, uint8_t	hw_id);
void 		os_add_drv_iic_handle			(__TYPE_HW_IIC_HANDLE_TYPE * iic_handle, uint8_t hw_id);
void 		os_add_drv_spi_handle			(__TYPE_HW_SPI_HANDLE_TYPE * iic_handle, uint8_t hw_id);
void 		os_add_drv_adc_handle			(__TYPE_HW_ADC_HANDLE_TYPE * iic_handle, uint8_t hw_id);
void 		os_add_drv_timer_handle			(__TYPE_HW_TIMER_HANDLE_TYPE * iic_handle, uint8_t hw_id);
void 		os_add_drv_iwdg_handle			(__TYPE_HW_IWDG_HANDLE_TYPE * iic_handle, uint8_t hw_id);

/* Entry Point of the OS and APP */
void 		os_entry						(void);


#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/











#endif
