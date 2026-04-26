/**
 * @file    main.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  core
 * @brief   Primary OS boot entry point and scheduler handoff
 *
 * @details
 * This file contains the main firmware entry routine executed after MCU reset
 * startup completes. Control reaches this file after the startup assembly
 * code initializes memory sections and calls @c main().
 *
 * This module is responsible for performing the complete system bring-up
 * sequence before handing control to the FreeRTOS scheduler.
 *
 * Boot responsibilities:
 * - Vendor HAL initialization
 * - System clock / PLL configuration
 * - Global peripheral clock enable
 * - IRQ descriptor and dispatch initialization
 * - IPC subsystem initialization
 * - OS service thread registration
 * - Application task registration
 * - CPU interrupt priority preparation for FreeRTOS
 * - Scheduler start
 *
 * Startup flow:
 * @code
 * Reset_Handler()
 *      ↓
 * SystemInit()
 *      ↓
 * .data copy / .bss clear
 *      ↓
 * main()
 *      ↓
 * HAL_Init()
 *      ↓
 * drv_rcc_clock_init()
 *      ↓
 * board_clk_enable()
 *      ↓
 * irq_hw_init_all()
 *      ↓
 * ipc_mqueue_init()
 *      ↓
 * os_kernel_thread_register()
 *      ↓
 * app_main()
 *      ↓
 * drv_cpu_interrupt_prio_set()
 *      ↓
 * vTaskStartScheduler()
 * @endcode
 *
 * Runtime interrupt model:
 * @code
 * Peripheral IRQ
 *      ↓
 * Vector table ISR
 *      ↓
 * HAL IRQ Handler
 *      ↓
 * irq_notify_from_isr()
 *      ↓
 * Generic subscriber callbacks
 *      ↓
 * Service thread wakeup / app notification
 * @endcode
 *
 * Application integration:
 * - User applications may override @c app_main()
 * - app_main() should create tasks, queues, timers, callbacks
 * - app_main() must return after initialization
 * - Scheduler starts only after app_main() completes
 *
 * Failure handling:
 * Any initialization failure causes boot to halt in an infinite loop and
 * the scheduler is never started.
 *
 * @dependencies
 * def_attributes.h, def_compiler.h, def_std.h,
 * device.h, board/board_config.h, board/irq_hw_init_generated.h,
 * drivers/drv_cpu.h, drivers/drv_irq.h, drivers/drv_rcc.h,
 * os/kernel.h, os/kernel_services.h, ipc/mqueue.h
 *
 * @note
 * - This file is the firmware entry point
 * - app_main() is weak and intended for user override
 * - FreeRTOS enables interrupts when scheduler starts
 * - Service threads are created before scheduler start
 *
 * @warning
 * - app_main() must not block forever
 * - If heap is insufficient scheduler may fail to start
 * - Incorrect IRQ priority setup can break RTOS behavior
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

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>

#include <device.h>
#include <board/board_config.h>
#include <board/irq_hw_init_generated.h>

#include <drivers/drv_cpu.h>
#include <drivers/drv_irq.h>
#include <drivers/drv_rcc.h>

#include <os/kernel.h>
#include <os/kernel_services.h>
#include <ipc/mqueue.h>

/* ── Weak application entry point ─────────────────────────────────────────── */

__WEAK 
int app_main(void) 
{ 
    return OS_ERR_NONE; 
}


/* ── OS entry point ───────────────────────────────────────────────────────── */
__SECTION_BOOT __USED 
void main(void)
{
    status_t os_init_status = OS_ERR_NONE;

    /** Driver level HAL Initialization: Vendor specific */
#if(CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

    HAL_StatusTypeDef hal_status = HAL_Init();

    if(hal_status == HAL_ERROR)
    {
        goto os_exit;
    }

#endif

    /** Drv Clock Init: Generic */
    os_init_status = drv_rcc_clock_init();

    if(os_init_status == OS_ERR_OP)
    {
        goto os_exit;
    }

    /*     Enable all RCC clocks (sys bus, peripherals, GPIO ports) so every
     *     HAL_xxx_MspInit() can configure GPIO and NVIC without enabling clocks
     *     itself.  Must run before irq_hw_init_all() and before any peripheral
     *     management thread calls HAL_xxx_Init(). */
    board_clk_enable();

     /*   Configure NVIC group bits required by FreeRTOS before any
     *    interrupt-driven peripheral is initialised. */
    drv_cpu_interrupt_prio_set();

    
    /*     Initialise irq_desc table and bind irq_chip to all hardware-backed
     *     software IRQ IDs.  Must run before any request_irq() call.
     *     TIM1 (HAL tick) is already firing at this point; hal_timebase_stm32_irq_handler
     *     uses an early-boot fallback until handle_irq is populated here. */
    irq_hw_init_all();

    /*     Reset IPC queue list sentinel — each service registers its own
     *     ring-buffer queues during thread startup. */
    os_init_status = ipc_mqueue_init();

    if(os_init_status == OS_ERR_OP)
    {
        goto os_exit;
    }

    /*     Register all OS service tasks in one call.
     *     See services/kernel_service_core.c for the full registration table. */
    os_init_status = os_kernel_thread_register();

    if(os_init_status == OS_ERR_OP)
    {
        goto os_exit;
    }

    /*    Application layer.  app_main() creates tasks, optionally registers
     *    additional IRQ subscribers via irq_register(), then returns.
     *    Tasks are driven by the scheduler; app_main must not block. */
    os_init_status = app_main();

    if(os_init_status == OS_ERR_OP)
    {
        goto os_exit;
    }

    
    /*    Hand over to FreeRTOS.  Interrupts are enabled for the first time
     *    inside vPortSVCHandler when the first task context loads. */
    vTaskStartScheduler();


    /** If any of the initialization is failed do not start OS */
    
    os_exit:

    while (true) 
    {

    }
}
