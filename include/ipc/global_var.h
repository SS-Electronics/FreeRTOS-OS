/**
 * @file        global_var.h
 * @brief       global var
 * @ingroup     ipc
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      IPC
 * @info        Inter-process communication: ring buffers (HW byte streams) and xQueue-based message queues.
 * @dependency  FreeRTOS queue, ringbuffer.h
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef OS_IPC_GLOBAL_GLOBAL_VAR_H_
#define OS_IPC_GLOBAL_GLOBAL_VAR_H_

#include <def_std.h>
#include <conf_os.h>
#include <board/board_config.h>

/* External variables declaration */
/* Driver mqueue ids */
extern int32_t global_uart_tx_mqueue_list[BOARD_UART_COUNT];
extern int32_t global_uart_rx_mqueue_list[BOARD_UART_COUNT];
extern int32_t global_iic_tx_mqueue_list[BOARD_IIC_COUNT];
extern int32_t global_iic_rx_mqueue_list[BOARD_IIC_COUNT];



#endif /* OS_IPC_GLOBAL_GLOBAL_VAR_H_ */
