/*
 * mcu_config.h
 *
 *  Created on: 23-Aug-2023
 *      Author: subhajit-roy
 */

#ifndef OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_
#define OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_




/* ***************************************************
 *
 *     MCU CHIP Vendor Variant
 * 
 * ***************************************************/
#define MCU_VAR_MICROCHIP   1
#define MCU_VAR_STM         2

#define CONFIG_DEVICE_VARIANT               MCU_VAR_MICROCHIP




/* ***************************************************
 *
 *     MCU Peripherals definition 
 *     Based on That Drivers and Services will be activated and configured
 * 
 * ***************************************************/
/* Peripheral Configurations */
#define CONFIG_MCU_WDG_EN					(0)
#define CONFIG_MCU_FLASH_DRV_EN             (0)
#define CONFIG_MCU_NO_OF_UART_PERIPHERAL 	(1)
#define CONFIG_MCU_NO_OF_IIC_PERIPHERAL 	(0)
#define CONFIG_MCU_NO_OF_SPI_PERIPHERAL     (0)
#define CONFIG_MCU_NO_OF_RS485_PERIPHERAL   (0)
#define CONFIG_MCU_NO_OF_CAN_PERIPHERAL 	(0)
#define CONFIG_MCU_NO_OF_TIMER_PERIPHERAL	(0)
#define CONFIG_MCU_NO_OF_USB_PERIPHERAL     (0)
#define CONFIG_MCU_NO_OF_ETH_PERIPHERAL     (0)



/* ***************************************************
 *
 *     Peripheral individual hardeare configuration
 * 
 * ***************************************************/
#define UART_1_EN                           (1)
#define UART_2_EN                           (0)
#define UART_3_EN                           (0)
#define UART_4_EN                           (0)
#define UART_5_EN                           (0)
#define UART_6_EN                           (0)




#define CAN_FILTER_FEATURE_EN				(0)
#define CAN_EXT_ID_EN                       (0)


/* ***************************************************
 *
 *     Peripheral identification macros
 * 
 * ***************************************************/
typedef enum
{
	UART_1 = 0,
	UART_2,
	UART_3,
	UART_4,
	UART_5,
	UART_6,
	UART_7,
	UART_8
}UART_PORTS;

typedef enum
{
	IIC_1 = 0,
	IIC_2,
	IIC_3,
	IIC_4,
	IIC_5
}IIC_PORTS;

typedef enum
{
	CAN_1 = 0,
	CAN_2,
	CAN_3
}CAN_PORTS;


typedef enum
{
	CAN_BUFFER_0 = 0,
	CAN_BUFFER_1,
	CAN_BUFFER_2,
	CAN_BUFFER_3
}CAN_BUFFERS;

typedef enum
{
	TIMER_1,
	TIMER_2,
	TIMER_3,
	TIMER_4,
	INVALID_TIMER_ID
}TIMER_PORTS;



#endif /* OS_CONFIG_ARCH_CONF_MCU_MCU_CONFIG_H_ */
