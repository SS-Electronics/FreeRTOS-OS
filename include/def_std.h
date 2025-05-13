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

#include <def_compiler.h>





typedef int	status_type;

#define ERROR_NONE			0
#define ERROR_OP			1
#define ERROR_TIMEOUT		-1
#define ERROR_NO_ACK		-2



typedef struct
{
	uint32_t err_code;
	uint32_t timestamp;
	uint32_t count;
}type_diagnostic_data;












#endif /* FREERTOS_OS_INCLUDE_DEF_STD_H_ */
