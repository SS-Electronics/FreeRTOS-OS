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

#include <ipc/global_var.h>

/* driver status variables */

int32_t global_uart_tx_mqueue_list[NO_OF_UART];
int32_t global_uart_rx_mqueue_list[NO_OF_UART];
int32_t global_iic_tx_mqueue_list[NO_OF_IIC];
int32_t global_iic_rx_mqueue_list[NO_OF_IIC];


