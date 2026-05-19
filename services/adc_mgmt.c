/**
 * @file        adc_mgmt.c
 * @brief       ADC management service thread
 * @ingroup     services
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Services
 * @info        FreeRTOS service threads that own peripherals (UART/I2C/SPI/GPIO/ADC) and accept commands.
 * @dependency  Driver layer, ipc/mqueue, board config
 *
 * @details
 * Centralised ADC management service running as a FreeRTOS thread.
 * Follows the uart_mgmt pattern: register → subscribe → decimate → ring buffer.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * IRQ FLOW (Hardware → Application Layer)
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *   startup (vector table)
 *         │
 *         ▼
 *   irq_periph_dispatch_generated.c
 *         │  ADC_IRQHandler()
 *         ▼
 *   hal_adc_stm32_irq_handler()
 *         │  reads DR, clears flags
 *         ▼
 *   drv_irq_dispatch_from_isr(IRQ_ID_ADC_DATA_READY(n), &raw, &hpt)
 *         │
 *         ▼
 *   _adc_data_cb   (registered by adc_mgmt_thread)
 *         │  xQueueSendFromISR → _sample_queue
 *         ▼
 *   adc_mgmt_thread
 *         │  decimates to ADC_TARGET_SAMPLE_RATE_HZ
 *         ▼
 *   ring buffer  →  adc_mgmt_read_sample() (app layer)
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * DECIMATION
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * ADC runs in continuous mode at maximum conversion rate (~290 kSPS @ 16 MHz
 * ADC clock with 47.5+7.5=55 cycle conversion time on H7).
 *
 * The management thread accepts every Nth sample where N is computed as:
 *   N = adc_actual_rate_hz / ADC_TARGET_SAMPLE_RATE_HZ
 *
 * For simplicity the actual rate is estimated at compile time as
 * ADC_ACTUAL_RATE_HZ (set to 250000 as a conservative default).
 * Adjust this constant to match actual ADC rate on the target.
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

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <conf_os.h>
#include <services/adc_mgmt.h>

#include <os/kernel.h>
#include <drivers/drv_adc.h>
#include <board/board_config.h>
#include <ipc/ringbuffer.h>
#include <irq/irq_notify.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#   include <drivers/hal/stm32/hal_adc_stm32.h>
#endif

#if defined(BOARD_ADC_COUNT) && (BOARD_ADC_COUNT > 0)

/* ── Tune these for the target board ──────────────────────────────────── */
/* Estimated continuous ADC conversion rate (sps) — used for decimation.
 * At HSI 64 MHz, ADC clock = 64/4 = 16 MHz, conversion = ~55 cycles → ~290 kSPS.
 * Use a conservative 200 kSPS as the default. */
#ifndef ADC_ACTUAL_RATE_HZ
#define ADC_ACTUAL_RATE_HZ   200000U
#endif

/* Derived decimation ratio */
#define ADC_DECIMATE_N   (ADC_ACTUAL_RATE_HZ / ADC_TARGET_SAMPLE_RATE_HZ)

/* ── Per-channel state ────────────────────────────────────────────────── */
typedef struct {
    QueueHandle_t    raw_queue;       /* raw samples from ISR */
    struct ringbuffer *sample_rb;     /* decimated ring buffer for app */
    uint32_t          decimate_cnt;   /* ISR sample counter */
} adc_mgmt_ch_t;

__SECTION_OS_DATA __USED
static adc_mgmt_ch_t _ch[BOARD_ADC_COUNT];

/* Ring buffer storage (flat arrays behind the ringbuffer structs) */
__SECTION_OS_DATA __USED
static uint8_t _rb_storage[BOARD_ADC_COUNT][ADC_MGMT_RING_BUF_SIZE * sizeof(uint16_t)];

__SECTION_OS_DATA __USED
static struct ringbuffer _rb[BOARD_ADC_COUNT];

/* ── ISR callback — fires in interrupt context ────────────────────────── */

__SECTION_OS __USED
static void _adc_data_cb(irq_id_t id,
                         void    *data,
                         void    *arg,
                         BaseType_t *pxHPT)
{
    (void)id;
    adc_mgmt_ch_t *ch = (adc_mgmt_ch_t *)arg;
    if (ch == NULL || data == NULL)
        return;

    /* Decimate: only enqueue every Nth sample */
    ch->decimate_cnt++;
    if (ch->decimate_cnt < ADC_DECIMATE_N)
        return;
    ch->decimate_cnt = 0;

    /* Enqueue 12-bit value (stored as uint16_t) */
    uint32_t raw  = *(uint32_t *)data;
    uint16_t val  = (uint16_t)(raw & 0x0FFFU);

    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(ch->raw_queue, &val, &woken);
    if (pxHPT)
        *pxHPT |= woken;
}

/* ── Management thread ────────────────────────────────────────────────── */

__SECTION_OS __USED
static void adc_mgmt_thread(void *arg)
{
    (void)arg;

    /* ── Initialise ring buffers ── */
    for (uint8_t i = 0; i < BOARD_ADC_COUNT; i++) {
        ringbuffer_init(&_rb[i],
                        _rb_storage[i],
                        ADC_MGMT_RING_BUF_SIZE * sizeof(uint16_t));
        _ch[i].sample_rb   = &_rb[i];
        _ch[i].decimate_cnt = 0;
        _ch[i].raw_queue = xQueueCreate(ADC_MGMT_QUEUE_DEPTH, sizeof(uint16_t));
    }

    os_thread_delay(TIME_OFFSET_SERIAL_MANAGEMENT + 500U);

    /* ── Register ADC peripherals ── */
    const drv_adc_hal_ops_t *ops = NULL;

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    ops = hal_adc_stm32_get_ops();
#endif

    if (ops != NULL) {
        const board_config_t *bc = board_get_config();
        for (uint8_t i = 0; i < bc->adc_count; i++) {
            const board_adc_desc_t *d = &bc->adc_table[i];
            drv_adc_handle_t *h = drv_adc_get_handle(d->dev_id);

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
            hal_adc_stm32_set_config(h, d->instance, d->channel,
                                     d->resolution, d->sample_time);
#endif
            drv_adc_register(d->dev_id, ops);

            /* Subscribe to data-ready IRQ */
            irq_register(IRQ_ID_ADC_DATA_READY(d->dev_id),
                         _adc_data_cb,
                         &_ch[d->dev_id]);

            /* Start continuous conversion */
            drv_adc_start(d->dev_id);
        }
    }

    /* ── Main loop: drain raw_queue → ring buffer ── */
    while (true) {
        for (uint8_t i = 0; i < BOARD_ADC_COUNT; i++) {
            uint16_t sample;
            while (xQueueReceive(_ch[i].raw_queue, &sample, 0) == pdTRUE) {
                /* Push as two bytes (LE) into ring buffer */
                ringbuffer_putchar(_ch[i].sample_rb, (uint8_t)(sample & 0xFFU));
                ringbuffer_putchar(_ch[i].sample_rb, (uint8_t)(sample >> 8));
            }
        }
        os_thread_delay(1U); /* yield every 1 ms */
    }
}

/* ── Public API ───────────────────────────────────────────────────────── */

#ifndef PROC_SERVICE_ADC_MGMT_STACK_SIZE
#define PROC_SERVICE_ADC_MGMT_STACK_SIZE  512
#endif
#ifndef PROC_SERVICE_ADC_MGMT_PRIORITY
#define PROC_SERVICE_ADC_MGMT_PRIORITY    1
#endif

__SECTION_OS __USED
int32_t adc_mgmt_start(void)
{
    int32_t tid = os_thread_create(adc_mgmt_thread,
                                   "MGMT_ADC",
                                   PROC_SERVICE_ADC_MGMT_STACK_SIZE,
                                   PROC_SERVICE_ADC_MGMT_PRIORITY,
                                   NULL);
    return (tid >= 0) ? OS_ERR_NONE : OS_ERR_OP;
}

__SECTION_OS __USED
int32_t adc_mgmt_read_sample(uint8_t dev_id, uint16_t *sample)
{
    if (dev_id >= BOARD_ADC_COUNT || sample == NULL)
        return OS_ERR_OP;

    struct ringbuffer *rb = _ch[dev_id].sample_rb;
    if (rb == NULL)
        return OS_ERR_OP;

    uint8_t lo, hi;
    if (ringbuffer_getchar(rb, &lo) <= 0)
        return OS_ERR_OP;
    if (ringbuffer_getchar(rb, &hi) <= 0)
        return OS_ERR_OP;

    *sample = (uint16_t)lo | ((uint16_t)hi << 8);
    return OS_ERR_NONE;
}

__SECTION_OS __USED
uint32_t adc_mgmt_available(uint8_t dev_id)
{
    if (dev_id >= BOARD_ADC_COUNT || _ch[dev_id].sample_rb == NULL)
        return 0U;
    /* Ring buffer length in bytes / 2 bytes per uint16_t sample */
    return (uint32_t)(ringbuffer_data_len(_ch[dev_id].sample_rb) / 2U);
}

#endif /* BOARD_ADC_COUNT > 0 */
