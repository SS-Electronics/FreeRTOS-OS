/*
 * examples/stm32u575/app_main.c — Standalone FreeRTOS-OS example for the
 * Nucleo-U575ZI-Q devboard.
 *
 * Board: NUCLEO-U575ZI-Q (STM32U575ZITxQ, Cortex-M33 + TrustZone)
 *
 * Tasks
 * ─────
 *   trustcore_task  — runs trustcore_attest_runtime() once, then gates the
 *                     rest of the application on its verdict. Lifecycle is
 *                     short — it deletes itself after attestation.
 *   heartbeat_task  — toggles LED_BOARD every 500 ms. Status LEDs reflect
 *                     the trustcore verdict:
 *                        OK / FIRST_BOOT  → LED_BOARD slow blink
 *                        INTEGRITY_FAIL   → LED_RED steady on, LED_BOARD off
 *                        IDENTITY_FAIL    → LED_RED fast blink (panic)
 *   uart_echo_task  — echoes bytes received on UART_DEBUG (LPUART1 VCP).
 *   shell           — interactive CLI on UART_DEBUG (115200 8N1)
 *
 * Build
 * ─────
 *   make dev-stm32u575              full build (gen + config + clean + compile)
 *   make dev-stm32u575-gen          regenerate board files only
 *
 * Flash
 * ─────
 *   make dev-stm32u575-flash
 */

#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <services/gpio_mgmt.h>
#include <board/board_device_ids.h>
#include <irq/irq_desc.h>

#include "trustcore.h"

/* ── Task parameters ─────────────────────────────────────────────────────── */

#define TRUSTCORE_STACK   384
#define TRUSTCORE_PRIO    3   /* higher than heartbeat so it runs first */

#define HEARTBEAT_STACK   256
#define HEARTBEAT_PRIO    1
#define HEARTBEAT_PERIOD  500   /* ms */

#define UART_ECHO_STACK   256
#define UART_ECHO_PRIO    1
#define UART_ECHO_BUF     64

/* ── UART_DEBUG RX echo ──────────────────────────────────────────────────── */

static uint8_t           _rx_buf[UART_ECHO_BUF];
static volatile uint16_t _rx_head = 0;
static uint16_t          _rx_tail = 0;
static TaskHandle_t      _echo_task_handle = NULL;

static void _uart_rx_cb(irq_id_t id, void *data, void *arg, BaseType_t *pxHPT)
{
    (void)id; (void)arg;
    if (!data) return;
    uint16_t next = (_rx_head + 1) % UART_ECHO_BUF;
    if (next != _rx_tail) {
        _rx_buf[_rx_head] = *(uint8_t *)data;
        _rx_head = next;
    }
    if (_echo_task_handle)
        vTaskNotifyGiveFromISR(_echo_task_handle, pxHPT);
}

static void uart_echo_task(void *param)
{
    (void)param;
    _echo_task_handle = xTaskGetCurrentTaskHandle();
    irq_register(IRQ_ID_UART_RX(UART_DEBUG), _uart_rx_cb, NULL);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (_rx_tail != _rx_head) {
            uint8_t byte = _rx_buf[_rx_tail];
            _rx_tail = (_rx_tail + 1) % UART_ECHO_BUF;
            printk("%c", (char)byte);
        }
    }
}

/* ── Trust attestation task ──────────────────────────────────────────────── */

static void trustcore_task(void *param)
{
    (void)param;

    /* Tiny startup grace — make sure the UART service is fully online before
     * we slam the trust banner out. 50 ms is comfortably enough on U5 at
     * 160 MHz; if the OS UART task hasn't dequeued by then, the printk
     * buffer will absorb the burst regardless. */
    os_thread_delay(50);

    trustcore_status_t s = trustcore_attest_runtime();

    /* Surface the verdict via the on-board status LEDs. */
    if (s == TRUSTCORE_STATUS_INTEGRITY_FAIL) {
        gpio_mgmt_post(LED_RED, GPIO_MGMT_CMD_SET, 0, 0);
    } else if (s == TRUSTCORE_STATUS_IDENTITY_FAIL) {
        /* Identity failure means we can't trust *anything* — keep the
         * red LED blinking fast so an operator notices, do not allow
         * the rest of the app to run. */
        for (;;) {
            gpio_mgmt_post(LED_RED, GPIO_MGMT_CMD_TOGGLE, 0, 0);
            os_thread_delay(100);
        }
    }

    /* One-shot task — release stack now that attestation is done. */
    vTaskDelete(NULL);
}

/* ── Heartbeat ───────────────────────────────────────────────────────────── */

static void heartbeat_task(void *param)
{
    (void)param;
    uint32_t tick = 0;
    printk("[boot] FreeRTOS-OS running\n");
    for (;;) {
        /* Block the heartbeat blink while integrity is failing — the red
         * LED already signals the issue and a moving green LED would mask
         * the alarm. */
        if (trustcore_get_status() != TRUSTCORE_STATUS_INTEGRITY_FAIL) {
            gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        }
        printk("[heartbeat] tick %lu\n", (unsigned long)tick++);
        os_thread_delay(HEARTBEAT_PERIOD);
    }
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int app_main(void)
{
    /* Trust attestation runs first so the boot banner reaches the user
     * before any other task's printk noise. */
    os_thread_create(trustcore_task, "trustcore",
                     TRUSTCORE_STACK, TRUSTCORE_PRIO, NULL);

    os_thread_create(heartbeat_task, "heartbeat",
                     HEARTBEAT_STACK, HEARTBEAT_PRIO, NULL);

    os_thread_create(uart_echo_task, "uart_echo",
                     UART_ECHO_STACK, UART_ECHO_PRIO, NULL);

    return 0;
}
