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

#ifndef INCLUDE_IPC_IPC_TYPES_H_
#define INCLUDE_IPC_IPC_TYPES_H_

#include <def_std.h>
#include <def_compiler.h>
#include <def_env_macros.h>



/* *****************************************************
 *
 *
 *
 * *****************************************************/
typedef struct
{
	uint16_t	device_id;
	uint32_t	slave_address;
	uint8_t 	iic_operation;
	uint8_t		reg_addr;
	uint8_t		txn_data[32];
	uint8_t		length;
	uint8_t		op_status;
	uint8_t		drv_status;
	uint32_t	time_stamp;
}type_iic_pdu_struct;





#endif /* INCLUDE_IPC_IPC_TYPES_H_ */
