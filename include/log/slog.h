/**
 * @file        slog.h
 * @brief       Structured severity logger for FreeRTOS-OS (IEC 62304 traceability)
 * @ingroup     log
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Logging
 * @info        32-entry ring-buffer structured logger with severity levels; safe pre-scheduler & from ISRs.
 * @dependency  Compiler intrinsics, optional printk echo
 *
 * @details
 * 32-entry circular ring buffer with severity levels, timestamps, and module
 * tagging.  Thread-safe via a minimal __disable_irq() window.
 *
 * Post-scheduler: ERROR and WARN entries are echoed to printk immediately.
 * Pre-scheduler (boot): entries are stored silently; dump with the shell
 * command  `log dump`  or via  slog_dump()  after UART service starts.
 *
 * Usage:
 * @code
 *   slog_init();
 *   LOG_I("BOOT", "System clock at %lu Hz", SystemCoreClock);
 *   LOG_E("WDOG", "Task heartbeat missed slot %u", slot);
 * @endcode
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

#ifndef FREERTOS_OS_INCLUDE_LOG_SLOG_H_
#define FREERTOS_OS_INCLUDE_LOG_SLOG_H_

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ───────────────────────────────────────────────────────── */

#define SLOG_MAX_ENTRIES   32U
#define SLOG_MSG_LEN       56U
#define SLOG_MODULE_LEN     8U

/* ── Severity levels ─────────────────────────────────────────────────────── */

typedef enum
{
    SLOG_ERROR = 0,   /* unrecoverable; triggers safe-state or reset */
    SLOG_WARN  = 1,   /* recoverable anomaly worth recording */
    SLOG_INFO  = 2,   /* normal operational milestone */
    SLOG_DEBUG = 3,   /* fine-grained diagnostic (filtered in release) */
} slog_level_t;

/* ── Per-entry record ────────────────────────────────────────────────────── */

typedef struct
{
    uint32_t      tick;                   /* xTaskGetTickCount() or HAL_GetTick() */
    slog_level_t  level;
    char          module[SLOG_MODULE_LEN]; /* e.g. "WDOG", "BOOT", "ADC " */
    char          msg[SLOG_MSG_LEN];
} slog_entry_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief Initialise log ring buffer.  Call once before any slog() calls.
 */
void     slog_init(void);

/**
 * @brief Write a formatted log entry.
 * @param level   Severity level.
 * @param module  Short tag string (truncated to SLOG_MODULE_LEN-1 chars).
 * @param fmt     printf-style format string.
 */
void     slog(slog_level_t level, const char *module, const char *fmt, ...);

/**
 * @brief Copy up to @p max_entries most-recent log entries into @p out_buf.
 * @return Number of entries actually copied.
 */
uint32_t slog_dump(slog_entry_t *out_buf, uint32_t max_entries);

/* ── Convenience macros ──────────────────────────────────────────────────── */

#define LOG_E(mod, fmt, ...)  slog(SLOG_ERROR, (mod), (fmt), ##__VA_ARGS__)
#define LOG_W(mod, fmt, ...)  slog(SLOG_WARN,  (mod), (fmt), ##__VA_ARGS__)
#define LOG_I(mod, fmt, ...)  slog(SLOG_INFO,  (mod), (fmt), ##__VA_ARGS__)
#define LOG_D(mod, fmt, ...)  slog(SLOG_DEBUG, (mod), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_LOG_SLOG_H_ */
