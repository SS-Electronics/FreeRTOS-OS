/*
 * main.c — OS entry point
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Boot and IRQ flow
 * ─────────────────
 *
 *  Reset_Handler  (startup_stm32f411vetx.c)
 *    SP ← _estack  |  SystemInit()  |  .data copy  |  .bss zero  |  main()
 *           │
 *           ▼
 *  main()  ← this file
 *    HAL_Init()                 — HAL timebase (TIM1), global tick
 *    drv_rcc_clock_init()       — PLL: HSI 16 MHz → 100 MHz SYSCLK
 *    drv_cpu_interrupt_prio_set()  — NVIC group bits before any IRQ enabled
 *    ipc_queues_init()          — ring-buffer queues for UART RX/TX channels
 *    uart_mgmt_start()          — UART mgmt task; subscribes IRQ_ID_UART_RX(n)
 *    iic_mgmt_start()           — I2C  mgmt task; subscribes IRQ_ID_I2C_*
 *    spi_mgmt_start()           — SPI  mgmt task; subscribes IRQ_ID_SPI_*
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
 *  [hal_it_stm32.c]  USART1_IRQHandler / I2C1_EV_IRQHandler / SPI1_IRQHandler …
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
#include <drivers/cpu/drv_cpu.h>
#include <drivers/drv_rcc.h>
#include <services/uart_mgmt.h>

#if (INC_SERVICE_IIC_MGMT == 1)
#  include <services/iic_mgmt.h>
#endif

#if (BOARD_SPI_COUNT > 0)
#  include <services/spi_mgmt.h>
#endif


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

    /* 2. Configure NVIC group bits required by FreeRTOS before any
     *    interrupt-driven peripheral is initialised. */
    drv_cpu_interrupt_prio_set();

    /* 3. IPC ring-buffer queues — UART mgmt reads ring-buffer handles after
     *    its startup delay, so these must be allocated first. */
    ipc_queues_init();

    /* 4. Start management threads.
     *
     *    Each thread:
     *      a. Initialises the peripheral hardware (after a startup delay).
     *      b. Subscribes to the generic IRQ notification framework so
     *         ISR-level events (RX byte, transfer-done, error) are delivered
     *         as pub/sub callbacks without polling.
     *
     *    UART: _uart_rx_cb  feeds bytes into the RX ring buffer (ISR-safe).
     *    I2C:  _iic_done_cb wakes the mgmt thread via task notification.
     *    SPI:  _spi_done_cb wakes the mgmt thread via task notification.
     */
    uart_mgmt_start();

#if (INC_SERVICE_IIC_MGMT == 1)
    iic_mgmt_start();
#endif

#if (BOARD_SPI_COUNT > 0)
    spi_mgmt_start();
#endif

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
