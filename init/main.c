/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS Basic Software
#
# RTOS Basic Software is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS Basic Software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#include <device.h>
#include <board/mcu_config.h>
#include <ipc/mqueue.h>
#include <ipc/global_var.h>
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <drivers/cpu/drv_cpu.h>
#include <drivers/drv_rcc.h>
#include <services/uart_mgmt.h>


/* ── Weak application entry point ─────────────────────────────────────── */

__attribute__((weak)) int app_main(void) { return 0; }


/* ── IPC message-queue registration for all enabled peripherals ────────── */

static void ipc_queues_init(void)
{
    ipc_mqueue_init();

    for (uint8_t i = 0; i < NO_OF_UART; i++)
    {
        global_uart_tx_mqueue_list[i] = ipc_mqueue_register(
            IPC_MQUEUE_TYPE_UART_HW, (int32_t)i,
            (int32_t)sizeof(uint8_t), PIPE_UART_1_DRV_TX_SIZE);

        global_uart_rx_mqueue_list[i] = ipc_mqueue_register(
            IPC_MQUEUE_TYPE_UART_HW, (int32_t)i,
            (int32_t)sizeof(uint8_t), PIPE_UART_1_DRV_RX_SIZE);
    }

#if (NO_OF_IIC > 0)
    /* IIC queues — only registered when the IIC service is enabled */
    for (uint8_t i = 0; i < NO_OF_IIC; i++)
    {
        global_iic_tx_mqueue_list[i] = 0;
        global_iic_rx_mqueue_list[i] = 0;
    }
#endif
}


/* ── OS entry point ────────────────────────────────────────────────────── */

__SECTION_BOOT __USED void main(void)
{
    /* 1. Bring up HAL and configure the system clock first so all
     *    peripheral clocks derived from SYSCLK are at their final rates. */
    HAL_Init();
    
    drv_rcc_clock_init();

    /* 2. Set NVIC interrupt priorities required by FreeRTOS before any
     *    interrupt-driven peripheral is initialised. */
    drv_cpu_interrupt_prio_set();

    /* 3. Initialise the IPC message-queue subsystem and register a TX and
     *    RX ring-buffer queue for every UART channel.  These IDs are stored
     *    in global_uart_*_mqueue_list[] and consumed by drv_uart and printk. */
    ipc_queues_init();

    /* 4. Create the UART management thread.  It performs peripheral HW init
     *    after TIME_OFFSET_SERIAL_MANAGEMENT ms once the scheduler starts. */
    uart_mgmt_start();

    /* 5. Bind printk() to the TX ring-buffer of COMM_PRINTK_HW_ID.
     *    Must be called after ipc_queues_init() registers the queue. */
    printk_init();

    /* 6. Hand control to the application layer.  app_main() creates tasks
     *    and returns immediately; tasks are driven by the scheduler below. */
    app_main();

    /* 7. Start the FreeRTOS scheduler — does not return under normal
     *    operation.  The while(1) is a safety net only. */
    vTaskStartScheduler();

    while (1) {}
}
