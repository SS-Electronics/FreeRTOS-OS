/*
 * os_shell_management.h — FreeRTOS+CLI interactive shell service
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * The shell reads bytes from the UART_SHELL RX ring buffer (populated by the
 * UART RX ISR via the irq_desc chain), performs line editing, and dispatches
 * complete lines to FreeRTOS_CLIProcessCommand().  Output is written directly
 * to the UART_SHELL TX ring buffer and kicked via drv_uart_tx_kick(), so the
 * existing interrupt-driven TX path (TXEIE ISR → ring buffer drain) handles
 * the actual transmission — the same path used by printk().
 *
 * RX data flow:
 *   USART2_IRQHandler
 *     → hal_uart_stm32_irq_handler(USART2)
 *       → irq_desc chain: handle_simple_irq → _uart_rx_cb (uart_mgmt.c)
 *         → ringbuffer_putchar(rx_rb)          ← shell reads from here
 *
 * TX data flow:
 *   _shell_write() → ringbuffer_put(tx_rb) → drv_uart_tx_kick()
 *     → TXEIE enabled → TXE ISR → drv_uart_tx_get_next_byte()
 *
 * Usage:
 *   Call os_shell_mgmt_start() from the OS init sequence (after
 *   ipc_queues_init() and uart_mgmt_start()).  The shell task waits
 *   TIME_OFFSET_OS_SHELL_MGMT ms before becoming active to let the UART
 *   driver come online first.
 */

#ifndef OS_SERVICES_OS_SHELL_MANAGEMENT_H_
#define OS_SERVICES_OS_SHELL_MANAGEMENT_H_

#include <def_std.h>
#include <FreeRTOS_CLI.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Create and start the OS shell management task.
 *
 * Must be called once from the OS init sequence before the scheduler starts.
 * The task will delay TIME_OFFSET_OS_SHELL_MGMT ms before printing the banner
 * and entering the read loop, ensuring the UART driver is initialised first.
 *
 * @return OS_ERR_NONE on success, OS_ERR_OP if task creation fails.
 */
int32_t os_shell_mgmt_start(void);

/**
 * @brief  Register a custom FreeRTOS+CLI command with the shell.
 *
 * Thin wrapper around FreeRTOS_CLIRegisterCommand() so callers only need to
 * include this header.  Must be called before os_shell_mgmt_start(), or at
 * any time from a task (FreeRTOS+CLI uses a linked list protected at the
 * application level).
 *
 * @param  cmd  Pointer to a const CLI_Command_Definition_t in static storage.
 * @return pdTRUE on success, pdFALSE if registration fails.
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
