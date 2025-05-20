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

#ifndef OS_IPC_MQUEUE_MQUEUE_H_
#define OS_IPC_MQUEUE_MQUEUE_H_

#include <def_std.h>
#include <def_compiler.h>
#include <def_env_macros.h>
#include <def_hw.h>
#include <conf_os.h>
#include <lib/ringbuffer.h>



#if (NO_OF_UART > 0)
extern struct ringbuffer	ipc_handle_uart_1_drv_rx_handle;
extern struct ringbuffer	ipc_handle_printk_buffer;
#endif

#if (NO_OF_UART > 1)
extern struct ringbuffer	ipc_handle_uart_2_drv_tx_handle;
extern struct ringbuffer	ipc_handle_uart_2_drv_rx_handle;
#endif



/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif

status_type 		ipc_mqueue_init(void);


#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/



#endif /* OS_IPC_MQUEUE_MQUEUE_H_ */
