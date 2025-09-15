/*
 * board.h
 *
 *  Created on: Aug 22, 2023
 *      Author: subhajit-roy
 */

#ifndef OS_CONFIG_ARCH_CONF_BOARD_BOARD_H_
#define OS_CONFIG_ARCH_CONF_BOARD_BOARD_H_

#include <bsw/utils/type_def/typedef_global.h>
#include <app/bsw_config/arch_conf/mcu/mcu_config.h>


#define SERIAL_DEBUG_UART       UART_1
#define UART_1_BAUD             (115200)
#define UART_1SRC_CLK           (150000000)




//
//#define SERIAL_DEBUG_COMM 						UART_1
//#define MAIN_COMM_BUS 							CAN_1
//#define MCP3427_COMM							IIC_1
//#define DS3502_COMM								IIC_1
//
//#define MCP3427_1_ADDRESS						(0xD0)
//#define MCP3427_2_ADDRESS						(0xDC)
//#define MCP3427_3_ADDRESS                       (0xD8)
//#define MCP3427_4_ADDRESS                       (0xD4)
//
////#define EXT_DS3502_1_ADDR						(0x50)
////#define EXT_DS3502_2_ADDR						(0x54)
//#define EXT_DS3502_1_ADDR						(0x50)
//
//#define PORT_KILL_SW                            (DRV_GPIO_PORT_D)
//#define PIN_KILL_SW                             (GPIO_PIN_4)
//
//#define PORT_MOTOR_DRIVER_EN					(DRV_GPIO_PORT_E)
//#define PIN_MOTOR_DRIVER_EN						(GPIO_PIN_7)
//
//#define PORT_AUTO_MAN							(DRV_GPIO_PORT_D)
//#define PIN_AUTO_MAN							(GPIO_PIN_11)
//#define AUTO_MODE_LOGIC							(0)
//#define MANUAL_MODE_LOGIC						(1)
//
//#define PORT_FRD_OP								(DRV_GPIO_PORT_D)
//#define PIN_FRD_OP								(GPIO_PIN_5)
//
//#define PORT_REV_OP								(DRV_GPIO_PORT_D)
//#define PIN_REV_OP								(GPIO_PIN_6)


//#define PORT_ACT_2_CW							(DRV_GPIO_PORT_E)
//#define PIN_ACT_2_CW							(GPIO_PIN_7)
//
//#define PORT_ACT_2_CCW							(DRV_GPIO_PORT_B)
//#define PIN_ACT_2_CCW							(GPIO_PIN_2)

#endif /* OS_CONFIG_ARCH_CONF_BOARD_BOARD_H_ */
