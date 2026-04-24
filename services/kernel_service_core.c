/*
 * kernel_service_core.c — Central OS service task registration
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * All FreeRTOS service threads are registered here via a single call to
 * os_kernel_thread_register() from main().  Each service's _start() function
 * is responsible for its own internal queue creation and task spawn — this
 * file is only the dispatch table.
 *
 * Registration order
 * ──────────────────
 *   1. task_mgr   — health monitor (always enabled)
 *   2. gpio_mgmt  — GPIO command queue (always enabled)
 *   3. uart_mgmt  — UART RX/TX daemon   (CONFIG_INC_SERVICE_UART_MGMT)
 *   4. iic_mgmt   — I2C transfer daemon  (INC_SERVICE_IIC_MGMT)
 *   5. spi_mgmt   — SPI transfer daemon  (BOARD_SPI_COUNT > 0)
 */

#include <os/kernel_services.h>
#include <os/kernel.h>
#include <services/gpio_mgmt.h>

#if (INC_SERVICE_UART_MGMT == 1)
#  include <services/uart_mgmt.h>
#endif

#if (INC_SERVICE_IIC_MGMT == 1)
#  include <services/iic_mgmt.h>
#endif

#if (BOARD_SPI_COUNT > 0)
#  include <services/spi_mgmt.h>
#endif


status_t os_kernel_thread_register(void)
{
    status_t status = OS_ERR_NONE;

    /* Task health monitor */
    if (os_thread_create(thread_task_mgmt, "task_mgr",
                         PROC_SERVICE_TASK_MGMT_STACK_SIZE,
                         PROC_SERVICE_TASK_MGMT_PRIORITY, NULL) < 0)
        status |= OS_ERR_OP;

    /* GPIO management */
    if (gpio_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;

#if (INC_SERVICE_UART_MGMT == 1)
    if (uart_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

#if (INC_SERVICE_IIC_MGMT == 1)
    if (iic_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

#if (BOARD_SPI_COUNT > 0)
    if (spi_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

    /* Start interactive shell on UART_APP (USART2, PA2/PA3, /dev/ttyUSB0).
     * The shell task delays TIME_OFFSET_OS_SHELL_MGMT ms internally so it
     * waits for uart_mgmt to complete UART initialisation first. */
    os_shell_mgmt_start();

    return status;
}
