/*
 * conf_os.h — OS and service configuration
 *
 * Fields that have a Kconfig counterpart are driven by autoconf.h.
 * Edit them with:  make menuconfig
 * Regenerate:      make config-outputs
 *
 * Fields without a Kconfig entry (protocol stacks, module drivers,
 * fixed IPC sizes) remain as compile-time constants here.
 */

#ifndef OS_CONFIG_OS_CONF_OS_CONFIG_H_
#define OS_CONFIG_OS_CONF_OS_CONFIG_H_

/* Pull in CONFIG_* symbols from menuconfig */
#include "autoconf.h"

/* ── Kernel selection ────────────────────────────────────────────────── */
#define OS_KERNEL_FREERTOS      1
#define OS_KERNEL_SELECT        OS_KERNEL_FREERTOS

/* ── Protocol stack configuration ───────────────────────────────────── */
#define ISOBUS_STACK_EN                 (1)
#define CANOPEN_STACK_EN                (0)

/* ── Module driver activation ───────────────────────────────────────── */
#define INC_DRIVER_PCA9685              (0)
#define INC_DRIVER_MC23017              (0)
#define INC_DRIVER_DS3502               (0)
#define INC_DRIVER_INA230               (0)
#define INC_DRIVER_MCP3427              (0)
#define INC_DRIVER_M95M02               (0)
#define INC_DRIVER_MCP4441              (0)
#define INC_DRIVER_MCP45HVX1            (0)

/* ── IPC / pipe sizes — driven by Kconfig where available ───────────── */
/* ── Management queue depths ────────────────────────────────────────── */
#define UART_MGMT_QUEUE_DEPTH           (16)
#define SPI_MGMT_QUEUE_DEPTH            (8)
#define IIC_MGMT_QUEUE_DEPTH            (8)
#define GPIO_MGMT_QUEUE_DEPTH           (16)



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

#define ITM_PRINT_BUFF_LENGTH           CONFIG_ITM_PRINT_BUFF_LENGTH
#define CONF_MAX_CHAR_IN_PRINTK         ITM_PRINT_BUFF_LENGTH

/* UART hardware index used by the OS shell CLI — must be < NO_OF_UART.
 * Matches UART_APP (dev_id=1) from board_device_ids.h. */
#define UART_SHELL_HW_ID                (1)   /* UART_APP  (USART2) */

/* printk() shares the shell UART so debug output appears inline in the shell */
#define COMM_PRINTK_HW_ID               UART_SHELL_HW_ID

/* Shell line-editing and output buffer sizes */
#define SHELL_LINE_BUF_LEN              (128)
#define SHELL_OUT_BUF_LEN               (128)

/* ── Debug activation — driven by Kconfig ───────────────────────────── */
#define DRV_DEBUG_EN                    CONFIG_DRV_DEBUG_EN
#define DEFAULT_DEBUG_EN                CONFIG_DEFAULT_DEBUG_EN
/* Not yet in Kconfig — keep as compile-time constants */
#define DRV_DETAIL_DEBUG_EN             (0)
#define GW_DEBUG_EN                     (0)
#define ITM_DEBUG_EN                    (0)

/* ── Service thread properties ──────────────────────────────────────── */
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

/* ── Startup timing offsets (ms) ────────────────────────────────────── */
#define TIME_OFFSET_GPIO_MANAGEMENT     (10)
#define TIME_OFFSET_SERIAL_MANAGEMENT   (20)
#define TIME_OFFSET_OS_SHELL_MGMT       (1000)
#define TIME_OFFSET_SPI_MANAGEMENT      (5500)
#define TIME_OFFSET_IIC_MANAGEMENT      (6500)
#define TIME_OFFSET_ETH_MANAGEMENT      (7000)
#define TIME_OFFSET_CAN_MANAGEMENT      (10000)
#define TIME_OFFSET_TEST_SUITE          (11000)

/* ── Timeout constants ──────────────────────────────────────────────── */
#define TASK_MGR_SCAN_PERIOD_MS         (1000)
#define TIMEOUT_IIC_PIPE_OP             (2)
#define IIC_ACK_TIMEOUT_MS              (100)



#endif /* OS_CONFIG_OS_CONF_OS_CONFIG_H_ */
