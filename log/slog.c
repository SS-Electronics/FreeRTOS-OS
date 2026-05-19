/**
 * @file        slog.c
 * @brief       Structured severity logger implementation
 * @ingroup     log
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Logging
 * @info        32-entry ring-buffer structured logger with severity levels; safe pre-scheduler & from ISRs.
 * @dependency  Compiler intrinsics, optional printk echo
 *
 * @details
 * Ring buffer of SLOG_MAX_ENTRIES entries.  Writes are protected by a
 * minimal __disable_irq() / __enable_irq() window (< ~30 instructions)
 * so they are safe from task, ISR, and pre-scheduler contexts.
 *
 * Post-scheduler: ERROR and WARN entries are also forwarded to printk()
 * for immediate serial output, outside the disabled-interrupt window.
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

#include <log/slog.h>
#include <os/kernel_syscall.h>

#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <stdio.h>

#include <device.h>   /* HAL_GetTick() */

/* ── Ring buffer ─────────────────────────────────────────────────────────── */

static slog_entry_t _ring[SLOG_MAX_ENTRIES];

/* Head and tail are plain uint32_t — they wrap naturally at 2^32.
 * count = head - tail; head points to next-write slot.            */
static volatile uint32_t _head = 0U;
static volatile uint32_t _tail = 0U;

static volatile uint8_t  _initialised = 0U;

/* ── Level strings (4-char, NULL-terminated) ─────────────────────────────── */

static const char * const _lvl_str[4] = { "ERR", "WRN", "INF", "DBG" };

/* ── Public: init ────────────────────────────────────────────────────────── */

void slog_init(void)
{
    __disable_irq();
    memset(_ring, 0, sizeof(_ring));
    _head        = 0U;
    _tail        = 0U;
    _initialised = 1U;
    __enable_irq();
}

/* ── Public: write ───────────────────────────────────────────────────────── */

void slog(slog_level_t level, const char *module, const char *fmt, ...)
{
    if (!_initialised || (level > SLOG_DEBUG))
        return;

    /* ── Get timestamp before entering disabled region ── */
    uint32_t tick;
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        tick = (uint32_t)xTaskGetTickCount();
    else
        tick = HAL_GetTick();

    /* ── Format message ── */
    char msg_buf[SLOG_MSG_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, ap);
    va_end(ap);

    /* ── Write to ring buffer (minimal disabled-IRQ window) ── */
    __disable_irq();

    slog_entry_t *e = &_ring[_head % SLOG_MAX_ENTRIES];
    e->tick  = tick;
    e->level = level;

    size_t mlen = strlen(module);
    if (mlen >= SLOG_MODULE_LEN) mlen = SLOG_MODULE_LEN - 1U;
    memcpy(e->module, module, mlen);
    e->module[mlen] = '\0';

    memcpy(e->msg, msg_buf, SLOG_MSG_LEN - 1U);
    e->msg[SLOG_MSG_LEN - 1U] = '\0';

    _head++;
    if ((_head - _tail) > SLOG_MAX_ENTRIES)
        _tail = _head - SLOG_MAX_ENTRIES;   /* overwrite oldest on overflow */

    __enable_irq();

    /* ── Echo to printk for ERROR and WARN (task context only) ── */
    if ((level <= SLOG_WARN) &&
        (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
    {
        printk("[%s][%s][%5lu] %s\n",
               _lvl_str[(int)level],
               e->module,
               (unsigned long)tick,
               e->msg);
    }
}

/* ── Public: dump ────────────────────────────────────────────────────────── */

uint32_t slog_dump(slog_entry_t *out_buf, uint32_t max_entries)
{
    if (!out_buf || (max_entries == 0U))
        return 0U;

    __disable_irq();
    uint32_t count = _head - _tail;
    if (count > max_entries) count = max_entries;
    uint32_t start = _head - count;
    for (uint32_t i = 0U; i < count; i++)
        out_buf[i] = _ring[(start + i) % SLOG_MAX_ENTRIES];
    __enable_irq();

    return count;
}
