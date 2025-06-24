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

#ifndef OS_IPC_GLOBAL_GLOBAL_VAR_H_
#define OS_IPC_GLOBAL_GLOBAL_VAR_H_

#include <def_std.h>
#include <conf_os.h>

/* External variables declaration */
/* Driver mqueue ids */
extern int32_t global_uart_tx_mqueue_list[NO_OF_UART];
extern int32_t global_uart_rx_mqueue_list[NO_OF_UART];
extern int32_t global_iic_tx_mqueue_list[NO_OF_IIC];
extern int32_t global_iic_rx_mqueue_list[NO_OF_IIC];



#endif /* OS_IPC_GLOBAL_GLOBAL_VAR_H_ */
