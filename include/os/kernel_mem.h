/**
 * @file        kernel_mem.h
 * @brief       kernel mem
 * @ingroup     public_api
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Public API
 * @info        Public API surface — included by application code and out-of-tree drivers.
 * @dependency  FreeRTOS, def_std.h
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

#ifndef FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_
#define FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_

#include <def_std.h>
#include <def_err.h>
#include <def_attributes.h>

#include <FreeRTOS.h>
#include <atomic.h>
#include <queue.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>
#include <event_groups.h>
#include <stream_buffer.h>





/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void*  		kmaloc(size_t size);

int32_t 	kfree(void *p);

uint32_t    os_get_free_mem(void);




#ifdef __cplusplus
}
#endif /* __cplusplus */
/**************  END API Export *****************/





#endif /* FREERTOS_OS_INCLUDE_OS_KERNEL_MEM_H_ */
