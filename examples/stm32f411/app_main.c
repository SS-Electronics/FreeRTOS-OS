/*
 * examples/stm32f411/app_main.c — Standalone FreeRTOS-OS example for the
 * STM32F411 devboard.
 *
 * Board: STM32F411 Devboard (STM32F411VET6, Cortex-M4F)
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
 *   make dev-stm32f411              full build (gen + config + clean + compile)
 *   make dev-stm32f411-gen          regenerate board files only
 *
 * Flash
 * ─────
 *   make dev-stm32f411-flash
 */

#include <os/kernel.h>
#include <os/kernel_syscall.h>
#include <services/gpio_mgmt.h>
#include <board/board_device_ids.h>
#include <irq/irq_desc.h>

/* ── Task parameters ─────────────────────────────────────────────────────── */

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

/* ── Heartbeat ───────────────────────────────────────────────────────────── */

static void heartbeat_task(void *param)
{
    (void)param;
    uint32_t tick = 0;
    printk("[boot] FreeRTOS-OS running\n");
    for (;;) {
        gpio_mgmt_post(LED_BOARD, GPIO_MGMT_CMD_TOGGLE, 0, 0);
        printk("[heartbeat] tick %lu\n", (unsigned long)tick++);
        os_thread_delay(HEARTBEAT_PERIOD);
    }
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int app_main(void)
{
    os_thread_create(heartbeat_task, "heartbeat",
                     HEARTBEAT_STACK, HEARTBEAT_PRIO, NULL);

    os_thread_create(uart_echo_task, "uart_echo",
                     UART_ECHO_STACK, UART_ECHO_PRIO, NULL);

    return 0;
}
