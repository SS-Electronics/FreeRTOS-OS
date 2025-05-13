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

#include "drv_can.h"
#if (INC_DRIVER_COMM_CAN == 1)

/* Serial driver handle */
static drv_can_handle_type 		can_handle;
/*IPC related  variables */
can_pdu_struct_type 			can_rx_pdu_buffer;
static BaseType_t 				pxHigherPriorityTaskWoken_1;
/* Local bufferers for check the CAN */
uint8_t 						can_rx_buffer[8];
uint8_t 						can_tx_buffer[8];
uint32_t 						count = 0; // for diagnostics

/* *************** [ Driver related HAL Layer variables  ] *********]
 * This will change MCU to MCU
 *
 * ****************************************************
 *  */

/* Variant handling */
#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
FDCAN_RxHeaderTypeDef 	can_rx_header_buffer;
FDCAN_TxHeaderTypeDef 	can_tx_header_buffer;
FDCAN_FilterTypeDef 	sFilterConfig_2;
FDCAN_FilterTypeDef 	can_network_filter;
#endif

#if defined(STM32F429xx) || defined(STM32L431xx) || defined(STM32F765xx)
CAN_TxHeaderTypeDef 	can_tx_header_buffer;
CAN_RxHeaderTypeDef 	can_rx_header_buffer;
CAN_FilterTypeDef 		can_network_filter;
CAN_FilterTypeDef 		sFilterConfig_2;
#endif


/* *************** [ CAN_OPEN related variables  ] *********]
 * This will change MCU to MCU
 *
 * ***************************************************
 *  */
#if(PERIPHERAL_CAN_1_EN == 1)
// Used by driver level by the CAN OPEN MODULE.
static void    co_drv_can_init_1 	(void);
static void    co_drv_can_enable_1 	(uint32_t baudrate);
static int16_t co_drv_can_send_1   	(CO_IF_FRM *frm);
static int16_t co_drv_can_read_1   	(CO_IF_FRM *frm);
static void    co_drv_can_reset_1  	(void);
static void    co_drv_can_close_1  	(void);

const CO_IF_CAN_DRV co_drv_can_1 =
{
	co_drv_can_init_1,
    co_drv_can_enable_1,
    co_drv_can_read_1,
    co_drv_can_send_1,
    co_drv_can_reset_1,
    co_drv_can_close_1
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_init_1(void)
{
	/* TODO: Need to implement fromCO driver level
	 * */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_enable_1(uint32_t baudrate)
{
    (void)baudrate;

    /* TODO: set the given baudrate to the CAN controller */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_send_1(CO_IF_FRM *frm)
{
	can_pdu_struct_type local_tx_buff_data;
	local_tx_buff_data.m_id = frm->Identifier;
	local_tx_buff_data.dlc = frm->DLC;
	for (int  i = 0 ; i< local_tx_buff_data.dlc; i++)
	{
		local_tx_buff_data.data[i] = frm->Data[i];
	}

	if (drv_can_transmit(CAN_1, &local_tx_buff_data)== DRV_ERR){
		count = count + 1;
		syscall_diagnostics_log(ERR_1002_CAN1_DRV_TX, count);
	}

//    /* wait for free CAN message slot and send the given CAN frame */
//    return (int16_t)HAL_FDCAN_GetTxFifoFreeLevel(can_handle.handle[CAN_1]);
	return 2;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_read_1 (CO_IF_FRM *frm)
{
    (void)frm;
    can_pdu_struct_type local_rx_buff_data;
    if(xQueueReceive(pipe_can_1_drv_rx_handle, &local_rx_buff_data, (TickType_t)0) == pdTRUE)
    {
    	frm->Identifier = local_rx_buff_data.m_id;
    	frm->DLC = local_rx_buff_data.dlc;
    	for (int  i = 0 ; i< local_rx_buff_data.dlc; i++)
    	{
    		frm->Data[i] = local_rx_buff_data.data[i];
    	}
    	return sizeof(CO_IF_FRM);
    }
    return (0u);
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_reset_1(void)
{
    /* TODO: reset CAN controller while keeping baudrate */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_close_1(void)
{
    /* TODO: remove CAN controller from CAN network */
}

#endif // END of #if(PERIPHERAL_CAN_1_EN == 1)


#if(PERIPHERAL_CAN_2_EN == 1)

static void    co_drv_can_init_2 	(void);
static void    co_drv_can_enable_2 	(uint32_t baudrate);
static int16_t co_drv_can_send_2   	(CO_IF_FRM *frm);
static int16_t co_drv_can_read_2   	(CO_IF_FRM *frm);
static void    co_drv_can_reset_2  	(void);
static void    co_drv_can_close_2  	(void);

const CO_IF_CAN_DRV co_drv_can_2 =
{
	co_drv_can_init_2,
	co_drv_can_enable_2,
    co_drv_can_read_2,
    co_drv_can_send_2,
	co_drv_can_reset_2,
	co_drv_can_close_2
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_init_2(void)
{

}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_enable_2(uint32_t baudrate)
{
    (void)baudrate;

    /* TODO: set the given baudrate to the CAN controller */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_send_2(CO_IF_FRM *frm)
{
	can_pdu_struct_type local_tx_buff_data;
	local_tx_buff_data.m_id = frm->Identifier;
	local_tx_buff_data.dlc = frm->DLC;
	for (int  i = 0 ; i< local_tx_buff_data.dlc; i++)
	{
		local_tx_buff_data.data[i] = frm->Data[i];
	}

	if (drv_can_transmit(CAN_2, &local_tx_buff_data)== DRV_ERR)
	{
		count = count + 1;
		syscall_diagnostics_log(ERR_1027_CAN2_DRV_TX, count);

	}

	/* wait for free CAN message slot and send the given CAN frame */
	return 2;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_read_2 (CO_IF_FRM *frm)
{
    (void)frm;
    can_pdu_struct_type local_rx_buff_data;
    if(xQueueReceive(pipe_can_2_drv_rx_handle, &local_rx_buff_data, (TickType_t)0) == pdTRUE)
    {
    	frm->Identifier = local_rx_buff_data.m_id;
    	frm->DLC = local_rx_buff_data.dlc;
    	for (int  i = 0 ; i< local_rx_buff_data.dlc; i++)
    	{
    		frm->Data[i] = local_rx_buff_data.data[i];
    	}
    	return sizeof(CO_IF_FRM);
    }
    return (0u);
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_reset_2(void)
{
    /* TODO: reset CAN controller while keeping baudrate */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_close_2(void)
{
    /* TODO: remove CAN controller from CAN network */
}

#endif // END of #if(PERIPHERAL_CAN_2_EN == 1)


#if(PERIPHERAL_CAN_3_EN == 1)

static void    co_drv_can_init_3 	(void);
static void    co_drv_can_enable_3 	(uint32_t baudrate);
static int16_t co_drv_can_send_3   	(CO_IF_FRM *frm);
static int16_t co_drv_can_read_3   	(CO_IF_FRM *frm);
static void    co_drv_can_reset_3  	(void);
static void    co_drv_can_close_3  	(void);

const CO_IF_CAN_DRV co_drv_can_3 =
{
	co_drv_can_init_3,
	co_drv_can_enable_3,
    co_drv_can_read_3,
    co_drv_can_send_3,
	co_drv_can_reset_3,
	co_drv_can_close_3
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_init_3(void)
{

}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_enable_3(uint32_t baudrate)
{
    (void)baudrate;

    /* TODO: set the given baudrate to the CAN controller */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_send_3(CO_IF_FRM *frm)
{
	can_pdu_struct_type local_tx_buff_data;
	local_tx_buff_data.m_id = frm->Identifier;
	local_tx_buff_data.dlc = frm->DLC;
	for (int  i = 0 ; i< local_tx_buff_data.dlc; i++)
	{
		local_tx_buff_data.data[i] = frm->Data[i];
	}

	if(drv_can_transmit(CAN_3, &local_tx_buff_data)== DRV_ERR)
	{
		count = count + 1;
		syscall_diagnostics_log(ERR_1052_CAN3_DRV_TX, count);
	}

	/* wait for free CAN message slot and send the given CAN frame */
	return 2;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static int16_t co_drv_can_read_3 (CO_IF_FRM *frm)
{
    (void)frm;
    can_pdu_struct_type local_rx_buff_data;
    if(xQueueReceive(pipe_can_3_drv_rx_handle, &local_rx_buff_data, (TickType_t)0) == pdTRUE)
    {
    	frm->Identifier = local_rx_buff_data.m_id;
    	frm->DLC = local_rx_buff_data.dlc;
    	for (int  i = 0 ; i< local_rx_buff_data.dlc; i++)
    	{
    		frm->Data[i] = local_rx_buff_data.data[i];
    	}
    	return sizeof(CO_IF_FRM);
    }
    return (0u);
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_reset_3(void)
{
    /* TODO: reset CAN controller while keeping baudrate */
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_drv_can_close_3(void)
{
    /* TODO: remove CAN controller from CAN network */
}

#endif // END of #if(PERIPHERAL_CAN_3_EN == 1)


/*************************************************************
 * Func:
 * Desc:Get the driver level handles to boot
 * 		and pass the low level handles.
 *
 * @parm: None
 * @ret: pointer to can driver handle
 *
 ************************************************************/
drv_can_handle_type * drv_can_get_handle(void)
{
	return &can_handle;
}

/* ********************************************
 *
 * Driver Level Abstraction
 *
 * ********************************************
 * */

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type drv_can_init(uint8_t device_id)
{
	drv_status_type drv_status = DRV_OK;

/* --------------------------------------------
 * STM32H7A3xxQ, STM32H743xx
 * --------------------------------------------
 * */
#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
	/* Data initialization */
	can_tx_header_buffer.IdType = FDCAN_STANDARD_ID;
	can_tx_header_buffer.TxFrameType = FDCAN_DATA_FRAME;
	can_tx_header_buffer.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	can_tx_header_buffer.BitRateSwitch = FDCAN_BRS_OFF;
	can_tx_header_buffer.FDFormat = FDCAN_CLASSIC_CAN;
	can_tx_header_buffer.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	can_tx_header_buffer.MessageMarker = 0x51;
#endif
/* --------------------------------------------
 * STM32F429xx | STM32L431xx
 * --------------------------------------------
 * */
#if defined(STM32F429xx)  || defined(STM32L431xx) || defined(STM32F765xx)
	can_tx_header_buffer.IDE = CAN_ID_STD;
#endif

	if(can_handle.handle[device_id] != NULL)
	{
/* --------------------------------------------
 * STM32H7A3xxQ, STM32H743xx
 * --------------------------------------------
 * */
#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
#if (CAN_FILTER_FEATURE_EN == 1)

		can_network_filter.IdType 		= FDCAN_STANDARD_ID;
		can_network_filter.FilterType	= FDCAN_FILTER_MASK;

		for(int j = 0; j< NO_OF_FILTER_CONFIGURATION ; j++)
		{
			if(global_can_filter_config[j].can_dev_id == device_id) // Checking for CAN peripheral
			{
				can_network_filter.FilterIndex 	= global_can_filter_config[j].filter_config_index;
				can_network_filter.FilterID1	= global_can_filter_config[j].filter_id;
				can_network_filter.FilterID2    = global_can_filter_config[j].filter_mask;
				can_network_filter.FilterConfig = global_can_filter_config[j].filter_rx;

				/* do the config filter operation */
				drv_status |= HAL_FDCAN_ConfigFilter(can_handle.handle[device_id], &can_network_filter);
			}
		}
#else
		/* Some driver level init  */
		sFilterConfig_2.IdType = FDCAN_STANDARD_ID;
		sFilterConfig_2.FilterIndex = 0;
		sFilterConfig_2.FilterType = FDCAN_FILTER_RANGE;
		sFilterConfig_2.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
		sFilterConfig_2.FilterID1 = 0x00;
		sFilterConfig_2.FilterID2 = 0x7FF; /* For acceptance, MessageID and FilterID1 must match exactly */
		drv_status |= HAL_FDCAN_ConfigFilter(can_handle.handle[device_id], &sFilterConfig_2);
#endif
		/* Configure global filter to reject all non-matching frames */
		drv_status |= HAL_FDCAN_ConfigGlobalFilter(can_handle.handle[device_id], FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
		/* Configure Rx FIFO 1 watermark to 2 */
		drv_status |= HAL_FDCAN_ConfigFifoWatermark(can_handle.handle[device_id], FDCAN_CFG_RX_FIFO1, 5);
		/* Activate Rx FIFO 1 watermark notification */
		drv_status |= HAL_FDCAN_ActivateNotification(can_handle.handle[device_id], FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
		/* Configure Rx FIFO 0 watermark to 2 */
		drv_status |= HAL_FDCAN_ConfigFifoWatermark(can_handle.handle[device_id], FDCAN_CFG_RX_FIFO0, 5);
		/* Activate Rx FIFO 0 watermark notification */
		drv_status |= HAL_FDCAN_ActivateNotification(can_handle.handle[device_id], FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
		/* Start the FDCAN module */
		drv_status |= HAL_FDCAN_Start(can_handle.handle[device_id]);
#endif //defined (STM32H7A3xxQ) || defined (STM32H743xx)
/* --------------------------------------------
 * STM32F429xx | STM32L431xx
 * --------------------------------------------
 * */
#if defined (STM32F429xx)  || defined(STM32L431xx) || defined(STM32F765xx)
#if (CAN_FILTER_FEATURE_EN == 1)

		can_network_filter.FilterActivation = CAN_FILTER_ENABLE;
		can_network_filter.FilterScale		= CAN_FILTERSCALE_32BIT;
		can_network_filter.FilterMode 		= CAN_FILTERMODE_IDMASK;

		for(int j = 0; j< NO_OF_FILTER_CONFIGURATION ; j++)
		{
			if(global_can_filter_config[device_id].can_dev_id == device_id) // Checking for CAN peripheral
			{
				can_network_filter.FilterBank 			= global_can_filter_config[j].filter_config_index;
				can_network_filter.FilterIdHigh			= (global_can_filter_config[j].filter_id << 5);
				can_network_filter.FilterIdLow			= 0x0000;
				can_network_filter.FilterMaskIdHigh 	= (global_can_filter_config[j].filter_mask << 5);
				can_network_filter.FilterMaskIdLow		= 0x0000;
				can_network_filter.FilterFIFOAssignment = global_can_filter_config[j].filter_rx;

				/* do the config filter operation */
				drv_status |= HAL_CAN_ConfigFilter(can_handle.handle[device_id], &can_network_filter);

				if(global_can_filter_config[j].filter_rx == CAN_FILTER_FIFO1)
				{
					drv_status |= HAL_CAN_ActivateNotification(can_handle.handle[device_id],CAN_IT_RX_FIFO1_MSG_PENDING);
				}
				else if(global_can_filter_config[j].filter_rx == CAN_FILTER_FIFO0)
				{
					drv_status |= HAL_CAN_ActivateNotification(can_handle.handle[device_id],CAN_IT_RX_FIFO0_MSG_PENDING);
				}
				else
				{

				}
			}
		}
#else
		can_network_filter.FilterActivation = CAN_FILTER_DISABLE;
		can_network_filter.FilterScale	= CAN_FILTERSCALE_32BIT;
		can_network_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
		can_network_filter.FilterMode = CAN_FILTERMODE_IDMASK;

		can_network_filter.FilterIdLow = 0;
		can_network_filter.FilterIdHigh = 0;
		can_network_filter.FilterMaskIdHigh = 0;
		can_network_filter.FilterMaskIdLow = 0;
		can_network_filter.FilterActivation = CAN_FILTER_ENABLE;


		switch(device_id)
		{
			case CAN_1:
				can_network_filter.FilterBank = 10;
				can_network_filter.SlaveStartFilterBank = 10;
			break;

			case CAN_2:
				can_network_filter.FilterBank = 11;
				can_network_filter.SlaveStartFilterBank = 11;
			break;

			case CAN_3:
				can_network_filter.FilterBank = 9;
				can_network_filter.SlaveStartFilterBank = 9;
			break;

			default:
				can_network_filter.FilterBank = 0;
			break;
		}


		/* do the config filter operation */
		drv_status |= HAL_CAN_ConfigFilter(can_handle.handle[device_id], &can_network_filter);
		drv_status |= HAL_CAN_ActivateNotification(can_handle.handle[device_id], CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING);

#endif /* Filter feature end */
		/* Reset all counters */
		global_drv_status.drv_can[device_id].rx_message_counter = 0;
		global_drv_status.drv_can[device_id].tx_message_counter = 0;
		drv_status |= HAL_CAN_Start(can_handle.handle[device_id]);
#endif //defined (STM32F429xx)

	}/* End of if statement */
	else
	{
		drv_status |= DRV_ERR;
	}

	return drv_status;
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type drv_can_get_tx_buffer_pending_status(uint8_t dev_id)
{
#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
	return HAL_FDCAN_IsTxBufferMessagePending(can_handle.handle[dev_id], FDCAN_TX_BUFFER1);
#endif

	return 0;
}


drv_status_type drv_can_get_protocol_status(uint8_t dev_id)
{
	drv_status_type can_tx_status = DRV_OK;

#if defined(STM32F765xx) || (STM32H7A3xxQ) || defined (STM32H743xx)
	FDCAN_ProtocolStatusTypeDef protocol_status;

	if(HAL_FDCAN_GetProtocolStatus(can_handle.handle[dev_id],&protocol_status) == 0){
		if(protocol_status.BusOff == 1){
			count = count + 1;
			if (dev_id == CAN_1){
				syscall_diagnostics_log(ERR_1000_CAN1_BUSOFF, count);
			}
			else if (dev_id == CAN_2){
				syscall_diagnostics_log(ERR_1025_CAN2_BUSOFF, count);
			}
			else if (dev_id == CAN_3){
				syscall_diagnostics_log(ERR_1050_CAN3_BUSOFF, count);
			}
			else{
				syscall_diagnostics_log(ERR_1075_CAN4_BUSOFF, count);
			}
		}
		if(protocol_status.ErrorPassive == 1){
			count = count + 1;
			if (dev_id == CAN_1){
				syscall_diagnostics_log(ERR_1001_CAN1_PASSIVE, count);
			}
			else if (dev_id == CAN_2){
				syscall_diagnostics_log(ERR_1026_CAN2_PASSIVE, count);
			}
			else if (dev_id == CAN_3){
				syscall_diagnostics_log(ERR_1051_CAN3_PASSIVE, count);
			}
			else{
				syscall_diagnostics_log(ERR_1076_CAN4_PASSIVE, count);
			}
		}

	}
	else{
		can_tx_status = DRV_ERR;
	}
#endif

	return can_tx_status;
}


/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type drv_can_transmit(uint8_t dev_id, can_pdu_struct_type * tx_pdu_ref)
{
	drv_status_type can_tx_status = DRV_OK;
/* --------------------------------------------
 * STM32H7A3xxQ, STM32H743xx
 * --------------------------------------------
 * */
#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
	/* Implementation 1: Checking if any pending messages are there on Tx Buffer
	 * and adding the messages to TxBuffer and enabling request for Tx */
	if(HAL_FDCAN_IsTxBufferMessagePending(can_handle.handle[dev_id], FDCAN_TX_BUFFER1) == 0)
	{
		can_tx_header_buffer.Identifier = tx_pdu_ref->m_id;
		switch(tx_pdu_ref->dlc)
		{
			case 1:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_1;
			break;
			case 2:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_2;
			break;
			case 3:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_3;
			break;
			case 4:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_4;
			break;
			case 5:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_5;
			break;
			case 6:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_6;
			break;
			case 7:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_7;
			break;
			case 8:
				can_tx_header_buffer.DataLength = FDCAN_DLC_BYTES_8;
			break;
		}

		can_tx_status |= HAL_FDCAN_AddMessageToTxBuffer(can_handle.handle[dev_id],
										&can_tx_header_buffer,
										tx_pdu_ref->data,
										FDCAN_TX_BUFFER1);

		can_tx_status |= HAL_FDCAN_EnableTxBufferRequest(can_handle.handle[dev_id], FDCAN_TX_BUFFER1);
	}
	else
	{
		can_tx_status = DRV_ERR;
		count = count +1;
		if (tx_pdu_ref->can_dev_id == CAN_1){
			syscall_diagnostics_log(ERR_1002_CAN1_DRV_TX, count);
		}

		else if (tx_pdu_ref->can_dev_id == CAN_2){
			syscall_diagnostics_log(ERR_1027_CAN2_DRV_TX, count);
		}

		else if (tx_pdu_ref->can_dev_id == CAN_3){
			syscall_diagnostics_log(ERR_1052_CAN3_DRV_TX, count);
		}
		else{
			syscall_diagnostics_log(ERR_1077_CAN4_DRV_TX, count);
		}


	}
#endif
/* --------------------------------------------
 * STM32F429xx | STM32L431xx
 * --------------------------------------------
 * */
#if defined (STM32F429xx) || defined(STM32L431xx) || defined(STM32F765xx)

	uint32_t mail_box_0 = CAN_TX_MAILBOX0;
	uint32_t mail_box_1 = CAN_TX_MAILBOX1;
	uint32_t mail_box_2 = CAN_TX_MAILBOX2;

	can_tx_header_buffer.StdId 	= tx_pdu_ref->m_id;
	can_tx_header_buffer.DLC	= tx_pdu_ref->dlc;
	can_tx_header_buffer.IDE 	= CAN_ID_STD;
	can_tx_header_buffer.RTR 	= CAN_RTR_DATA;
	can_tx_header_buffer.TransmitGlobalTime = DISABLE;

	/* Check the mail box 0 free level */
	if(HAL_CAN_IsTxMessagePending(can_handle.handle[dev_id], mail_box_0) == 0 )
	{
		can_tx_status |= HAL_CAN_AddTxMessage(can_handle.handle[dev_id],
												&can_tx_header_buffer,
												tx_pdu_ref->data,
												&mail_box_0
											 );
	}
	else if(HAL_CAN_IsTxMessagePending(can_handle.handle[dev_id], mail_box_1) == 0 )
	{
		can_tx_status |= HAL_CAN_AddTxMessage(can_handle.handle[dev_id],
												&can_tx_header_buffer,
												tx_pdu_ref->data,
												&mail_box_1
											  );
	}
	else if(HAL_CAN_IsTxMessagePending(can_handle.handle[dev_id], mail_box_2) == 0)
	{
		can_tx_status |= HAL_CAN_AddTxMessage(can_handle.handle[dev_id],
												&can_tx_header_buffer,
												tx_pdu_ref->data,
												&mail_box_2
											  );
	}
	else
	{
		can_tx_status |= DRV_ERR;
		count = count +1;
		if (tx_pdu_ref->can_dev_id == CAN_1){
			syscall_diagnostics_log(1002, count);
		}

		else if (tx_pdu_ref->can_dev_id == CAN_2){
			syscall_diagnostics_log(1027, count);
		}

		else{
			syscall_diagnostics_log(1052, count);
		}
	}

#endif
	return can_tx_status;
}

/* ********************************************
 *
 *CALL Backs from HAL Layer
 *
 * ********************************************
 * */

#if defined (STM32H7A3xxQ) || defined (STM32H743xx)
/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
  if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &can_rx_header_buffer, can_rx_buffer) == HAL_OK)
    {

    	can_rx_pdu_buffer.rx_buffer_id = CAN_RX_FIFO_1;
    	can_rx_pdu_buffer.m_id = can_rx_header_buffer.Identifier;

    	switch(can_rx_header_buffer.DataLength)
    	{
			case FDCAN_DLC_BYTES_1:
				can_rx_pdu_buffer.dlc = 1;
			break;
			case FDCAN_DLC_BYTES_2:
				can_rx_pdu_buffer.dlc = 2;
			break;
			case FDCAN_DLC_BYTES_3:
				can_rx_pdu_buffer.dlc = 3;
			break;
			case FDCAN_DLC_BYTES_4:
				can_rx_pdu_buffer.dlc = 4;
			break;
			case FDCAN_DLC_BYTES_5:
				can_rx_pdu_buffer.dlc = 5;
			break;
			case FDCAN_DLC_BYTES_6:
				can_rx_pdu_buffer.dlc = 6;
			break;
			case FDCAN_DLC_BYTES_7:
				can_rx_pdu_buffer.dlc = 7;
			break;
			case FDCAN_DLC_BYTES_8:
				can_rx_pdu_buffer.dlc = 8;
			break;

			default:
				can_rx_pdu_buffer.dlc = 0;
			break;
    	}

    	for(int i = 0; ((i < can_rx_pdu_buffer.dlc) && (i < 8)); i++)
    	{
    		can_rx_pdu_buffer.data[i] = can_rx_buffer[i];
    	}


    	/* Assign the device ID according to the handler definition */
    	/* Go through all the peripherals and compare handles */
    	if(hfdcan == can_handle.handle[CAN_1])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_1;

#if(PERIPHERAL_CAN_1_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_1_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	     );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hfdcan == can_handle.handle[CAN_2])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_2;

#if(PERIPHERAL_CAN_2_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_2_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	   	  );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hfdcan == can_handle.handle[CAN_2])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_2;

#if(PERIPHERAL_CAN_3_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_3_drv_rx_handle,
    				 	 	 &can_rx_pdu_buffer,
							 &pxHigherPriorityTaskWoken_1
    			   	   	   );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else
    	{
    		can_rx_pdu_buffer.can_dev_id = 101;
    	}// END OF if(hfdcan == can_handle.handle[CAN_1])
    }
  }
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &can_rx_header_buffer, can_rx_buffer) == HAL_OK)
    {
    	/* Assign the device ID according to the handler definition */

    	can_rx_pdu_buffer.rx_buffer_id = CAN_RX_FIFO_0;

    	can_rx_pdu_buffer.m_id = can_rx_header_buffer.Identifier;

    	switch(can_rx_header_buffer.DataLength)
    	{
			case FDCAN_DLC_BYTES_1:
				can_rx_pdu_buffer.dlc = 1;
			break;
			case FDCAN_DLC_BYTES_2:
				can_rx_pdu_buffer.dlc = 2;
			break;
			case FDCAN_DLC_BYTES_3:
				can_rx_pdu_buffer.dlc = 3;
			break;
			case FDCAN_DLC_BYTES_4:
				can_rx_pdu_buffer.dlc = 4;
			break;
			case FDCAN_DLC_BYTES_5:
				can_rx_pdu_buffer.dlc = 5;
			break;
			case FDCAN_DLC_BYTES_6:
				can_rx_pdu_buffer.dlc = 6;
			break;
			case FDCAN_DLC_BYTES_7:
				can_rx_pdu_buffer.dlc = 7;
			break;
			case FDCAN_DLC_BYTES_8:
				can_rx_pdu_buffer.dlc = 8;
			break;
			default:
				can_rx_pdu_buffer.dlc = 0;
			break;
    	}

    	for(int i = 0; ((i < can_rx_pdu_buffer.dlc) && (i < 8)); i++)
    	{
    		can_rx_pdu_buffer.data[i] = can_rx_buffer[i];
    	}

    	/* Assign the device ID according to the handler definition */
    	/* Go through all the peripherals and compare handles */
    	if(hfdcan == can_handle.handle[CAN_1])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_1;

#if(PERIPHERAL_CAN_1_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_1_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	     );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hfdcan == can_handle.handle[CAN_2])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_2;

#if(PERIPHERAL_CAN_2_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_2_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	   	  );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hfdcan == can_handle.handle[CAN_2])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_2;

#if(PERIPHERAL_CAN_3_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_3_drv_rx_handle,
    				 	 	 &can_rx_pdu_buffer,
							 &pxHigherPriorityTaskWoken_1
    			   	   	   );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else
    	{
    		can_rx_pdu_buffer.can_dev_id = 101;
    	}// END OF if(hfdcan == can_handle.handle[CAN_1])
    }
  }
}
#endif
/* --------------------------------------------
 * STM32F429xx || STM32L431xx
 * --------------------------------------------
 * */
#if defined (STM32F429xx) || defined(STM32L431xx) || defined(STM32F765xx)

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&can_rx_header_buffer,can_rx_buffer) == HAL_OK)
	{
		can_rx_pdu_buffer.rx_buffer_id 	= CAN_RX_FIFO_0;
		can_rx_pdu_buffer.m_id 			= can_rx_header_buffer.StdId;
		can_rx_pdu_buffer.dlc  			= can_rx_header_buffer.DLC;

		for(int i = 0; ((i < can_rx_pdu_buffer.dlc) && (i < 8)); i++)
		{
			can_rx_pdu_buffer.data[i] = can_rx_buffer[i];
		}
    	/* Assign the device ID according to the handler definition */
    	/* Go through all the peripherals and compare handles */
    	if(hcan == can_handle.handle[CAN_1])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_1;
    		global_drv_status.drv_can[CAN_1].rx_message_counter++;
#if(PERIPHERAL_CAN_1_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_1_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	     );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hcan == can_handle.handle[CAN_2])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_2;
    		global_drv_status.drv_can[CAN_2].rx_message_counter++;
#if(PERIPHERAL_CAN_2_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_2_drv_rx_handle,
    				 	 	  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
    			   	   	   	  );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else if(hcan == can_handle.handle[CAN_3])
    	{
    		can_rx_pdu_buffer.can_dev_id = CAN_3;
    		global_drv_status.drv_can[CAN_3].rx_message_counter++;
#if(PERIPHERAL_CAN_3_EN == 1)
    		/* Send the data to queue */
    		xQueueSendFromISR(pipe_can_3_drv_rx_handle,
    				 	 	 &can_rx_pdu_buffer,
							 &pxHigherPriorityTaskWoken_1
    			   	   	   );

        	if( pxHigherPriorityTaskWoken_1 != pdFAIL )
        	{
        		/* Actual macro used here is port specific. */
        		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
        	}
#endif
    	}
    	else
    	{
    		can_rx_pdu_buffer.can_dev_id = 101;
    	}// END OF if(hfdcan == can_handle.handle[CAN_1])
	}
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO1,&can_rx_header_buffer,can_rx_buffer) == HAL_OK)
	{
		can_rx_pdu_buffer.rx_buffer_id 	= CAN_RX_FIFO_1;
		can_rx_pdu_buffer.m_id 			= can_rx_header_buffer.StdId;
		can_rx_pdu_buffer.dlc  			= can_rx_header_buffer.DLC;

		for(int i = 0; ((i < can_rx_pdu_buffer.dlc) && (i < 8)); i++)
		{
			can_rx_pdu_buffer.data[i] = can_rx_buffer[i];
		}
		/* Assign the device ID according to the handler definition */
		/* Go through all the peripherals and compare handles */
		if(hcan == can_handle.handle[CAN_1])
		{
	    		can_rx_pdu_buffer.can_dev_id = CAN_1;

#if(PERIPHERAL_CAN_1_EN == 1)
			/* Send the data to queue */
			xQueueSendFromISR(pipe_can_1_drv_rx_handle,
							  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
							 );

			if( pxHigherPriorityTaskWoken_1 != pdFAIL )
			{
				/* Actual macro used here is port specific. */
				portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
			}
#endif
		}
		else if(hcan == can_handle.handle[CAN_2])
		{
			can_rx_pdu_buffer.can_dev_id = CAN_2;

#if(PERIPHERAL_CAN_2_EN == 1)
			/* Send the data to queue */
			xQueueSendFromISR(pipe_can_2_drv_rx_handle,
							  &can_rx_pdu_buffer,
							  &pxHigherPriorityTaskWoken_1
							  );

			if( pxHigherPriorityTaskWoken_1 != pdFAIL )
			{
				/* Actual macro used here is port specific. */
				portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
			}
#endif
		}
		else if(hcan == can_handle.handle[CAN_3])
		{
			can_rx_pdu_buffer.can_dev_id = CAN_3;

#if(PERIPHERAL_CAN_3_EN == 1)
			/* Send the data to queue */
			xQueueSendFromISR(pipe_can_3_drv_rx_handle,
							 &can_rx_pdu_buffer,
							 &pxHigherPriorityTaskWoken_1
						   );

			if( pxHigherPriorityTaskWoken_1 != pdFAIL )
			{
				/* Actual macro used here is port specific. */
				portYIELD_FROM_ISR(pxHigherPriorityTaskWoken_1);
			}
#endif
		}
		else
		{
			can_rx_pdu_buffer.can_dev_id = 101;
		}// END OF if(hfdcan == can_handle.handle[CAN_1])
	}
}
#endif





#endif /* End of can driver inclusion */

