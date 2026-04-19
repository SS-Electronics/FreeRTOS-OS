/*
 * M95M02_SPI.c
 *
 *  Created on: Sep 30, 2024
 *      Author: subhajitroy005
 */
#include "M95M02_SPI.h"

#if(INC_DRIVER_M95M02 == 1)

#ifdef M95M02_COMM


/*************************************************************
 * Func: Local variables
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static spi_pdu_struct_type spi_txn_buffer;


/*************************************************************
 * Func: Init the SPI eeeprom module
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
kernel_status_type	drv_m92m02_spi_init(uint8_t device_address)
{
	kernel_status_type status = KERNEL_OK;

	spi_txn_buffer.device_id	= M95M02_COMM;
	spi_txn_buffer.length 		= 1; //  Length 2
	spi_txn_buffer.tx_data[0]	= M95M02_CMD_WREN;
	status |= drv_spi_transfer(&spi_txn_buffer);

	return status;
}

/*************************************************************
 * Func: Read the SPI status
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
kernel_status_type	drv_m92m02_spi_get_status(uint8_t device_address, uint8_t * str_ptr)
{
	kernel_status_type status = KERNEL_OK;

//	spi_txn_buffer.device_id	= M95M02_COMM;
//	spi_txn_buffer.length 		= 2; //  Length 2
//	spi_txn_buffer.tx_data[0]	= M95M02_CMD_RDSR;
//	status |= drv_spi_transmit(&spi_txn_buffer);
//
//	spi_txn_buffer.rx_data[0]	= 0;
//	status |= drv_spi_receive(&spi_txn_buffer);



		spi_txn_buffer.device_id	= M95M02_COMM;
		spi_txn_buffer.length 		= 2; //  Length 2
		spi_txn_buffer.tx_data[0]	= M95M02_CMD_RDSR;
		spi_txn_buffer.rx_data[0]	= 0;
		status |= drv_spi_transfer(&spi_txn_buffer);



	*str_ptr = spi_txn_buffer.rx_data[1];

	return status;
}

/*************************************************************
 * Func: read a uint32_t data to the memory address
 * Desc:
 *
 * @parm:
 * @ret:
 *TODO: Need to implemennt the length later. As of now only one byte data
 ************************************************************/
kernel_status_type 		drv_m92m02_spi_write_to_address(uint8_t device_address, uint32_t mem_addr ,
		                                                uint8_t data, uint8_t length)
{
	kernel_status_type status = KERNEL_OK;

	spi_txn_buffer.device_id	= M95M02_COMM;
	spi_txn_buffer.length 		= 1; //  Length 2
	spi_txn_buffer.tx_data[0]	= M95M02_CMD_WREN;
	status |= drv_spi_transfer(&spi_txn_buffer);

	spi_txn_buffer.device_id	= device_address;
	spi_txn_buffer.length 		= 5; //  Length 2
	spi_txn_buffer.tx_data[0]	= M95M02_CMD_WRITE;
	spi_txn_buffer.tx_data[1]	= (uint8_t)((mem_addr >> 16) & 0xFF);
	spi_txn_buffer.tx_data[2]	= (uint8_t)((mem_addr >> 8) & 0xFF);
	spi_txn_buffer.tx_data[3]	= (uint8_t)((mem_addr) & 0xFF);
	spi_txn_buffer.tx_data[4]	= data;
	status |= drv_spi_transfer(&spi_txn_buffer);

	return status;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
  *TODO: Need to implemennt the length later. As of now only one byte data
 ************************************************************/
kernel_status_type 		drv_m92m02_spi_read_from_address(uint8_t device_address, uint32_t mem_addr ,
														 uint8_t * data, uint8_t length )
{
	kernel_status_type status = KERNEL_OK;

	spi_txn_buffer.device_id	= device_address;
	spi_txn_buffer.length 		= 5; //  Length 2
	spi_txn_buffer.tx_data[0]	= M95M02_CMD_READ;
	spi_txn_buffer.tx_data[1]	= (uint8_t)((mem_addr >> 16) & 0xFF);
	spi_txn_buffer.tx_data[2]	= (uint8_t)((mem_addr >> 8) & 0xFF);
	spi_txn_buffer.tx_data[3]	= (uint8_t)((mem_addr) & 0xFF);
	spi_txn_buffer.rx_data[4]	= 0;
	status |= drv_spi_transfer(&spi_txn_buffer);

	*data = spi_txn_buffer.rx_data[4];
	return status;
}

#else
/* SPI comm  not defined so give error */
#error "M95M02_COMM Not defined! Define in board_config.h!"
#endif

#endif
