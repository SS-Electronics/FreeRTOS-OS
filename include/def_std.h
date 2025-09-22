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

#ifndef FREERTOS_OS_INCLUDE_DEF_STD_H_
#define FREERTOS_OS_INCLUDE_DEF_STD_H_

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>



typedef int32_t status_t;    


/**************************************
 * 
 *  Error definitions
 * 
 *************************************/

#define OS_ERR_NONE         0
#define OS_ERR_OP           -1
#define OS_ERR_MEM_OF       -2
#define OS_ERR_MEM_OP       -3





#endif /* FREERTOS_OS_INCLUDE_DEF_STD_H_ */
