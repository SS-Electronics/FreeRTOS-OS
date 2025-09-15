/*
 * MCP4441_IIC.c
 *
 *  Created on: Feb 19, 2024
 *      Author: sourav-dey
 */
/*
 * MCP4441_IIC.c
 *
 *  Created on: Feb 19, 2024
 *      Author: sourav-dey
 */

#include "MCP4441_IIC.h"
#if (INC_DRIVER_MCP4441 == 1)

#ifdef MCP4441_COMM

/* IIC management IPC ref  */
static i2c_pdu_struct_type	iic_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[32];



kernel_status_type		drv_mcp4441_iic_init(uint8_t device_address)
{
	kernel_status_type op_status = KERNEL_OK;

	pipe_iic_write_ref = ipc_get_queue_handle(PIPE_IIC_PDU_TX);
	pipe_iic_read_ref  = ipc_get_queue_handle(PIPE_IIC_PDU_RX);

	/* Set the TCON0 and TCON1 register*/

	data_buffer[0] = 0b11111111;

	/*Initializing the TCON0 register.*/
	op_status |= drv_mcp4441_iic_write_register(device_address,TCON_REG_0, data_buffer, 1);
	drv_time_delay_ms(1);

	/*Initializing the TCON1 register.*/
	op_status |= drv_mcp4441_iic_write_register(device_address,TCON_REG_1, data_buffer, 1);
	drv_time_delay_ms(1);

	return op_status;
}


kernel_status_type drv_mcp4441_iic_write_pot(uint8_t device_address, uint8_t w_num, uint8_t pot_val)
{
	kernel_status_type op_status = KERNEL_OK;
	data_buffer[0] = pot_val;
	switch (w_num)
	{

		case WIPER_0:
			op_status |= drv_mcp4441_iic_write_register(device_address, WIPER_REG_0,data_buffer,1);
			break;

		case WIPER_1:
			op_status |= drv_mcp4441_iic_write_register(device_address, WIPER_REG_1,data_buffer,1);
			break;

		case WIPER_2:
			op_status |= drv_mcp4441_iic_write_register(device_address, WIPER_REG_2,data_buffer,1);
			break;

		case WIPER_3:
			op_status |= drv_mcp4441_iic_write_register(device_address, WIPER_REG_3,data_buffer,1);
			break;

	}
	return op_status;
}


/* The basic write operation for IIC MCP4441. */
kernel_status_type 		drv_mcp4441_iic_write_register(uint8_t device_address,uint8_t reg_address, uint8_t* data , uint8_t length)
{
	if (length == 1)	// Single register write
	{
		iic_pdu_buffer.device_id = MCP4441_COMM;
		iic_pdu_buffer.iic_operation = IIC_OPERATION_REG_WRITE;
		iic_pdu_buffer.slave_address = device_address;
		iic_pdu_buffer.reg_addr = reg_address;
		iic_pdu_buffer.txn_data[0] = data[0];
		/* do a IIC operation */
		xQueueSend(pipe_iic_pdu_tx_handle,
					&iic_pdu_buffer,
					(TickType_t)0
				  );
	}
	else{
		iic_pdu_buffer.device_id = MCP4441_COMM;
		iic_pdu_buffer.iic_operation = IIC_OPERATION_WRITE;
		iic_pdu_buffer.slave_address = device_address;
		iic_pdu_buffer.txn_data[0] = reg_address;
		for (int i = 0; i < length; i ++){
			iic_pdu_buffer.txn_data[i+1] = data[i];
		}
		/* do a IIC operation */
		xQueueSend(pipe_iic_pdu_tx_handle,
					&iic_pdu_buffer,
					(TickType_t)0
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
					(TickType_t)0
				   );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == MCP4441_COMM) && \
		   (iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive(pipe_iic_pdu_rx_handle,
							&iic_pdu_buffer,
							(TickType_t)0
						  );
			break;
		}
	}

	return iic_pdu_buffer.op_status;
}

/* The basic read operation for IIC MCP4441*/
kernel_status_type		drv_mcp4441_iic_read_register(uint8_t device_address,uint8_t reg_address)
{
	iic_pdu_buffer.device_id = MCP4441_COMM;
	iic_pdu_buffer.iic_operation = IIC_OPERATION_REG_READ;
	iic_pdu_buffer.slave_address = device_address;
	iic_pdu_buffer.reg_addr = reg_address;
	/* do a IIC operation */
	xQueueSend(pipe_iic_pdu_tx_handle,
				&iic_pdu_buffer,
				(TickType_t)0
			  );

	/* Mark the current time */
	iic_pdu_buffer.op_status = KERNEL_ERR;
	set_time_ms = drv_time_get_ticks();

	while((drv_time_get_ticks() - set_time_ms) < IIC_ACK_TIMEOUT_MS)
	{
		xQueuePeek( pipe_iic_pdu_rx_handle,
					 &iic_pdu_buffer,
					 (TickType_t)0
				  );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == MCP4441_COMM) && \
				(iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive(pipe_iic_pdu_rx_handle,
							&iic_pdu_buffer,
							(TickType_t)0
						  );
			 break;
		}
	}
	if(iic_pdu_buffer.op_status == KERNEL_OK)
		return iic_pdu_buffer.txn_data[0];
	else
		return 2;
}

#else
#error  "MCP441Driver included but bus MCP4441_COMM bus not defined!!"
#endif

#endif



