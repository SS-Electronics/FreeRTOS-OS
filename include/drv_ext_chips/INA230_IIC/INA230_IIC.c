/*
 * INA230_IIC.c
 *
 *  Created on: Jan 11, 2024
 *      Author: subhajit-roy
 */

#include "INA230_IIC.h"

#if (INC_DRIVER_INA230 == 1)
#ifdef INA230_COMM

static i2c_pdu_struct_type	iic_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[32];
static uint8_t				rx_buffer[4];

kernel_status_type	drv_ina230_iic_init(uint8_t device_address)
{
	/* Get the queue handles */
	kernel_status_type status = KERNEL_OK;

	uint16_t regVal = 0x4000;
	/* Form command register value. */
	regVal |= (INA230_AVG_128 << INA230_AVG_BITPOS);
	regVal |= (INA230_CT_1_1MS << INA230_VBUS_CT_BITPOS);
	regVal |= (INA230_CT_1_1MS << INA230_VSH_CT_BITPOS);
	/* Select continuous VBUS & VSHUNT measurement mode. */
	regVal |= 0x07;
	/* Reg Val MSB. */
	data_buffer[0] = regVal >> 8;
	/* Reg Val LSB. */
	data_buffer[1] = regVal;
	status |= drv_ina230_iic_write_register(device_address, INA230_CONFIG_REG, data_buffer, 2);


	/* Calibration value */
	regVal = 0x0000;
	/* Calculate calibration register value. */
    regVal = (uint16_t)(5120.0/(INA230_CURRENT_LSB * INA230_RSHUNT_MILLIOHMS));
    /* Reg Val MSB. */
    data_buffer[0] = regVal >> 8;
    /* Reg Val LSB. */
    data_buffer[1] = regVal;
    status |= drv_ina230_iic_write_register(device_address, INA230_CALIB_REG, data_buffer, 2);

	return status;

}


float	drv_ina230_iic_read_volt(uint8_t device_address)
{
	uint16_t reg_val;
	float result;

	if(drv_ina230_iic_read_register(device_address, INA230_VBUS_REG, rx_buffer, 2) == KERNEL_OK)
	{
		reg_val = (rx_buffer[0] << 8);
		reg_val |= rx_buffer[1];
		result = (((float)reg_val * INA230_VBUS_LSB)/1000.0f);
	}
	else
	{
		result = -1;
	}
	return result;
}

float drv_ina230_iic_read_current(uint8_t device_address)
{
	uint16_t reg_val;
	float result;

	if(drv_ina230_iic_read_register(device_address, INA230_CURRENT_REG, rx_buffer, 2) == KERNEL_OK)
	{
		reg_val = (rx_buffer[0] << 8);
		reg_val |= rx_buffer[1];
		result = ((float)reg_val * INA230_CURRENT_LSB);

	}
	else
	{
		result = -1;
	}

	return result;
}





/* Thease are basic read and write operation */
kernel_status_type drv_ina230_iic_write_register(uint8_t device_address, uint8_t reg_address, uint8_t* data , uint8_t length)
{
	if(length == 1)	// single register write
	{
		iic_pdu_buffer.device_id 		= INA230_COMM;
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_REG_WRITE;
		iic_pdu_buffer.slave_address 	= device_address;
		iic_pdu_buffer.reg_addr 		= reg_address;
		iic_pdu_buffer.txn_data[0] 		= data[0];
		/* do a IIC operation */
		xQueueSend(pipe_iic_pdu_tx_handle,
					&iic_pdu_buffer,
					(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
				  );
	}
	else
	{
		iic_pdu_buffer.device_id 		= INA230_COMM;
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_WRITE;
		iic_pdu_buffer.slave_address 	= device_address;
		iic_pdu_buffer.txn_data[0] 		= reg_address;
		iic_pdu_buffer.length 			= length + 1;
		for(int i = 0; i < length; i++)
		{
			iic_pdu_buffer.txn_data[i+1] =  data[i];
		}
		/* do a IIC operation */
		xQueueSend(pipe_iic_pdu_tx_handle,
					&iic_pdu_buffer,
					(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
				  );
	}
	/* Mark the current time */
	iic_pdu_buffer.op_status = KERNEL_ERR;
	set_time_ms = drv_time_get_ticks();
	/* wait for the feedback */
	while((drv_time_get_ticks() - set_time_ms) < IIC_ACK_TIMEOUT_MS)
	{
		xQueuePeek( pipe_iic_pdu_rx_handle,
		             &iic_pdu_buffer,
					(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
		           );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == INA230_COMM) && \
		   (iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive(pipe_iic_pdu_rx_handle,
							&iic_pdu_buffer,
							(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
						  );
		    break;
		}
	}

	return iic_pdu_buffer.op_status;
}

kernel_status_type drv_ina230_iic_read_register(uint8_t device_address, uint8_t reg_address, uint8_t* buff, uint8_t length)
{
	if(length == 1)
	{
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_REG_READ;
	}
	else
	{
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_READ;
	}

	iic_pdu_buffer.device_id 		= INA230_COMM;
	iic_pdu_buffer.slave_address 	= device_address;
	iic_pdu_buffer.reg_addr 		= reg_address;
	iic_pdu_buffer.length 		    = length;
	/* do a IIC operation */
	xQueueSend(pipe_iic_pdu_tx_handle,
				&iic_pdu_buffer,
				(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
			  );

	/* Mark the current time */
	iic_pdu_buffer.op_status = KERNEL_ERR;
	set_time_ms = drv_time_get_ticks();

	while((drv_time_get_ticks() - set_time_ms) < IIC_ACK_TIMEOUT_MS)
	{
		xQueuePeek( pipe_iic_pdu_rx_handle,
			         &iic_pdu_buffer,
					 (TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
			      );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == INA230_COMM) && \
				(iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive(pipe_iic_pdu_rx_handle,
							&iic_pdu_buffer,
							(TickType_t)pdMS_TO_TICKS(TIMEOUT_IIC_PIPE_OP)
						  );
			 break;
		}
	}

	for(int i = 0; i<length; i++)
	{
		buff[i] = iic_pdu_buffer.txn_data[i];
	}

	return iic_pdu_buffer.op_status;
}



#else
#error "INA230_IIC driver included but INA230_COMM not defined!"
#endif // end #ifdef INA230_COMM
#endif // end #if (INC_DRIVER_INA230 == 1)
