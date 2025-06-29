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

static int32_t mqueue_id_1_printk_buffer = 0;


/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_UART > 0)
void os_add_drv_uart_handle	(__TYPE_HW_UART_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_UART)
	{
		handle_os_harware.handle_hw_uart[hw_id] = handle;
	}
}
#endif

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_IIC > 0)
void os_add_drv_iic_handle	(__TYPE_HW_IIC_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_IIC)
	{
		handle_os_harware.handle_hw_iic[hw_id] = handle;
	}
}
#endif

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_SPI > 0)
void os_add_drv_spi_handle	(__TYPE_HW_SPI_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_SPI)
	{
		handle_os_harware.handle_hw_spi[hw_id] = handle;
	}
}
#endif

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_ADC > 0)
void os_add_drv_adc_handle	(__TYPE_HW_ADC_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_ADC)
	{
		handle_os_harware.handle_hw_adc[hw_id] = handle;
	}
}
#endif

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_TIMER > 0)
void os_add_drv_timer_handle	(__TYPE_HW_TIMER_HANDLE_TYPE * handle, uint8_t hw_id)
{
	if(hw_id < NO_OF_TIMER)
	{
		handle_os_harware.handle_hw_timer[hw_id] = handle;
	}
}
#endif

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (IWDG_INCLUDE > 0)
void os_add_drv_iwdg_handle	(__TYPE_HW_IWDG_HANDLE_TYPE * handle)
{
	if(IWDG_INCLUDE == 1)
	{
		handle_os_harware.handle_hw_iwdg = handle;
	}
}
#endif

/* *****************************************************
 *
 * week definition  of app_main()
 *
 * *****************************************************/
__attribute__((weak)) int app_main(void)
{

	return ERROR_NONE;
}


/**
 * OS entry function give control to kernel and start OS
 *
 */
void os_entry(void)
{
	status_type status = ERROR_NONE;

	/*
	 * Send the driver handles to each driver
	 *  */
#if (NO_OF_UART > 0)
	for(int i = 0; i < NO_OF_UART; i++)
	{
		drv_serial_update_handle(handle_os_harware.handle_hw_uart[i], i);
	}
#endif

#if (NO_OF_IIC > 0)
	for(int i = 0; i < NO_OF_TIMER; i++)
	{
//		drv_iic_update_handle(handle_os_harware.handle_hw_iic[i], i);
	}
#endif

	/*
	 * Initialize all the message queues
	 * */
	status |= ipc_mqueue_init();



	/*
	 * Register all the kernel threads
	 * */
	status |= os_kernel_thread_register();


	/* Entry point to the main */
	status |= app_main();


	drv_cpu_interrupt_prio_set();

	if(status == ERROR_NONE)
	{
		/*Start the schedular */
		vTaskStartScheduler();
	}



	/* Program shouldn't go after that  */
	while(1)
	{

	}

}






