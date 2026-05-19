/**
 * @file        init.h
 * @brief       init
 * @ingroup     boot
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Boot & Init
 * @info        System bring-up: HAL init, clock tree, IRQ tables, services, scheduler entry.
 * @dependency  FreeRTOS, HAL, IRQ table, board BSP
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

#ifndef _INIT_H_
#define _INIT_H_

/* Device drivers include for getting the handles*/

#include <def_hw.h>

/* Driver related include */
#include <drivers/drv_uart.h>


#include <ipc/mqueue.h>


#include <os/kernel_services.h>
/* Here we have accommodate all the handles as per of the requirements */









/**************  API Export *****************/





/* Entry Point of the OS and APP */

#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/











#endif
