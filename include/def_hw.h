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

#ifndef INCLUDE_DEF_HW_H_
#define INCLUDE_DEF_HW_H_


#include <def_compiler.h>
#include <def_env_macros.h>

#include <conf_os.h>






typedef struct
{

	uint8_t registered_peripherals;

#if(NO_OF_UART > 0)
	__TYPE_HW_UART_HANDLE_TYPE		*handle_hw_uart[NO_OF_UART];
#endif

#if(NO_OF_IIC > 0)
	__TYPE_HW_IIC_HANDLE_TYPE		*handle_hw_iic[NO_OF_IIC];
#endif

#if(NO_OF_SPI > 0)
	__TYPE_HW_SPI_HANDLE_TYPE		*handle_hw_spi[NO_OF_SPI];
#endif

#if(NO_OF_TIMER > 0)
	__TYPE_HW_ADC_HANDLE_TYPE		*handle_hw_adc[NO_OF_ADC];
#endif

#if(IWDG_INCLUDE == 1)
	__TYPE_HW_IWDG_HANDLE_TYPE		*handle_hw_iwdg;
#endif

#if(NO_OF_TIMER > 0)
	__TYPE_HW_TIMER_HANDLE_TYPE     *handle_hw_timer[NO_OF_TIMER];
#endif

}type_drv_hw_handle;


typedef enum
{
	HW_ID_UART_1 = 0,
	HW_ID_UART_2,
	HW_ID_UART_3,
	HW_ID_UART_4,
	HW_ID_UART_5,
	HW_ID_UART_6,
	HW_ID_UART_7,
	HW_ID_UART_8,
	HW_ID_UART_9,
	HW_ID_UART_10

}type_drv_hw_uart_list;


typedef enum
{
	HW_ID_IIC_1 = 0,
	HW_ID_IIC_2,
	HW_ID_IIC_3,
	HW_ID_IIC_4,
	HW_ID_IIC_5,
	HW_ID_IIC_6,
	HW_ID_IIC_7,
	HW_ID_IIC_8,
	HW_ID_IIC_9,
	HW_ID_IIC_10

}type_drv_hw_iic_list;


typedef enum
{
	HW_ID_SPI_1 = 0,
	HW_ID_SPI_2,
	HW_ID_SPI_3,
	HW_ID_SPI_4,
	HW_ID_SPI_5,
	HW_ID_SPI_6,
	HW_ID_SPI_7,
	HW_ID_SPI_8,
	HW_ID_SPI_9,
	HW_ID_SPI_10

}type_drv_hw_spi_list;


typedef enum
{
	HW_ID_TIM_1 = 0,
	HW_ID_TIM_2,
	HW_ID_TIM_3,
	HW_ID_TIM_4,
	HW_ID_TIM_5,
	HW_ID_TIM_6,
	HW_ID_TIM_7,
	HW_ID_TIM_8,
	HW_ID_TIM_9,
	HW_ID_TIM_10

}type_drv_hw_timer_list;



#endif /* INCLUDE_DEF_HW_H_ */
