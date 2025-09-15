/*
 * MCP23017_IIC.c
 *
 *  Created on: Sep 27, 2023
 *      Author: subhajit-roy
 */

/*
 * PCA9685_IIC.c
 *
 *  Created on: Sep 26, 2023
 *      Author: subhajit-roy
 */
#include "MCP23017_IIC.h"

#if (INC_DRIVER_MC23017 == 1)

#ifdef MCP23017_COMM

/* IIC management IPC ref  */
static i2c_pdu_struct_type	iic_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[32];
static uint8_t 				port_a_reg_val; // save the local data of register
static uint8_t 				port_b_reg_val; // save the local data of register


kernel_status_type drv_mcp23017_iic_init(uint8_t device_address)
{
	kernel_status_type op_status = KERNEL_OK;

	/* dafault port values */
	port_a_reg_val = 0;
	port_b_reg_val = 0;

	/* Set the IOCON register
	 * AUTO increment enabled */
	data_buffer[0] = 0b00111010;
	op_status |= drv_mcp23017_iic_write_register(device_address,REG_IOCON, data_buffer, 1);
	/* All set as output for both PORT A and PORT B*/
	drv_time_delay_ms(1);

	data_buffer[0] = port_a_reg_val;
	op_status |= drv_mcp23017_iic_write_register(device_address,REG_IODIR_A, data_buffer, 1);
	data_buffer[0] = port_b_reg_val;
	op_status |= drv_mcp23017_iic_write_register(device_address,REG_IODIR_A + 1, data_buffer, 1);

	data_buffer[0] = port_a_reg_val;
	op_status |= drv_mcp23017_iic_write_register(device_address,REG_OLATA, data_buffer, 1);
	data_buffer[0] = port_b_reg_val;
	op_status |= drv_mcp23017_iic_write_register(device_address, REG_OLATA + 1,data_buffer,1);

	drv_time_delay_ms(1);

	return op_status;
}

kernel_status_type drv_mcp23017_iic_write(uint8_t device_address, uint8_t port , uint8_t pin, uint8_t state)
{
	kernel_status_type op_status = KERNEL_OK;
	switch(port)
	{
		case PORT_A:
//			port_reg_val = drv_mcp23017_iic_read_register(REG_OLATA);
			switch(state)
			{
				case FLAG_SET:
					port_a_reg_val |= (1<<pin);
				break;

				case FLAG_RESET:
					port_a_reg_val &= ~(1<<pin);
				break;
			}
			data_buffer[0] = port_a_reg_val;
			op_status |= drv_mcp23017_iic_write_register(device_address, REG_GPIOA,data_buffer,1);

		break;

		case PORT_B:
			/* TODO: reading not working properly so saving data locally */
//			port_reg_val = drv_mcp23017_iic_read_register(REG_GPIOA + 1);
			switch(state)
			{
				case FLAG_SET:
					port_b_reg_val |= (1<<pin);
				break;

				case FLAG_RESET:
					port_b_reg_val &= ~(1<<pin);
				break;
			}
			data_buffer[0] = port_b_reg_val;
			op_status |= drv_mcp23017_iic_write_register(device_address,(REG_GPIOA + 1),data_buffer,1);

		break;

	}
	return op_status;
}


/* Thease are basic read and write operation */
kernel_status_type drv_mcp23017_iic_write_register(uint8_t device_address, uint8_t reg_address, uint8_t* data , uint8_t length)
{
	if(length == 1)	// single register write
	{
		iic_pdu_buffer.device_id = MCP23017_COMM;
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
	else
	{
		iic_pdu_buffer.device_id = MCP23017_COMM;
		iic_pdu_buffer.iic_operation = IIC_OPERATION_WRITE;
		iic_pdu_buffer.slave_address = device_address;
		iic_pdu_buffer.txn_data[0] = reg_address;
		iic_pdu_buffer.length = length + 1;
		for(int i = 0; i < length; i++)
		{
			iic_pdu_buffer.txn_data[i+1] =  data[i];
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
	while((drv_time_get_ticks() - set_time_ms) < DRV_PCA9685_ACK_TIMEOUT_MS)
	{
		xQueuePeek( pipe_iic_pdu_rx_handle,
		             &iic_pdu_buffer,
					(TickType_t)0
		           );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == MCP23017_COMM) && \
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

uint8_t drv_mcp23017_iic_read_register(uint8_t device_address, uint8_t reg_address)
{
	iic_pdu_buffer.device_id = MCP23017_COMM;
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

	while((drv_time_get_ticks() - set_time_ms) < DRV_PCA9685_ACK_TIMEOUT_MS)
	{
		xQueuePeek(pipe_iic_pdu_rx_handle,
			         &iic_pdu_buffer,
					 (TickType_t)0
			      );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == MCP23017_COMM) && \
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
#error  "MCP23017 Driver included but bus MCP23017_COMM bus not defined!!"
#endif

#endif


