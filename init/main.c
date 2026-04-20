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
#include <services/uart_mgmt.h>


/* ── Weak application entry point ─────────────────────────────────────── */

__attribute__((weak)) int app_main(void) { return 0; }


/* ── STM32F411 system clock: HSI PLL → 100 MHz ────────────────────────── */

static void system_clock_config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* Enable PWR clock and set core voltage for 100 MHz operation */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* HSI on, drive PLL: VCO_in = 16/16 = 1 MHz, VCO_out = 200 MHz,
     * SYSCLK = 200/2 = 100 MHz */
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM            = 16;
    osc.PLL.PLLN            = 200;
    osc.PLL.PLLP            = RCC_PLLP_DIV2;
    osc.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&osc);

    /* SYSCLK = PLL (100 MHz), AHB = 100 MHz,
     * APB1 = 50 MHz (max), APB2 = 100 MHz, 3 flash wait-states */
    clk.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);
}


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
    system_clock_config();

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
