/*
 * os_config.h
 *
 *  Created on: Aug 23, 2023
 *      Author: subhajit-roy
 */

#ifndef OS_CONFIG_OS_CONF_OS_CONFIG_H_
#define OS_CONFIG_OS_CONF_OS_CONFIG_H_
/* All OS related configuration */
/* System related configuration */

/* Kernel types */
#define OS_KERNEL_FREERTOS	1


#define OS_KERNEL_SELECT  						OS_KERNEL_FREERTOS

/* ***************************************************
 *
 *    Service Layer Activation
 * 
 * ***************************************************/
#define INC_SERVICE_CAN_MGMT					(0)
#define INC_SERVICE_UART_MGMT					(1)
#define INC_SERVICE_IIC_MGMT					(0)
#define INC_SERVICE_OS_SHELL_MGMT				(1)
#define INC_SERVICE_ETH_MGMT					(0)
#define INC_SERVICE_DIAGNOSTIC_MGMT             (0)



/* ***************************************************
 *
 *    Protocol Stack configuration
 * 
 * ***************************************************/
#define ISOBUS_STACK_EN							(1)
#define CANOPEN_STACK_EN						(0)




/* ***************************************************
 *
 *    Module drivers activation
 * 
 * ***************************************************/
#define INC_DRIVER_PCA9685						(0)
#define INC_DRIVER_MC23017						(0)
#define INC_DRIVER_DS3502						(0)
#define INC_DRIVER_INA230						(0)
#define INC_DRIVER_MCP3427                      (0)
#define INC_DRIVER_M95M02                       (0)
#define INC_DRIVER_MCP4441                      (0)
#define INC_DRIVER_MCP45HVX1                    (0)


/* ***************************************************
 *
 *    Memory Properties
 * 
 * ***************************************************/
/* IPC configuration */
/* Driver IPC configuration */
#define PIPE_USB_1_DRV_RX_SIZE					(4096)
#define PIPE_UART_1_DRV_RX_SIZE					(30)
#define PIPE_UART_1_DRV_TX_SIZE					(2048)
#define PIPE_CAN_1_DRV_RX_SIZE					(128)
#define PIPE_CAN_2_DRV_RX_SIZE					(128)
#define PIPE_CAN_3_DRV_RX_SIZE					(128)
/* Driver IPC application configuration */
#define PIPE_CAN_PDU_TX_SIZE					(20)
#define PIPE_CAN_PDU_RX_SIZE					(100)
#define PIPE_CAN_APP_TX_SIZE					(128)
#define PIPE_CAN_APP_RX_SIZE					(128)
#define PIPE_IIC_PDU_TX_SIZE					(20)
#define PIPE_IIC_PDU_RX_SIZE					(100)
#define PIPE_DIAGNOSTICS_SIZE					(1)
#define ITM_PRINT_BUFF_LENGTH                   (50)






/* ***************************************************
 *
 *    Debug activation
 * 
 * ***************************************************/
/* Debug activation */
#define DRV_DEBUG_EN							(1) // Print fail cases of driver
#define DRV_DETAIL_DEBUG_EN						(0) // All driver level TXN details
#define DEFAULT_DEBUG_EN						(1) // Default UART related debug app level
#define GW_DEBUG_EN								(0) // CAN Gateway debug en/dis
#define ITM_DEBUG_EN							(0) // Debug through ITM




/* ***************************************************
 *
 *   Service properties
 * 
 * ***************************************************/
#define PROC_SERVICE_SERIAL_MGMT_STACK_SIZE  	(512)
#define PROC_SERVICE_SERIAL_MGMT_PRIORITY    	(1)

#define PROC_SERVICE_CAN_MGMT_STACK_SIZE		(1024)
#define PROC_SERVICE_CAN_MGMT_PRIORITY			(1)

#define PROC_SERVICE_IIC_MGMT_STACK_SIZE		(1024)
#define PROC_SERVICE_IIC_MGMT_PRIORITY			(1)

#define PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE  	(1024)
#define PROC_SERVICE_OS_SHELL_MGMT_PRIORITY    	(1)

#define TEST_SUITE_STACK_SIZE					(512)
#define TEST_SUITE_PRIORITY						(1)


/* ***************************************************
 *
 *   Timing related properties
 * 
 * ***************************************************/
#define TIME_OFFSET_SERIAL_MANAGEMENT			(4000)
#define TIME_OFFSET_OS_SHELL_MGMT				(5000)
#define TIME_OFFSET_IIC_MANAGEMENT				(6500)
#define TIME_OFFSET_ETH_MANAGEMENT				(7000)
#define	TIME_OFFSET_CAN_MANAGEMENT				(10000)
#define TIME_OFFSET_TEST_SUITE					(11000)



#define TIMEOUT_IIC_PIPE_OP						(2)
#define IIC_ACK_TIMEOUT_MS						(100)






#endif /* OS_CONFIG_OS_CONF_OS_CONFIG_H_ */
