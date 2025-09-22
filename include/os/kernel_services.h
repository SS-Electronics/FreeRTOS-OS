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

#ifndef KERNEL_SERVICES_H_
#define KERNEL_SERVICES_H_

#include <def_std.h>

/* Driver related includes*/





/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif


status_type os_kernel_thread_register(void);






void thread_uart_mgmt(void * arg);
void therad_iic_mgmt(void * arg);


#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* KERNEL_SERVICES_H_ */
