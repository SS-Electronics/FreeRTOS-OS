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

#include "drv_gpio.h"



drv_status_type	drv_gpio_int()
{
	drv_status_type drv_status = DRV_OK;

	return drv_status;
}


drv_status_type	drv_gpio_set_pin(drv_gpio_port_list_type port , uint32_t pin)
{
	switch(port)
	{
		case DRV_GPIO_PORT_A:
			HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_B:
			HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_C:
			HAL_GPIO_WritePin(GPIOC, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_D:
			HAL_GPIO_WritePin(GPIOD, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_E:
			HAL_GPIO_WritePin(GPIOE, pin, GPIO_PIN_SET);
		break;

#if defined(STM32H7A3xxQ) || defined(STM32H743xx)
		case DRV_GPIO_PORT_F:
			HAL_GPIO_WritePin(GPIOF, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_G:
			HAL_GPIO_WritePin(GPIOG, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_H:
			HAL_GPIO_WritePin(GPIOH, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_I:
			HAL_GPIO_WritePin(GPIOI, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_J:
			HAL_GPIO_WritePin(GPIOJ, pin, GPIO_PIN_SET);
		break;

		case DRV_GPIO_PORT_K:
			HAL_GPIO_WritePin(GPIOK, pin, GPIO_PIN_SET);
		break;
#endif
	}

	return DRV_OK;
}

drv_status_type drv_gpio_clear_pin(drv_gpio_port_list_type port , uint32_t pin)
{
	switch(port)
	{
		case DRV_GPIO_PORT_A:
			HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_B:
			HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_C:
			HAL_GPIO_WritePin(GPIOC, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_D:
			HAL_GPIO_WritePin(GPIOD, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_E:
			HAL_GPIO_WritePin(GPIOE, pin, GPIO_PIN_RESET);
		break;
#if defined(STM32H7A3xxQ) || defined(STM32H743xx)
		case DRV_GPIO_PORT_F:
			HAL_GPIO_WritePin(GPIOF, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_G:
			HAL_GPIO_WritePin(GPIOG, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_H:
			HAL_GPIO_WritePin(GPIOH, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_I:
			HAL_GPIO_WritePin(GPIOI, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_J:
			HAL_GPIO_WritePin(GPIOJ, pin, GPIO_PIN_RESET);
		break;

		case DRV_GPIO_PORT_K:
			HAL_GPIO_WritePin(GPIOK, pin, GPIO_PIN_RESET);
		break;
#endif
	}
	return DRV_OK;
}


drv_status_type drv_gpio_set_pin_state(drv_gpio_port_list_type port , uint32_t pin, uint8_t pin_state)
{
	switch(port)
	{
		case DRV_GPIO_PORT_A:
			HAL_GPIO_WritePin(GPIOA, pin, pin_state);
		break;

		case DRV_GPIO_PORT_B:
			HAL_GPIO_WritePin(GPIOB, pin, pin_state);
		break;

		case DRV_GPIO_PORT_C:
			HAL_GPIO_WritePin(GPIOC, pin, pin_state);
		break;

		case DRV_GPIO_PORT_D:
			HAL_GPIO_WritePin(GPIOD, pin, pin_state);
		break;

		case DRV_GPIO_PORT_E:
			HAL_GPIO_WritePin(GPIOE, pin, pin_state);
		break;
#if defined(STM32H7A3xxQ) || defined(STM32H743xx)
		case DRV_GPIO_PORT_F:
			HAL_GPIO_WritePin(GPIOF, pin, pin_state);
		break;

		case DRV_GPIO_PORT_G:
			HAL_GPIO_WritePin(GPIOG, pin, pin_state);
		break;

		case DRV_GPIO_PORT_H:
			HAL_GPIO_WritePin(GPIOH, pin, pin_state);
		break;

		case DRV_GPIO_PORT_I:
			HAL_GPIO_WritePin(GPIOI, pin, pin_state);
		break;

		case DRV_GPIO_PORT_J:
			HAL_GPIO_WritePin(GPIOJ, pin, pin_state);
		break;

		case DRV_GPIO_PORT_K:
			HAL_GPIO_WritePin(GPIOK, pin, pin_state);
		break;
#endif
	}
	return DRV_OK;
}


uint8_t	drv_read_pin(drv_gpio_port_list_type port , uint32_t pin)
{
	switch(port)
	{
		case DRV_GPIO_PORT_A:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOA,pin);
		break;

		case DRV_GPIO_PORT_B:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOB,pin);
		break;

		case DRV_GPIO_PORT_C:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOC,pin);
		break;

		case DRV_GPIO_PORT_D:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOD,pin);
		break;

		case DRV_GPIO_PORT_E:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOE,pin);
		break;
#if defined(STM32H7A3xxQ) || defined(STM32H743xx)
		case DRV_GPIO_PORT_F:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOF,pin);
		break;

		case DRV_GPIO_PORT_G:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOG,pin);
		break;

		case DRV_GPIO_PORT_H:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOH,pin);
		break;

		case DRV_GPIO_PORT_I:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOI,pin);
		break;

		case DRV_GPIO_PORT_J:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOJ,pin);
		break;

		case DRV_GPIO_PORT_K:
			return (uint8_t)HAL_GPIO_ReadPin(GPIOK,pin);
		break;
#endif
	}
	return 0;
}




/* Call back funtin */

