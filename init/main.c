/*
 * main.c — OS entry point
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Boot sequence
 * ─────────────
 *
 *  Reset_Handler  (startup_stm32f411vetx.c)
 *    SP ← _estack  |  SystemInit()  |  .data copy  |  .bss zero  |  main()
 *           │
 *           ▼
 *  main()  ← this file
 *    HAL_Init()                 — HAL timebase (TIM1), global tick
 *    drv_rcc_clock_init()       — PLL: HSI 16 MHz → 100 MHz SYSCLK
 *    drv_cpu_interrupt_prio_set()  — NVIC group bits before any IRQ enabled
 *    irq_hw_init_all()          — irq_desc table + irq_chip bindings for all HW IRQs
 *    ipc_queues_init()          — ring-buffer queues for UART RX/TX channels
 *    os_kernel_thread_register() — all OS service tasks (see kernel_service_core.c)
 *    printk_init()              — binds printk to UART TX ring buffer
 *    app_main()                 — creates application tasks (may subscribe IRQs)
 *    vTaskStartScheduler()      — interrupts enabled inside vPortSVCHandler
 *
 *  Generic IRQ chain (after scheduler starts)
 *  ──────────────────────────────────────────
 *
 *  [startup vector table]  weak alias  →  Default_Handler
 *           │ overridden by strong symbol
 *           ▼
 *  [irq_periph_dispatch_generated.c]  USART1_IRQHandler / I2C1_EV_IRQHandler / SPI1_IRQHandler …
 *           │  → hal_uart_stm32_irq_handler / hal_iic_stm32_ev_irq_handler …
 *           ▼
 *  [hal_uart/iic/spi_stm32.c]  HAL_UART_RxCpltCallback / HAL_I2C_MasterTxCpltCallback …
 *           │  → irq_notify_from_isr(IRQ_ID_UART_RX(n) | IRQ_ID_I2C_TX_DONE(n) | …)
 *           ▼
 *  [irq/irq.c]  generic pub/sub dispatch to all registered subscribers
 *           │
 *           ├──► uart_mgmt: _uart_rx_cb    → ringbuffer_putchar(rb, byte)
 *           ├──► iic_mgmt:  _iic_done_cb   → vTaskNotifyGiveFromISR(mgmt_task)
 *           ├──► spi_mgmt:  _spi_done_cb   → vTaskNotifyGiveFromISR(mgmt_task)
 *           └──► app code:  user callback  → vTaskNotifyGiveFromISR(app_task)
 */

#include <device.h>
#include <board/mcu_config.h>
#include <ipc/mqueue.h>
#include <ipc/global_var.h>
#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <os/kernel_services.h>
#include <drivers/cpu/drv_cpu.h>
#include <drivers/drv_irq.h>
#include <drivers/drv_rcc.h>
#include <board/board_config.h>
#include <board/irq_hw_init_generated.h>


/* ── Weak application entry point ─────────────────────────────────────────── */

__attribute__((weak)) int app_main(void) { return 0; }


/* ── IPC message-queue registration for all enabled peripherals ───────────── */

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
    for (uint8_t i = 0; i < NO_OF_IIC; i++)
    {
        global_iic_tx_mqueue_list[i] = 0;
        global_iic_rx_mqueue_list[i] = 0;
    }
#endif
}


/* ── OS entry point ───────────────────────────────────────────────────────── */

__SECTION_BOOT __USED void main(void)
{
    /* 1. HAL init and system clock.  All peripheral clocks are derived from
     *    SYSCLK so this must complete before any peripheral is touched. */
    HAL_Init();
    drv_rcc_clock_init();

    /* 1a. Enable all RCC clocks (sys bus, peripherals, GPIO ports) so every
     *     HAL_xxx_MspInit() can configure GPIO and NVIC without enabling clocks
     *     itself.  Must run before irq_hw_init_all() and before any peripheral
     *     management thread calls HAL_xxx_Init(). */
    board_clk_enable();

    /* 2. Configure NVIC group bits required by FreeRTOS before any
     *    interrupt-driven peripheral is initialised. */
    drv_cpu_interrupt_prio_set();

    /* 2a. Initialise irq_desc table and bind irq_chip to all hardware-backed
     *     software IRQ IDs.  Must run before any request_irq() call.
     *     TIM1 (HAL tick) is already firing at this point; hal_timebase_stm32_irq_handler
     *     uses an early-boot fallback until handle_irq is populated here. */
    irq_hw_init_all();

    /* 3. IPC ring-buffer queues — UART mgmt reads ring-buffer handles after
     *    its startup delay, so these must be allocated first. */
    ipc_queues_init();

    /* 4. Register all OS service tasks in one call.
     *    See services/kernel_service_core.c for the full registration table. */
    os_kernel_thread_register();

    /* 5. Bind printk() to the TX ring-buffer of COMM_PRINTK_HW_ID.
     *    Must be called after ipc_queues_init() registers the queue. */
    printk_init();

    /* 6. Application layer.  app_main() creates tasks, optionally registers
     *    additional IRQ subscribers via irq_register(), then returns.
     *    Tasks are driven by the scheduler; app_main must not block. */
    app_main();

    /* 7. Hand over to FreeRTOS.  Interrupts are enabled for the first time
     *    inside vPortSVCHandler when the first task context loads. */
    vTaskStartScheduler();

    while (1) {}
}
