/*
 * examples/stm32h723/app_main.c — Standalone FreeRTOS-OS example for the
 * STM32H723 devboard.
 *
 * Board: NUCLEO-H723ZG (STM32H723ZGTx, Cortex-M7)
 *
 * Tasks
 * ─────
 *   heartbeat_task  — toggles LED_BOARD every 500 ms to confirm the RTOS is running
 *   uart_echo_task  — echoes bytes received on UART_DEBUG back to the sender
 *   shell           — interactive CLI on UART_APP (115200 8N1)
 *                     connect with any serial terminal
 *
 * Build
 * ─────
 *   make dev-stm32h723              full build (gen + config + clean + compile)
 *   make dev-stm32h723-gen          regenerate board files only
 *
 * Flash
 * ─────
 *   make dev-stm32h723-flash
 */

#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <services/gpio_mgmt.h>
#include <board/board_device_ids.h>
#include <autoconf.h>

#if defined(CONFIG_NET)
#include <net/net_service.h>
#endif

/* ── Task parameters ─────────────────────────────────────────────────────── */

#define HEARTBEAT_STACK   256
#define HEARTBEAT_PRIO    1
#define HEARTBEAT_PERIOD  500   /* ms */

/* ── Heartbeat ───────────────────────────────────────────────────────────── */

static void heartbeat_task(void *param)
{
    (void)param;
    printk("[boot] FreeRTOS-OS running\n");
    for (;;) {
        gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        os_thread_delay(HEARTBEAT_PERIOD);
    }
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int app_main(void)
{
    os_thread_create(heartbeat_task, "heartbeat",
                     HEARTBEAT_STACK, HEARTBEAT_PRIO, NULL);

#if defined(CONFIG_NET)
    /* Bring up lwIP + static-IP netif + RX poll thread. A PHY/link failure is
     * logged inside the stack but must not abort the rest of the boot. */
    if (net_service_start() != OS_ERR_NONE)
    {
        printk("[net] stack init failed (check Ethernet link)\n");
    }
#endif

    return 0;
}
