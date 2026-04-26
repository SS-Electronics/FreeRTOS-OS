/**
 * @file    os_shell_management.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  services
 * @brief   FreeRTOS+CLI interactive shell service interface
 *
 * @details
 * This module provides the public interface for the OS shell service built on
 * top of FreeRTOS+CLI. The shell runs as a dedicated FreeRTOS task and
 * provides an interactive UART-based command-line interface.
 *
 * Architecture overview:
 *
 * RX path (input):
 * - UART RX ISR (USARTx_IRQHandler)
 *   → HAL UART IRQ handler
 *   → IRQ dispatch layer (irq_desc chain)
 *   → UART RX callback (_uart_rx_cb in uart_mgmt.c)
 *   → RX ring buffer (per UART channel)
 *   → Shell task polls buffer and processes input
 *
 * TX path (output):
 * - Shell write function (_shell_write)
 *   → UART TX ring buffer
 *   → drv_uart_tx_kick()
 *   → TXE interrupt enabled
 *   → ISR drains buffer byte-by-byte
 *
 * This design ensures:
 * - Non-blocking shell I/O
 * - ISR-driven UART transmission
 * - Decoupling between CLI processing and hardware I/O
 *
 * Shell responsibilities:
 * - Line editing (backspace, delete, ESC, Ctrl-C handling)
 * - Command parsing via FreeRTOS+CLI
 * - Multi-line CLI output handling
 * - UART-based interactive terminal interface
 *
 * Initialization sequence:
 * @code
 * os_shell_mgmt_start()
 *      ↓
 * Create FreeRTOS shell task
 *      ↓
 * Delay (TIME_OFFSET_OS_SHELL_MGMT)
 *      ↓
 * Ensure UART + IPC subsystems are ready
 *      ↓
 * Print boot banner
 *      ↓
 * Enter interactive CLI loop
 * @endcode
 *
 * Dependencies:
 * - FreeRTOS kernel
 * - FreeRTOS+CLI
 * - UART management service (uart_mgmt)
 * - IPC ringbuffer system
 * - drv_uart HAL abstraction
 * - board configuration system
 *
 * @note
 * - Shell depends on UART_SHELL being initialized by uart_mgmt service
 * - RX/TX are fully interrupt-driven via ring buffers
 * - CLI execution runs in shell task context (not ISR safe)
 *
 * @warning
 * - Input buffer is statically allocated; long input lines are truncated
 * - Command execution must not block indefinitely
 * - TX buffer overflow may drop output characters
 *
 * @license
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OS_SERVICES_OS_SHELL_MANAGEMENT_H_
#define OS_SERVICES_OS_SHELL_MANAGEMENT_H_

#include <services/os_shell_management.h>
#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <conf_os.h>
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <ipc/ringbuffer.h>
#include <ipc/global_var.h>
#include <ipc/mqueue.h>
#include <drivers/drv_time.h>
#include <drivers/drv_uart.h>
#include <FreeRTOS_CLI.h>
#include <shell/shell_task_mgmt.h>
#include <board/board_config.h>
#include <board/board_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the OS shell management task.
 *
 * This function spawns the shell task responsible for:
 * - Reading UART RX ring buffer input
 * - Processing CLI commands
 * - Managing interactive terminal session
 *
 * The task delays startup by TIME_OFFSET_OS_SHELL_MGMT to ensure that:
 * - UART drivers are initialized
 * - IPC ringbuffers are ready
 * - System services are stable
 *
 * @return OS_ERR_NONE on success
 * @return OS_ERR_OP if task creation fails
 */
int32_t os_shell_mgmt_start(void);

/**
 * @brief Register a FreeRTOS+CLI command with the shell.
 *
 * Convenience wrapper around FreeRTOS_CLIRegisterCommand().
 * Allows external modules to register CLI commands without directly
 * including FreeRTOS+CLI headers.
 *
 * Command storage must remain valid for the lifetime of the system.
 *
 * @param cmd Pointer to CLI command definition structure
 * @return pdTRUE if registration succeeds
 * @return pdFALSE if registration fails
 */
static inline BaseType_t os_shell_register_command(
        const CLI_Command_Definition_t *cmd)
{
    return FreeRTOS_CLIRegisterCommand(cmd);
}

#ifdef __cplusplus
}
#endif

#endif /* OS_SERVICES_OS_SHELL_MANAGEMENT_H_ */