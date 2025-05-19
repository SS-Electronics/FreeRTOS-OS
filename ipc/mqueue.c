/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS OS project
#
# RTOS OS project is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS OS project is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#include <ipc/mqueue.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/
#if (NO_OF_UART > 0)

struct ringbuffer				ipc_handle_uart_1_drv_rx_handle;

#ifndef CONF_IPC_RD_UART_1_RX_SIZE
#error "[ OS ] UART 1 CONF_IPC_RD_UART_1_RX_SIZE not defined...!"
#else
static uint8_t					ipc_handle_uart_1_drv_rx_storage[CONF_IPC_RD_UART_1_RX_SIZE];
#endif

struct ringbuffer				ipc_handle_printk_buffer;
static uint8_t					ipc_handle_printk_buffer_storage[CONF_IPC_PRINTK_BUFFER_SIZE];

#endif


#if (NO_OF_UART > 1)
struct ringbuffer				ipc_handle_uart_2_drv_rx_handle;

#ifndef CONF_IPC_RD_UART_1_RX_SIZE
#error "[ OS ] UART 2 CONF_IPC_RD_UART_2_RX_SIZE not defined...!"
#else
static uint8_t					ipc_handle_uart_2_drv_rx_storage[CONF_IPC_RD_UART_2_RX_SIZE];
#endif

#endif

//#if(PERIPHERAL_CAN_1_EN == FLAG_ENABLE)
//static uint8_t					pipe_can_1_drv_rx_storage[PIPE_CAN_1_DRV_RX_SIZE * sizeof(can_pdu_struct_type)];
//QueueHandle_t					pipe_can_1_drv_rx_handle = NULL;
//static StaticQueue_t 			pipe_can_1_drv_rx;
//#endif
//
//#if(PERIPHERAL_CAN_2_EN == FLAG_ENABLE)
//static uint8_t					pipe_can_2_drv_rx_storage[PIPE_CAN_2_DRV_RX_SIZE * sizeof(can_pdu_struct_type)];
//QueueHandle_t					pipe_can_2_drv_rx_handle = NULL;
//static StaticQueue_t 			pipe_can_2_drv_rx;
//#endif
//
//#if(PERIPHERAL_CAN_3_EN == FLAG_ENABLE)
//static uint8_t					pipe_can_3_drv_rx_storage[PIPE_CAN_3_DRV_RX_SIZE * sizeof(can_pdu_struct_type)];
//QueueHandle_t					pipe_can_3_drv_rx_handle = NULL;
//static StaticQueue_t 			pipe_can_3_drv_rx;
//#endif
//
//#if(PERIPHERAL_USB_1_EN == FLAG_SET)
//struct ringbuffer				pipe_usb_1_drv_rx_handle;
//static uint8_t					pipe_usb_1_drv_rx_storage[PIPE_USB_1_DRV_RX_SIZE];
//#endif


/* *****************************************************
 *
 *
 *
 * *****************************************************/
//#if(INC_SERVICE_CAN_MGMT == FLAG_SET)
//static uint8_t					pipe_can_app_tx_storage[PIPE_CAN_APP_TX_SIZE * sizeof(can_pdu_struct_type)];
//QueueHandle_t					pipe_can_app_tx_handle = NULL;
//static StaticQueue_t 			pipe_can_app_tx;
//
//static uint8_t					pipe_can_app_rx_storage[PIPE_CAN_APP_RX_SIZE * sizeof(can_pdu_struct_type)];
//QueueHandle_t					pipe_can_app_rx_handle = NULL;
//static StaticQueue_t 			pipe_can_app_rx;
//#endif
//
//
//#if(INC_SERVICE_IIC_MGMT == FLAG_SET)
//static uint8_t					pipe_iic_pdu_tx_storage[PIPE_IIC_PDU_TX_SIZE * sizeof(i2c_pdu_struct_type)];
//QueueHandle_t					pipe_iic_pdu_tx_handle = NULL;
//static StaticQueue_t 			pipe_iic_pdu_tx;
//
//static uint8_t					pipe_iic_pdu_rx_storage[PIPE_IIC_PDU_RX_SIZE * sizeof(i2c_pdu_struct_type)];
//QueueHandle_t					pipe_iic_pdu_rx_handle = NULL;
//static StaticQueue_t 			pipe_iic_pdu_rx;
//#endif
//
///* Diagnostic Queue init*/
//static uint8_t					pipe_diagnostics_storage[PIPE_DIAGNOSTICS_SIZE * sizeof(diagnostics_type)];
//QueueHandle_t					pipe_diagnostics_handle = NULL;
//static StaticQueue_t 			pipe_diagnostics;


/* *****************************************************
 *
 *
 *
 * *****************************************************/
status_type 	ipc_mqueue_init(void)
{
	type_kernel_status status = ERROR_NONE;


#if(NO_OF_UART > 0)
	ringbuffer_init(&ipc_handle_uart_1_drv_rx_handle, ipc_handle_uart_1_drv_rx_storage, CONF_IPC_RD_UART_1_RX_SIZE);
	ringbuffer_init(&ipc_handle_printk_buffer, ipc_handle_printk_buffer_storage, CONF_IPC_PRINTK_BUFFER_SIZE);
#endif

#if(NO_OF_UART > 1)
	ringbuffer_init(&ipc_handle_uart_2_drv_rx_handle, ipc_handle_uart_2_drv_rx_storage, CONF_IPC_RD_UART_2_RX_SIZE);
#endif
	/* USB DRIVER 1 ISR to canopen read funciton linkage */
//#if(PERIPHERAL_USB_1_EN == FLAG_ENABLE)
//	ringbuffer_init(&pipe_usb_1_drv_rx_handle, pipe_usb_1_drv_rx_storage, PIPE_USB_1_DRV_RX_SIZE);
//#endif


//
//	/* CAN DRIVER 1 ISR to canopen read funciton linkage */
//#if(PERIPHERAL_CAN_1_EN == FLAG_ENABLE)
//	pipe_can_1_drv_rx_handle 		= 	xQueueCreateStatic( PIPE_CAN_1_DRV_RX_SIZE,
//	                                 	 	 	 	  sizeof(can_pdu_struct_type),
//													  pipe_can_1_drv_rx_storage,
//													  &pipe_can_1_drv_rx );
//	if(pipe_can_1_drv_rx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//	}
//#endif
//	/* CAN DRIVER 2 ISR to canopen read funciton linkage */
//#if(PERIPHERAL_CAN_2_EN == FLAG_ENABLE)
//	pipe_can_2_drv_rx_handle 	= 	xQueueCreateStatic( PIPE_CAN_2_DRV_RX_SIZE,
//	                                 	 	 	 	  sizeof(can_pdu_struct_type),
//													  pipe_can_2_drv_rx_storage,
//													  &pipe_can_2_drv_rx );
//	if(pipe_can_2_drv_rx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//#endif
//	/* CAN DRIVER 3 ISR to canopen read funciton linkage */
//#if(PERIPHERAL_CAN_3_EN == FLAG_ENABLE)
//	pipe_can_3_drv_rx_handle 		= 	xQueueCreateStatic( PIPE_CAN_3_DRV_RX_SIZE,
//	                                 	 	 	 	  sizeof(can_pdu_struct_type),
//													  pipe_can_3_drv_rx_storage,
//													  &pipe_can_3_drv_rx );
//	if(pipe_can_3_drv_rx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//#endif




//#if(INC_SERVICE_CAN_MGMT == 1)
//	/* CAN Application level tx */
//	pipe_can_app_tx_handle 		= 	xQueueCreateStatic( PIPE_CAN_APP_TX_SIZE,
//	                                 	 	 	 	  sizeof(can_pdu_struct_type),
//													  pipe_can_app_tx_storage,
//													  &pipe_can_app_tx );
//	if(pipe_can_app_tx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//
//	/* CAN Application level rx */
//	pipe_can_app_rx_handle 		= 	xQueueCreateStatic( PIPE_CAN_APP_RX_SIZE,
//	                                 	 	 	 	  sizeof(can_pdu_struct_type),
//													  pipe_can_app_rx_storage,
//													  &pipe_can_app_rx );
//	if(pipe_can_app_rx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//#endif




//#if(INC_SERVICE_IIC_MGMT == FLAG_SET)
//	/* IIC driver and service related pipes */
//	pipe_iic_pdu_tx_handle 		= 	xQueueCreateStatic( PIPE_IIC_PDU_TX_SIZE,
//	                                 	 	 	 	  sizeof(i2c_pdu_struct_type),
//													  pipe_iic_pdu_tx_storage,
//													  &pipe_iic_pdu_tx );
//	if(pipe_iic_pdu_tx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//
//	pipe_iic_pdu_rx_handle 		= 	xQueueCreateStatic( PIPE_IIC_PDU_RX_SIZE,
//	                                 	 	 	 	  sizeof(i2c_pdu_struct_type),
//													  pipe_iic_pdu_rx_storage,
//													  &pipe_iic_pdu_rx );
//	if(pipe_iic_pdu_rx_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}
//#endif
//
//	/*Diagnostic Queue*/
//	pipe_diagnostics_handle 		= 	xQueueCreateStatic( PIPE_DIAGNOSTICS_SIZE,
//														  sizeof(diagnostics_type),
//														  pipe_diagnostics_storage,
//														  &pipe_diagnostics );
//	if(pipe_diagnostics_handle == NULL)
//	{
//		status |= KERNEL_ERR;
//
//	}


	return status;
}



