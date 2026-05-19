/**
 * @file        kernel_services.h
 * @brief       kernel services
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

#ifndef KERNEL_SERVICES_H_
#define KERNEL_SERVICES_H_

#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <ipc/mqueue.h>

#include <FreeRTOS.h>
#include <task.h>

/* Driver related includes*/

/* Map legacy CONF_THREAD_* names to the conf_os.h PROC_SERVICE_* names */
#define CONF_THREAD_UART_MGMT_STACK_SIZE    PROC_SERVICE_SERIAL_MGMT_STACK_SIZE
#define CONF_THREAD_UART_MGMT_PRIORITY      PROC_SERVICE_SERIAL_MGMT_PRIORITY
#define CONF_THREAD_IIC_MGMT_STACK_SIZE     PROC_SERVICE_IIC_MGMT_STACK_SIZE
#define CONF_THREAD_IIC_MGMT_PRIORITY       PROC_SERVICE_IIC_MGMT_PRIORITY



/**************  API Export *****************/
#ifdef __cplusplus
extern "C" {
#endif


status_t os_kernel_thread_register(void);
int32_t  task_mgr_start(void);

#if defined(CONFIG_INC_SERVICE_WDOG) && (CONFIG_INC_SERVICE_WDOG == 1)
int32_t  wdog_service_start(void);
#endif




void thread_uart_mgmt(void * arg);
void therad_iic_mgmt(void * arg);
void thread_task_mgmt(void * arg);


#ifdef __cplusplus
}
#endif
/**************  END API Export *****************/


#endif /* KERNEL_SERVICES_H_ */
