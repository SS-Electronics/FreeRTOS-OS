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

#include <os/kernel_mem.h>











 void *  kmaloc(size_t size)
 {
    void *ret;
	ret = pvPortMalloc(size);

	return ret;
}

int32_t kfree(void *p)
{
    if(p != NULL)
    {
        vPortFree(p);
        return ERROR_NONE;
    }
    else
    {
        return ERROR_OP;
    }
}
