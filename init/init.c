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

#include <init/init.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/

static type_drv_hw_handle handle_os_harware;



/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_uart_handle	(__TYPE_HW_UART_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_UART)
	{
		handle_os_harware.handle_hw_uart[hw_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_iic_handle	(__TYPE_HW_IIC_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_IIC)
	{
		handle_os_harware.handle_hw_iic[hw_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_spi_handle	(__TYPE_HW_SPI_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_SPI)
	{
		handle_os_harware.handle_hw_spi[hw_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_adc_handle	(__TYPE_HW_ADC_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_ADC)
	{
		handle_os_harware.handle_hw_adc[hw_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_timer_handle	(__TYPE_HW_TIMER_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_TIMER)
	{
		handle_os_harware.handle_hw_timer[hw_id] = handle;
	}
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void os_add_drv_iwdg_handle	(__TYPE_HW_IWDG_HANDLE_TYPE * handle)
{
	if(IWDG_INCLUDE == 1)
	{
		handle_os_harware.handle_hw_iwdg = handle;
	}
}


/**
 * OS entry function give control to kernel and start OS
 *
 */
void os_entry(void)
{
	/* Send the driver handles to each driver */
	drv_serial_update_handle(&type_drv_hw_handle);



//	drv_wdg_handle_ref = drv_wdg_get_handle();
//
//	drv_wdg_handle_ref->handle = handle_list->wdg_handle;
//
//	/* Initialize the Service */
//	kernel_status_type status = service_init();
//	/* is status is ok and manual tests are not
//	 * enabled then Start the kernel */
//	/* IF the manual tests are not enabled */
//
//	if(status == KERNEL_OK)
//	{
//		service_start_kernel();
//	}
//	/* Program shouldn't go after that  */
	while(1)
	{

	}

}



