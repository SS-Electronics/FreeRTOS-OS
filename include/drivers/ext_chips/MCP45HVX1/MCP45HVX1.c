/*
 * MCP45HVX1.c
 *
 *  Created on: Jan 29, 2024
 *      Author: subhajit-roy
 */
#include "MCP45HVX1.h"

#if (INC_DRIVER_MCP45HVX1 == 1)

#ifdef MCP45HVX1_COMM


static i2c_pdu_struct_type	iic_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[32];




kernel_status_type	drv_mcp45hvx1_iic_init(uint8_t device_address)
{
	kernel_status_type op_status = KERNEL_OK;

	/* Set TCON register */
	data_buffer[0] = 0xFF;
	op_status |= drv_mcp45hvx1_iic_write_register(device_address, ((MCP45HV_REG_TCON_VOL<<4) | (MCP45HV_CMD_WRITE<<2)) , data_buffer, 1);

	/* Initialize wiper pot at 0*/
	op_status |= drv_mcp45hvx1_set_pot(device_address, 0);
	return op_status;
}




kernel_status_type 	drv_mcp45hvx1_set_pot(uint8_t device_address, uint8_t pot_val)
{
	kernel_status_type op_status = KERNEL_OK;

	data_buffer[0] = pot_val;
	op_status |= drv_mcp45hvx1_iic_write_register(device_address, ((MCP45HV_REG_WIPER_VOL<<4) | (MCP45HV_CMD_WRITE<<2)) , data_buffer, 1);

	return op_status;
}


kernel_status_type drv_mcp45hvx1_iic_write_register(uint8_t device_address, uint8_t reg_address, uint8_t* data , uint8_t length)
{
	if(length == 1)	// single register write
	{
		iic_pdu_buffer.device_id 		= MCP45HVX1_COMM;
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
		iic_pdu_buffer.device_id 		= MCP45HVX1_COMM;
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
		if((iic_pdu_buffer.device_id == MCP45HVX1_COMM) && \
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

kernel_status_type drv_mcp45hvx1_iic_read_register(uint8_t device_address, uint8_t reg_address, uint8_t* buff, uint8_t length)
{
	if(length == 1)
	{
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_REG_READ;
	}
	else
	{
		iic_pdu_buffer.iic_operation 	= IIC_OPERATION_READ;
	}

	iic_pdu_buffer.device_id 		= MCP45HVX1_COMM;
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
		if((iic_pdu_buffer.device_id == MCP45HVX1_COMM) && \
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
#error "MCP45HVX1_COMM Not defined!"
#endif


#else // If there is no inclusion give the warning
#warning "MC23017 Module Driver Not included!!"
#endif

