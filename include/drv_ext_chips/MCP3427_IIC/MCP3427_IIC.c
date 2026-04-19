/*
 * MCP3427_IIC.c
 *
 *  Created on: Feb15, 2024
 *      Author: koripalli yaswanth
 */


#include "MCP3427_IIC.h"

#if (INC_DRIVER_MCP3427 == 1)
#ifdef MCP3427_COMM

static i2c_pdu_struct_type	iic_tx_pdu_buffer, iic_rx_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[4];
static uint8_t				rx_buffer[4];

static uint8_t print_buffer[PRINTK_BUFF_LENGTH];


kernel_status_type	drv_mcp3427_iic_init(uint8_t device_address)
{
	/* Get the queue handles */

	kernel_status_type status = KERNEL_OK;

    data_buffer[0] = 0x80;
    status |= drv_mcp3427_iic_write_register(device_address,data_buffer,1);

	return status;

}

float	drv_mcp3427_iic_read_volt(uint8_t device_address, uint8_t channel)
{
	uint16_t adcValue;
	float voltage;
	kernel_status_type status = KERNEL_OK;

	switch(channel)
	{
		case MCP3427_CH_1:
			 data_buffer[0] = 0x80;
			 status |= drv_mcp3427_iic_write_register(device_address,data_buffer,1);

		break;

		case MCP3427_CH_2:
			 data_buffer[0] = 0xA0;
			 status |= drv_mcp3427_iic_write_register(device_address,data_buffer,1);
		break;
	}
	vTaskDelay(pdMS_TO_TICKS(1));

	status |= drv_mcp3427_iic_read_register(device_address,rx_buffer, 3);
	if (status == KERNEL_OK)
	{

     	  adcValue = (rx_buffer[0] << 8) | rx_buffer[1];
		  int16_t adc = (int16_t)(adcValue);
		  voltage = (adc * LSB_12BIT);
	}
	else
	{
		voltage = -1.0;
	}
	return voltage;
}

/* Thease are basic read and write operation */
kernel_status_type drv_mcp3427_iic_write_register(uint8_t device_address, uint8_t* data , uint8_t length)
{
	/* Service request to IIC management */
	iic_tx_pdu_buffer.device_id 		= MCP3427_COMM;
	iic_tx_pdu_buffer.iic_operation 	= IIC_OPERATION_WRITE;
	iic_tx_pdu_buffer.slave_address 	= device_address;
	iic_tx_pdu_buffer.txn_data[0] 		= data[0];
	iic_tx_pdu_buffer.length 			= length;

	/* do a IIC operation */
	xQueueSend( pipe_iic_pdu_tx_handle,
				&iic_tx_pdu_buffer,
				(TickType_t)0
			  );

	/* wait for Service response from  IIC management */
	/* Mark the current time */
	iic_rx_pdu_buffer.op_status = KERNEL_ERR;
	set_time_ms = drv_time_get_ticks();
	/* wait for the feedback */
	while((drv_time_get_ticks() - set_time_ms) < IIC_ACK_TIMEOUT_MS)
	{
		if(xQueuePeek( pipe_iic_pdu_rx_handle,
		             &iic_rx_pdu_buffer,
					(TickType_t)0
		           ) == pdTRUE )
		{
			/* If the PDU receive for */
			if((iic_rx_pdu_buffer.device_id == MCP3427_COMM) && \
			   (iic_rx_pdu_buffer.slave_address == device_address))
			{
				if(xQueueReceive(pipe_iic_pdu_rx_handle,
								&iic_rx_pdu_buffer,
								(TickType_t)0
							  ) == pdTRUE)
				{
					break; // break from wait loop and give error status
				}

			}
		}
	}

	return iic_rx_pdu_buffer.op_status;
}

kernel_status_type drv_mcp3427_iic_read_register(uint8_t device_address, uint8_t* buff, uint8_t length)
{
	/* Service request to IIC management */
	iic_tx_pdu_buffer.iic_operation 	= IIC_OPERATION_READ;
	iic_tx_pdu_buffer.device_id 		= MCP3427_COMM;
	iic_tx_pdu_buffer.slave_address 	= device_address;
	iic_tx_pdu_buffer.length 		    = length;
	/* do a IIC operation */
	xQueueSend( pipe_iic_pdu_tx_handle,
				&iic_tx_pdu_buffer,
				(TickType_t)0
			  );

	/* wait for Service response from  IIC management */
	/* Mark the current time */
	iic_rx_pdu_buffer.op_status = KERNEL_ERR;
	set_time_ms = drv_time_get_ticks();

	while((drv_time_get_ticks() - set_time_ms) < IIC_ACK_TIMEOUT_MS)
	{
		if(xQueuePeek( pipe_iic_pdu_rx_handle,
					 &iic_rx_pdu_buffer,
					(TickType_t)0
				   ) == pdTRUE )
		{
			/* If the PDU receive for */
			if((iic_rx_pdu_buffer.device_id == MCP3427_COMM) && \
			   (iic_rx_pdu_buffer.slave_address == device_address))
			{
				if(xQueueReceive(pipe_iic_pdu_rx_handle,
								&iic_rx_pdu_buffer,
								(TickType_t)0
							  ) == pdTRUE)
				{
					break; // break from wait loop and give error status
				}

			}
		}
	}

	for(int i = 0; i<length; i++)
	{
		buff[i] = iic_rx_pdu_buffer.txn_data[i];
	}

	return iic_rx_pdu_buffer.op_status;
}


#else
#error "MCP3427_IIC driver included but MCP3427_COMM not defined!"
#endif // end #ifdef MCP3427_COMM
#endif // end #if (INC_DRIVER_MCP3427 == 1)
