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

#include "drv_iic.h"
#if (INC_DRIVER_COMM_IIC == 1)

/* IIC driver handle */
static drv_iic_handle_type 		iic_handle;
/* Global driver status */
extern global_drv_status_type 	global_drv_status;
/* Kernel Event handles */
static EventGroupHandle_t * 	event_drv_iic_ref;
static BaseType_t 				iic_HigherPriorityTaskWoken,
								iic_event_get_result;
/*initialization status */
static uint8_t 					initializaiton_counter;
static uint32_t					iic_counter = 0;
/* Operational Variables*/

/**
  * @brief  Get the driver level handles to boot
  * 		and pass the low level handles.
  * @param  None
  * @retval pointer to iic driver handle
  */
drv_iic_handle_type * drv_iic_get_handle(void)
{
	return &iic_handle;
}

/**
  * @brief  Initialize the driver
  * 		if the driver already initialized at low level
  * 		don't init it again..
  * @param  None
  * @retval Driver operation status.
  */
drv_status_type drv_iic_init(void)
{
	drv_status_type status  = DRV_OK;
	/* Variables reset */
	initializaiton_counter = 0;
	/* Low level initialization has been done */
	/* Get the event handle */
	event_drv_iic_ref = ipc_signal_get_event_handle(IPC_EVENT_IIC_DRV);
	/*-------------------------------------------------------*/
	/* Check the status of the driver initialize the extended functionality */
	for(int i = 0; i<CONFIG_MCU_NO_OF_IIC_PERIPHERAL; i++)
	{
		/* Check if the handles are NULL or not */
		if(iic_handle.handle[i] != NULL)
		{
			/* on every driver initialization we are incrementing the variable */
			initializaiton_counter++;
			global_drv_status.drv_serial[i].status = HAL_I2C_GetState(iic_handle.handle[i]);
			/* Update the error state */
			global_drv_status.drv_serial[i].error_state = HAL_I2C_GetError(iic_handle.handle[i]);

			/* If any extra functionality functionality need to initialize */
		}
	}
	/*-------------------------------------------------------*/
	return status;
}

/**
  * @brief  Transmit one PDU to bus
  * @param  Device ID (which IIC handle)
  * @param  Slave Device Address
  * @param  data array pointer
  * @param  length of data to be transmit
  * @retval Driver operation status.
  */
drv_status_type	drv_iic_transmit(uint8_t dev_id, uint16_t device_address, uint8_t* data, uint16_t len)
{
	drv_status_type status = DRV_BUSY;
	if(initializaiton_counter!=0)
	{
		status = HAL_I2C_Master_Transmit(iic_handle.handle[dev_id], device_address, data , len, 1500);
		return status;
	}
	else
	{
		iic_counter = iic_counter + 1;
		if (dev_id == IIC_1)
		{
			syscall_diagnostics_log(ERR_1200_IIC1_TX, iic_counter);
		}
		else if (dev_id == IIC_2)
		{
			syscall_diagnostics_log(ERR_1225_IIC2_TX, iic_counter);
		}
		else if (dev_id == IIC_3){
			syscall_diagnostics_log(ERR_1250_IIC3_TX, iic_counter);
		}
		else{
			syscall_diagnostics_log(ERR_1275_IIC4_TX, iic_counter);
		}
		return DRV_ERR;
	}
}

/**
  * @brief  Receive one PDU to bus
  * @param  Device ID (which IIC handle)
  * @param  Slave Device Address
  * @param  data array pointer
  * @param  length of data to be transmit
  * @retval Driver operation status.
  */
drv_status_type	drv_iic_receive(uint8_t dev_id, uint16_t device_address, uint8_t* data, uint16_t len)
{
	drv_status_type status = DRV_BUSY;
	if(initializaiton_counter!=0)
	{
		status = HAL_I2C_Master_Receive(iic_handle.handle[dev_id], device_address, data , len, 1500);
		return status;
	}
	else
	{
		if (dev_id == IIC_1)
		{
			syscall_diagnostics_log(ERR_1201_IIC1_RX, iic_counter);
		}
		else if (dev_id == IIC_2)
		{
			syscall_diagnostics_log(ERR_1226_IIC2_RX, iic_counter);
		}
		else if (dev_id == IIC_3){
			syscall_diagnostics_log(ERR_1251_IIC3_RX, iic_counter);
		}
		else{
			syscall_diagnostics_log(ERR_1276_IIC4_RX, iic_counter);
		}
		return DRV_ERR;
	}
}

/**
  * @brief  Get the device status
  * @param  Device ID (which IIC handle)
  * @param  Slave Device Address
  * @retval Driver operation status.
  */
drv_status_type	drv_iic_device_ready(uint8_t dev_id, uint16_t device_address)
{
	drv_status_type drv_status;
	drv_status = HAL_I2C_IsDeviceReady(iic_handle.handle[dev_id],
								device_address,
								2,
								50);
	return drv_status;
}



/**
  * @brief Callbacks
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	/* if all the drivers are initialized properly */
	if(initializaiton_counter!=0)
	{

		/* update the particular device error get */
		if(hi2c == iic_handle.handle[IIC_1])
		{
			global_drv_status.drv_iic[IIC_1].status = HAL_I2C_GetState(iic_handle.handle[IIC_1]);
		}
		else if(hi2c == iic_handle.handle[IIC_2])
		{
			global_drv_status.drv_iic[IIC_2].status = HAL_I2C_GetState(iic_handle.handle[IIC_2]);
		}
		else if(hi2c == iic_handle.handle[IIC_3])
		{
			global_drv_status.drv_iic[IIC_3].status = HAL_I2C_GetState(iic_handle.handle[IIC_3]);
		}
		else if(hi2c == iic_handle.handle[IIC_4])
		{
			global_drv_status.drv_iic[IIC_4].status = HAL_I2C_GetState(iic_handle.handle[IIC_4]);
		}

		/*Set the event */
		iic_HigherPriorityTaskWoken = pdFALSE;
		iic_event_get_result = xEventGroupSetBitsFromISR(
							  	*event_drv_iic_ref,
								 EVENT_BIT_DRV_ERR_GET,
					             &iic_HigherPriorityTaskWoken );
		/* If the message passed successfully */
		if( iic_event_get_result != pdFAIL )
		{
			portYIELD_FROM_ISR( iic_HigherPriorityTaskWoken );
		}
	}

}

#endif

