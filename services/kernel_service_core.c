/**
 * @file    kernel_service_core.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  kernel
 * @brief   Central OS service task registration and initialization dispatcher
 *
 * @details
 * This module acts as the centralized registration point for all FreeRTOS-OS
 * service threads. It is responsible for starting system-level services in a
 * controlled and deterministic order during system boot.
 *
 * Each service module (GPIO, UART, I2C, SPI, Shell, Task Manager, etc.)
 * encapsulates its own internal initialization logic, including:
 * - FreeRTOS queue creation
 * - Hardware driver registration
 * - Task creation and startup delay handling
 *
 * This file does NOT implement service logic itself; it only orchestrates
 * startup sequencing.
 *
 * ---------------------------------------------------------------------------
 * Service Registration Model
 * ---------------------------------------------------------------------------
 *
 * All services are started via a single entry point:
 *
 *      os_kernel_thread_register()
 *                  ↓
 *     Sequential service startup calls
 *
 * Each service is responsible for:
 *   - Creating its own FreeRTOS task(s)
 *   - Initializing internal queues
 *   - Registering hardware drivers
 *   - Handling deferred initialization if needed
 *
 * ---------------------------------------------------------------------------
 * Startup Order (Boot Sequence)
 * ---------------------------------------------------------------------------
 *
 * 1. GPIO management service
 *    - Immediate GPIO subsystem initialization
 *
 * 2. UART management service (conditional)
 *    - UART driver registration
 *    - RX/TX interrupt wiring
 *
 * 3. I2C management service (conditional)
 *    - I2C HAL registration
 *    - Interrupt-driven transaction support
 *
 * 4. SPI management service (conditional)
 *    - SPI HAL initialization
 *    - Full-duplex transfer support
 *
 * 5. Task manager service
 *    - System health monitoring
 *    - Stack, heap, and timer diagnostics
 *
 * 6. Shell management service
 *    - CLI initialization
 *    - Depends on UART service readiness (internal delay handling)
 *
 * ---------------------------------------------------------------------------
 * Design Principles
 * ---------------------------------------------------------------------------
 *
 * - Centralized service startup for deterministic boot sequence
 * - Decoupled service implementation (each module self-contained)
 * - Conditional compilation based on hardware configuration
 * - Failure tracking via aggregated status flags
 *
 * ---------------------------------------------------------------------------
 * Dependencies
 * ---------------------------------------------------------------------------
 * os/kernel_services.h
 * os/kernel.h
 * services/gpio_mgmt.h
 * services/os_shell_management.h
 * services/uart_mgmt.h (optional)
 * services/iic_mgmt.h (optional)
 * services/spi_mgmt.h (optional)
 *
 * ---------------------------------------------------------------------------
 * @note
 * - This module is typically invoked once from main() during system boot
 * - Service failures are aggregated but do not stop subsequent services
 *
 * @warning
 * - Partial initialization may occur if some services fail
 * - Hardware-dependent services are conditionally compiled
 * - Order matters for dependency-sensitive services (e.g., shell depends on UART)
 *
 * ---------------------------------------------------------------------------
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

#include <os/kernel_services.h>
#include <os/kernel.h>
#include <services/gpio_mgmt.h>
#include <services/os_shell_management.h>

#if (BOARD_UART_COUNT > 0)
#  include <services/uart_mgmt.h>
#endif

#if (BOARD_IIC_COUNT > 0)
#  include <services/iic_mgmt.h>
#endif

#if (BOARD_SPI_COUNT > 0)
#  include <services/spi_mgmt.h>
#endif

status_t os_kernel_thread_register(void)
{
    status_t status = OS_ERR_NONE;

    /* GPIO management service (always enabled) */
    if (gpio_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;

#if (BOARD_UART_COUNT > 0)
    if (uart_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

#if (BOARD_IIC_COUNT > 0)
    if (iic_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

#if (BOARD_SPI_COUNT > 0)
    if (spi_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;
#endif

    /* Task health monitoring service */
    if (task_mgr_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;

    /* Shell service (internally waits for UART initialization) */
    if (os_shell_mgmt_start() != OS_ERR_NONE)
        status |= OS_ERR_OP;

    return status;
}