/*
 * PCA9685_IIC.c
 *
 *  Created on: Sep 26, 2023
 *      Author: subhajit-roy
 */
#include "PCA9685_IIC.h"

#if (INC_DRIVER_PCA9685 == 1)

#ifdef PCA9685_COMM

/* IIC management IPC ref  */
static i2c_pdu_struct_type	iic_pdu_buffer;
static uint64_t				set_time_ms;
static uint8_t				data_buffer[32];




kernel_status_type drv_pca9685_iic_init(uint8_t dev_address)
{
	kernel_status_type op_status = KERNEL_OK;

	/* Initialization */
	data_buffer[0] = 0x80;
	op_status |= drv_pca9685_iic_write_register(dev_address, REG_MODE_0, data_buffer, 1);
	drv_time_delay_ms(100);

	/* Set PWM frequency */
//	op_status |= drv_pca9685_iic_set_pwm_freq(dev_address, 48);

	/* Set the MODE 0 register
	 * AUTO increment enabled */
	op_status |= drv_pca9685_iic_set_bit(dev_address, REG_MODE_0, REG_MODE_1_AI_BIT_POS, 1);

	return op_status;
}



kernel_status_type drv_pca9685_iic_set_bit(uint8_t dev_address, uint8_t reg_address, uint8_t bit_pos, uint8_t val)
{
	kernel_status_type op_status = KERNEL_OK;
	uint8_t	reg_val = 0;
	uint8_t data[2];

	reg_val = drv_pca9685_iic_read_register(dev_address, reg_address);

	reg_val &= ~((1 << REG_MODE_1_RESTART_BIT_POS) | (1 << bit_pos));
	reg_val |= (val & 1) << bit_pos;

	data[0] = reg_val;
	op_status |= drv_pca9685_iic_write_register(dev_address, reg_address, data, 1);

	return op_status;
}


kernel_status_type drv_pca9685_iic_sw_reset(uint8_t dev_address)
{
	kernel_status_type op_status = KERNEL_OK;

	data_buffer[0] = 0x6;
	op_status |= drv_pca9685_iic_write_register(dev_address, REG_MODE_0, data_buffer, 1);

	return op_status;
}


kernel_status_type drv_pca9685_iic_set_pwm_freq(uint8_t dev_address, uint32_t frequency)
{
	kernel_status_type op_status;
	float PrescalerVal;
	uint8_t Prescale;

	if (frequency >= 1526) {
		Prescale = 0x03;
	} else if (frequency <= 24) {
		Prescale = 0xFF;
	} else {
		PrescalerVal = (25000000 / (4096.0 * (float) frequency)) - 1;
		Prescale = floor(PrescalerVal + 0.5);
	}
	op_status = KERNEL_OK;
	/* to set the PWN frequencty the controller has to be in sleep mode */

	/* write the PWM prescaler */
	data_buffer[0] = Prescale;
	op_status |= drv_pca9685_iic_write_register(dev_address, REG_PRESCALE, data_buffer, 1);
	drv_pca9685_iic_set_bit(dev_address, REG_MODE_0, REG_MODE_1_SLEEP_BIT_POS, 1);
	drv_time_delay_ms(100);
	/* Starting normal operation */
	drv_pca9685_iic_set_bit(dev_address, REG_MODE_0, REG_MODE_1_SLEEP_BIT_POS, 0);
	drv_time_delay_ms(100);
	drv_pca9685_iic_set_bit(dev_address, REG_MODE_0, REG_MODE_1_RESTART_BIT_POS, 1);
	drv_time_delay_ms(100);

	return op_status;
}

kernel_status_type drv_pca9685_iic_set_pwm(uint8_t dev_address, uint8_t channel, uint16_t on_time, uint16_t off_time)
{
	kernel_status_type op_status = KERNEL_OK;
	uint8_t reg_address;
	uint8_t data[4];

	reg_address = REG_LED0_ON_L + (4 * channel);

	data[0] = on_time & 0xFF;
	data[1] = (on_time >> 8) & 0x0F;
	data[2] = off_time & 0xFF;
	data[3] = (off_time >> 8) & 0x0F;

	op_status |= drv_pca9685_iic_write_register(dev_address, reg_address, data, 4);

	return op_status;
}

kernel_status_type drv_pca9685_iic_set_pin(uint8_t dev_address, uint8_t channel, uint8_t value , uint8_t invert)
{
	if(invert)
	{
		if(value == 0)
		{
			return drv_pca9685_iic_set_pwm(dev_address,channel,1,0);
		}
		else
		{
			return drv_pca9685_iic_set_pwm(dev_address,channel,0,1);
		}
	}
	else
	{
		if(value == 0)
		{
			return drv_pca9685_iic_set_pwm(dev_address,channel,0,1);
		}
		else
		{
			return drv_pca9685_iic_set_pwm(dev_address,channel,1,0);
		}
	}
}


/* Thease are basic read and write operation */
kernel_status_type drv_pca9685_iic_write_register(uint8_t device_address, uint8_t reg_address, uint8_t* data , uint8_t length)
{
	if(length == 1)	// single register write
	{
		iic_pdu_buffer.device_id = PCA9685_COMM;
		iic_pdu_buffer.iic_operation = IIC_OPERATION_REG_WRITE;
		iic_pdu_buffer.slave_address = device_address;
		iic_pdu_buffer.reg_addr = reg_address;
		iic_pdu_buffer.txn_data[0] = data[0];
		/* do a IIC operation */
		xQueueSend( pipe_iic_pdu_tx_handle,
					&iic_pdu_buffer,
					(TickType_t)0
				  );
	}
	else
	{
		iic_pdu_buffer.device_id = PCA9685_COMM;
		iic_pdu_buffer.iic_operation = IIC_OPERATION_WRITE;
		iic_pdu_buffer.slave_address = device_address;
		iic_pdu_buffer.txn_data[0] = reg_address;
		iic_pdu_buffer.length = length + 1;
		for(int i = 0; i < length; i++)
		{
			iic_pdu_buffer.txn_data[i+1] =  data[i];
		}
		/* do a IIC operation */
		xQueueSend( pipe_iic_pdu_tx_handle,
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
		if((iic_pdu_buffer.device_id == PCA9685_COMM) && \
		   (iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive( pipe_iic_pdu_rx_handle,
							&iic_pdu_buffer,
							(TickType_t)0
						  );
		    break;
		}
	}

	return iic_pdu_buffer.op_status;
}

uint8_t drv_pca9685_iic_read_register(uint8_t device_address, uint8_t reg_address)
{
	iic_pdu_buffer.device_id = PCA9685_COMM;
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
		xQueuePeek( pipe_iic_pdu_rx_handle,
			         &iic_pdu_buffer,
					 (TickType_t)0
			      );
		/* If the PDU receive for */
		if((iic_pdu_buffer.device_id == PCA9685_COMM) && \
				(iic_pdu_buffer.slave_address == device_address))
		{
			xQueueReceive( pipe_iic_pdu_rx_handle,
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
#error "PCA9685 Driver included but bus PCA9685_COMM bus not defined!!"
#endif

#endif
