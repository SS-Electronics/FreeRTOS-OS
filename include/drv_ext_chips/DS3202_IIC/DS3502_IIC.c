/*
 * DS3502.c
 *
 *  Created on: Sep 28, 2023
 *      Author: subhajit-roy
 */
#include "DS3502_IIC.h"

#if (INC_DRIVER_DS3502 == FLAG_SET)

#ifdef DS3502_COMM // Check for COM bus

/* IIC management IPC ref  */
static i2c_pdu_struct_type	iic_tx_pdu_buffer, iic_rx_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[4];
static uint8_t				rx_buffer[4];



kernel_status_type drv_ds3502_iic_init(uint8_t device_address)
{
	kernel_status_type op_status = KERNEL_OK;

	/* Set the IOCON register
	 * AUTO increment enabled */
	data_buffer[0] = REG_CR;
	data_buffer[1] = 0;
	op_status |= drv_ds3502_iic_write_register(device_address, data_buffer,2);
	/* All set as output for both PORT A and PORT B*/
	drv_time_delay_ms(1);

	return op_status;
}

kernel_status_type	drv_ds3502_iic_write_pot(uint8_t device_address, uint8_t pot_val)
{
	kernel_status_type op_status = KERNEL_OK;
	/**/
	data_buffer[0] = REG_CTRL;
	data_buffer[1] = pot_val;
	op_status |= drv_ds3502_iic_write_register(device_address, data_buffer, 2);

	return op_status;
}


/* Thease are basic read and write operation */
kernel_status_type drv_ds3502_iic_write_register(uint8_t device_address, uint8_t* data , uint8_t length)
{
	/* Service request to IIC management */
	iic_tx_pdu_buffer.device_id 		= DS3502_COMM;
	iic_tx_pdu_buffer.iic_operation 	= IIC_OPERATION_WRITE;
	iic_tx_pdu_buffer.slave_address 	= device_address;
	iic_tx_pdu_buffer.txn_data[0] 		= data[0];
	iic_tx_pdu_buffer.txn_data[1] 		= data[1];
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
			if((iic_rx_pdu_buffer.device_id == DS3502_COMM) && \
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

uint8_t drv_ds3502_iic_read_register(uint8_t device_address, uint8_t* buff, uint8_t length)
{
	/* Service request to IIC management */
	iic_tx_pdu_buffer.iic_operation 	= IIC_OPERATION_READ;
	iic_tx_pdu_buffer.device_id 		= DS3502_COMM;
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
			if((iic_rx_pdu_buffer.device_id == DS3502_COMM) && \
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
#error "DS3502 Driver included but bus DS3502_COMM bus not defined!!"
#endif

#endif
