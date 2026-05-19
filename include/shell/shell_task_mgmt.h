/**
 * @file        shell_task_mgmt.h
 * @brief       shell_task_mgmt.h — CLI commands for task and heap inspection
 * @ingroup     shell
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Shell
 * @info        Interactive shell over UART_APP using FreeRTOS-Plus-CLI; registers OS introspection commands.
 * @dependency  FreeRTOS-Plus-CLI, UART mgmt service
 *
 * @details
 * shell_task_mgmt.h — CLI commands for task and heap inspection
 *
 * Commands registered by shell_task_mgmt_register_cmds():
 *   heap   — live heap usage: total / used / free / min-ever-free + fault count
 *   tasks  — per-task snapshot: name, state, stack high-watermark, thread ID
 *
 * Call shell_task_mgmt_register_cmds() once before the scheduler starts
 * (from os_shell_management.c task startup) so the commands are available
 * as soon as the shell is active.
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

#ifndef FREERTOS_OS_INCLUDE_SHELL_SHELL_TASK_MGMT_H_
#define FREERTOS_OS_INCLUDE_SHELL_SHELL_TASK_MGMT_H_

#ifdef __cplusplus
extern "C" {
#endif

void shell_task_mgmt_register_cmds(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_SHELL_SHELL_TASK_MGMT_H_ */
