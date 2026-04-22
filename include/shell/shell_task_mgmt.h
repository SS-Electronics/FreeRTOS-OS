/*
 * shell_task_mgmt.h — CLI commands for task and heap inspection
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Commands registered by shell_task_mgmt_register_cmds():
 *   heap   — live heap usage: total / used / free / min-ever-free + fault count
 *   tasks  — per-task snapshot: name, state, stack high-watermark, thread ID
 *
 * Call shell_task_mgmt_register_cmds() once before the scheduler starts
 * (from os_shell_management.c task startup) so the commands are available
 * as soon as the shell is active.
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
