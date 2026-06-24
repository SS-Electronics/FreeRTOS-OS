/**
 * @file        conf_os.h
 * @brief       FreeRTOS-OS compile-time configuration (services, IPC, timing)
 * @ingroup     config
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Config
 * @info        Central knobs for the OS service layer: management-thread stack
 *              sizes and priorities, IPC pipe/queue depths, startup staggering,
 *              and debug switches. This is the file an integrator tunes when
 *              porting FreeRTOS-OS to a new board or trimming it for size.
 *
 * @details
 * Two sources of truth
 * ────────────────────
 *   Kconfig (autoconf.h)  Values that change per board/build — target MCU,
 *                         clock, HAL module set, heap size, IPC byte sizes,
 *                         and which services are linked in. Edit via
 *                         `make menuconfig`, then `make config-outputs`.
 *                         These appear below as `CONFIG_*` references.
 *
 *   This header           Values that are structural to the OS and rarely
 *                         change per board — thread stack/priority defaults,
 *                         management queue depths, startup offsets. Plain
 *                         compile-time constants, documented in place.
 *
 * Conventions
 * ───────────
 *   *_STACK_SIZE   FreeRTOS stack depth in WORDS (4 bytes each on Cortex-M),
 *                  not bytes. 256 words = 1 KiB.
 *   *_PRIORITY     FreeRTOS task priority. 0 = idle; higher = more urgent.
 *                  Must stay below CONFIG_RTOS_MAX_PRIORITIES.
 *   TIME_OFFSET_*  Milliseconds after scheduler start before a service spins
 *                  up. Staggering avoids a boot-time allocation/IRQ storm.
 *   *_QUEUE_DEPTH  Number of messages an IPC management queue can hold.
 *   PIPE_*_SIZE    Ring-buffer capacity in bytes for a driver byte stream.
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

#ifndef OS_CONFIG_OS_CONF_OS_CONFIG_H_
#define OS_CONFIG_OS_CONF_OS_CONFIG_H_

/* CONFIG_* symbols from menuconfig (target, clock, services, IPC sizes). */
#include "autoconf.h"

/* ── Kernel selection ───────────────────────────────────────────────────────
 * Abstraction hook for a future second RTOS back end. Only FreeRTOS exists
 * today, so OS_KERNEL_SELECT always resolves to OS_KERNEL_FREERTOS.
 */
#define OS_KERNEL_FREERTOS              1
#define OS_KERNEL_SELECT                OS_KERNEL_FREERTOS

/* ── Optional protocol stacks ───────────────────────────────────────────────
 * Higher-level stacks layered on the CAN/comm drivers. 1 = compiled in,
 * 0 = excluded. Leave off unless the application uses them.
 */
#define ISOBUS_STACK_EN                 (1)
#define CANOPEN_STACK_EN                (0)

/* ── External chip drivers (I2C/SPI add-on devices) ─────────────────────────
 * 1 = link the driver for that part, 0 = leave it out. Enable only the chips
 * physically present on your board to keep the image small.
 */
#define INC_DRIVER_PCA9685              (0)     /* 16-ch PWM (I2C)            */
#define INC_DRIVER_MC23017              (0)     /* 16-bit GPIO expander (I2C) */
#define INC_DRIVER_DS3502               (0)     /* digital potentiometer (I2C) */
#define INC_DRIVER_INA230               (0)     /* current/power monitor (I2C) */
#define INC_DRIVER_MCP3427              (0)     /* 16-bit ADC (I2C)          */
#define INC_DRIVER_M95M02               (0)     /* 2-Mbit EEPROM (SPI)       */
#define INC_DRIVER_MCP4441              (0)     /* quad digital pot (I2C)    */
#define INC_DRIVER_MCP45HVX1            (0)     /* high-voltage digital pot (I2C) */

/* ── Management queue depths (messages) ─────────────────────────────────────
 * Each management service drains a command queue. Depth bounds how many
 * in-flight requests it can buffer before a producer blocks. Powers of two
 * are not required.
 */
#define UART_MGMT_QUEUE_DEPTH           (16)
#define SPI_MGMT_QUEUE_DEPTH            (8)
#define IIC_MGMT_QUEUE_DEPTH            (8)
#define GPIO_MGMT_QUEUE_DEPTH           (16)

/* ── Driver byte-stream ring buffers (bytes) ────────────────────────────────
 * Capacity of the producer/consumer ring buffers behind each byte-oriented
 * driver. UART sizes come from Kconfig so they can be tuned per build; the
 * rest are structural defaults. Size them to one comfortably-large transfer.
 */
#define PIPE_USB_1_DRV_RX_SIZE          (4096)
#define PIPE_UART_1_DRV_RX_SIZE         CONFIG_PIPE_UART_1_DRV_RX_SIZE
#define PIPE_UART_1_DRV_TX_SIZE         CONFIG_PIPE_UART_1_DRV_TX_SIZE
#define PIPE_CAN_1_DRV_RX_SIZE          (128)
#define PIPE_CAN_2_DRV_RX_SIZE          (128)
#define PIPE_CAN_3_DRV_RX_SIZE          (128)
#define PIPE_CAN_PDU_TX_SIZE            (20)
#define PIPE_CAN_PDU_RX_SIZE            (100)
#define PIPE_CAN_APP_TX_SIZE            (128)
#define PIPE_CAN_APP_RX_SIZE            (128)
#define PIPE_IIC_PDU_TX_SIZE            (20)
#define PIPE_IIC_PDU_RX_SIZE            (100)
#define PIPE_DIAGNOSTICS_SIZE           (1)

/* ── printk / ITM print buffer ──────────────────────────────────────────────
 * Longest single line printk() can format. Kconfig-driven so a size-limited
 * build can shrink it. CONF_MAX_CHAR_IN_PRINTK is the name the printk core
 * uses; it tracks ITM_PRINT_BUFF_LENGTH exactly (no second value to maintain).
 */
#define ITM_PRINT_BUFF_LENGTH           CONFIG_ITM_PRINT_BUFF_LENGTH
#define CONF_MAX_CHAR_IN_PRINTK         ITM_PRINT_BUFF_LENGTH

/* ── Shell buffers (bytes) ──────────────────────────────────────────────────
 * SHELL_LINE_BUF_LEN  longest command line the shell accepts.
 * SHELL_OUT_BUF_LEN   per-shot output buffer handed to FreeRTOS+CLI; long
 *                     output is streamed across multiple shots, so this need
 *                     not hold a whole 'help' listing.
 */
#define SHELL_LINE_BUF_LEN              (128)
#define SHELL_OUT_BUF_LEN               (256)

/* ── Debug switches ─────────────────────────────────────────────────────────
 * DRV_DEBUG_EN / DEFAULT_DEBUG_EN are Kconfig-driven (printk on/off at boot
 * and driver-layer verbosity). The remaining flags are niche and stay as
 * compile-time constants until they earn a Kconfig entry. 1 = on, 0 = off.
 */
#define DRV_DEBUG_EN                    CONFIG_DRV_DEBUG_EN
#define DEFAULT_DEBUG_EN                CONFIG_DEFAULT_DEBUG_EN
#define DRV_DETAIL_DEBUG_EN             (0)     /* extra-verbose driver traces */
#define GW_DEBUG_EN                     (0)     /* gateway/protocol traces     */
#define ITM_DEBUG_EN                    (0)     /* route printk to SWO/ITM      */

/* ── Management service threads (stack words, priority) ──────────────────────
 * One pair per service. Bump the stack if a service overflows (visible via
 * the 'tasks'/'ps' shell commands' high-watermark column). Priorities sit at
 * 1 by default — just above idle — so the shell and app stay responsive.
 */
#define PROC_SERVICE_GPIO_MGMT_STACK_SIZE       (256)
#define PROC_SERVICE_GPIO_MGMT_PRIORITY         (1)

#define PROC_SERVICE_SERIAL_MGMT_STACK_SIZE     (512)
#define PROC_SERVICE_SERIAL_MGMT_PRIORITY       (1)

#define PROC_SERVICE_CAN_MGMT_STACK_SIZE        (1024)
#define PROC_SERVICE_CAN_MGMT_PRIORITY          (1)

#define PROC_SERVICE_IIC_MGMT_STACK_SIZE        (1024)
#define PROC_SERVICE_IIC_MGMT_PRIORITY          (1)

#define PROC_SERVICE_SPI_MGMT_STACK_SIZE        (512)
#define PROC_SERVICE_SPI_MGMT_PRIORITY          (1)

#define PROC_SERVICE_OS_SHELL_MGMT_STACK_SIZE   (1024)
#define PROC_SERVICE_OS_SHELL_MGMT_PRIORITY     (1)

#define PROC_SERVICE_TASK_MGMT_STACK_SIZE       (512)
#define PROC_SERVICE_TASK_MGMT_PRIORITY         (1)

#define TEST_SUITE_STACK_SIZE                   (512)
#define TEST_SUITE_PRIORITY                     (1)

/* ── Startup staggering (ms after scheduler start) ──────────────────────────
 * Services come up in this order, spaced out so heap allocation, PHY/PLL
 * settling and IRQ wiring do not all hit at once. Adjust only if a service
 * has a hard dependency that must initialise earlier/later.
 */
#define TIME_OFFSET_GPIO_MANAGEMENT     (10)
#define TIME_OFFSET_SERIAL_MANAGEMENT   (20)
#define TIME_OFFSET_OS_SHELL_MGMT       (1000)
#define TIME_OFFSET_SPI_MANAGEMENT      (5500)
#define TIME_OFFSET_IIC_MANAGEMENT      (6500)
#define TIME_OFFSET_ETH_MANAGEMENT      (7000)
#define TIME_OFFSET_CAN_MANAGEMENT      (10000)
#define TIME_OFFSET_TEST_SUITE          (11000)

/* ── Timeouts / periods (ms unless noted) ───────────────────────────────────
 * TASK_MGR_SCAN_PERIOD_MS  how often the task manager refreshes its health
 *                          snapshot (read by 'tasks'/'ps'/'heap').
 * TIMEOUT_IIC_PIPE_OP      I2C IPC pipe operation timeout (ms).
 * IIC_ACK_TIMEOUT_MS       per-address ACK wait during an I2C probe/scan.
 */
#define TASK_MGR_SCAN_PERIOD_MS         (1000)
#define TIMEOUT_IIC_PIPE_OP             (2)
#define IIC_ACK_TIMEOUT_MS              (100)

/* ── Watchdog service thread (stack words, priority) ────────────────────────
 * The watchdog must preempt anything it supervises, so it runs ABOVE every
 * monitored task. Keep PROC_SERVICE_WDOG_PRIORITY at least one step above the
 * highest application-task priority, or a runaway task could starve it.
 */
#define PROC_SERVICE_WDOG_STACK_SIZE    (512)
#define PROC_SERVICE_WDOG_PRIORITY      (4)

/* ── Structured logger echo threshold ───────────────────────────────────────
 * Log records at or below this severity are mirrored to printk() immediately.
 * 0 = ERROR only, 1 = +WARN, 2 = +INFO, 3 = +DEBUG.
 */
#define SLOG_PRINTK_MIN_LEVEL           (1)

#endif /* OS_CONFIG_OS_CONF_OS_CONFIG_H_ */
