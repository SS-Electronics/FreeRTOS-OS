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

#include "drv_spi.h"
#if (INC_DRIVER_COMM_IIC == 1)

static drv_spi_handle_type 	spi_handle;
static uint8_t 			   	print_buffer[PRINTK_BUFF_LENGTH];
static uint8_t			   	spi_dev_tx_buffer[32];
static uint8_t				spi_dev_rx_buffer[32];




/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_spi_handle_type* drv_spi_get_handle(void)
{
	return &spi_handle;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type drv_spi_init(uint8_t device_id)
{
	drv_status_type status = KERNEL_OK;

	status |= HAL_SPI_Init(spi_handle.handle[device_id]);

	/* Keep the CS pin high */
	drv_gpio_set_pin(PORT_SPI_CS, PIN_SPI_CS);
	return status;
}

/*************************************************************
 * Func: Transmit operation
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type		drv_spi_transmit(spi_pdu_struct_type * tx_pdu)
{
	drv_status_type status = KERNEL_OK;

	for( int i = 0; i < tx_pdu->length ; i++ )
	{
		spi_dev_tx_buffer[i] = tx_pdu->tx_data[i];
	}

	/* Keet the CS LOW */
	drv_gpio_clear_pin(PORT_SPI_CS, PIN_SPI_CS);

	status |= HAL_SPI_Transmit(spi_handle.handle[tx_pdu->device_id],
								spi_dev_tx_buffer, tx_pdu->length, 1000);

	/* keep the CS HIGH */
	drv_gpio_set_pin(PORT_SPI_CS, PIN_SPI_CS);

#if(DRV_DETAIL_DEBUG_EN == 1)
	if(status != KERNEL_OK)
	{
		sprintf((char*)print_buffer, "[DRV: SPI-> %d ] RD OP Failed!\n\r",tx_pdu->device_id);
		printk(print_buffer);
	}
#endif


	return status;
}

/*************************************************************
 * Func: Receive operation
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type		drv_spi_receive(spi_pdu_struct_type * rx_pdu)
{
	drv_status_type status = KERNEL_OK;

	for( int i = 0; i < rx_pdu->length ; i++ )
	{
		spi_dev_rx_buffer[i] = 0;
	}

	/* Keet the CS LOW */
	drv_gpio_clear_pin(PORT_SPI_CS, PIN_SPI_CS);

	status |= HAL_SPI_Receive(spi_handle.handle[rx_pdu->device_id],
							  spi_dev_rx_buffer, rx_pdu->length, 1000);

	for( int i = 0; i < rx_pdu->length ; i++ )
	{
		rx_pdu->rx_data[i] = spi_dev_rx_buffer[i];
	}

	/* keep the CS HIGH */
	drv_gpio_set_pin(PORT_SPI_CS, PIN_SPI_CS);

#if(DRV_DETAIL_DEBUG_EN == 1)
	if(status != KERNEL_OK)
	{
		sprintf((char*)print_buffer, "[DRV: SPI-> %d ] RD OP Failed!\n\r",rx_pdu->device_id);
		printk(print_buffer);
	}
#endif
	return status;
}

/*************************************************************
 * Func: Transaction both tx + rx
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type	drv_spi_transfer(spi_pdu_struct_type * txn_pdu)
{
	drv_status_type status = KERNEL_OK;



	for( int i = 0; i < txn_pdu->length ; i++ )
	{
		spi_dev_tx_buffer[i] = txn_pdu->tx_data[i];
		spi_dev_rx_buffer[i] = 0;
	}

	/* Keet the CS LOW */
	drv_gpio_clear_pin(PORT_SPI_CS, PIN_SPI_CS);

	status |= HAL_SPI_TransmitReceive(spi_handle.handle[txn_pdu->device_id],
									  spi_dev_tx_buffer,
									  spi_dev_rx_buffer,
									  txn_pdu->length, 1000);


	for( int i = 0; i < txn_pdu->length ; i++ )
	{
		txn_pdu->rx_data[i] = spi_dev_rx_buffer[i];
	}

	/* keep the CS HIGH */
	drv_gpio_set_pin(PORT_SPI_CS, PIN_SPI_CS);

#if(DRV_DETAIL_DEBUG_EN == 1)
	if(status != KERNEL_OK)
	{
		sprintf((char*)print_buffer, "[DRV: SPI-> %d ] TXN OP Failed!\n\r",txn_pdu->device_id);
		printk(print_buffer);
	}
#endif
	return status;
}



#endif
