/**
 * @file        wdog.h
 * @brief       Hardware IWDG + multi-task software watchdog (IEC 62304 safety)
 * @ingroup     safety
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Safety
 * @info        Medical-grade safety stack: IWDG, per-task software watchdog, safe-state shutdown.
 * @dependency  STM32 HAL IWDG, .noinit RAM section
 *
 * @details
 * Two-layer watchdog:
 *
 * 1. **Hardware IWDG** (STM32H7 IWDG1 / STM32F4 IWDG)
 *    - LSI clock / prescaler 64 → ~2 ms/tick
 *    - Reload 2000 → 4-second hardware reset if never refreshed
 *    - Started by wdog_hw_init() — CANNOT be stopped once running
 *
 * 2. **Software task watchdog** (WDOG_CHECK_PERIOD_MS cadence)
 *    - Each critical task holds one slot bit in a volatile bitmask
 *    - Tasks call wdog_task_kick(slot_id) to set their bit each period
 *    - The watchdog service thread verifies all required bits are set,
 *      then kicks IWDG and clears the mask
 *    - If WDOG_MAX_MISSED_CHECKS consecutive checks fail → safe state
 *
 * Application integration:
 * @code
 *   // In app_main(), after creating critical tasks:
 *   wdog_sw_init(WDOG_REQUIRED_MASK);   // register required slots
 *
 *   // In each critical task body (inside the main loop):
 *   wdog_task_kick(WDOG_SLOT_HEARTBEAT);
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

#ifndef FREERTOS_OS_INCLUDE_SAFETY_WDOG_H_
#define FREERTOS_OS_INCLUDE_SAFETY_WDOG_H_

#include <stdint.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Software watchdog slot IDs ──────────────────────────────────────────── */

#define WDOG_SLOT_HEARTBEAT    0U   /* heartbeat_task         */
#define WDOG_SLOT_ECG_ACQ      1U   /* ecg_acquire_task       */
#define WDOG_SLOT_ECG_INF      2U   /* ecg_infer_task         */
#define WDOG_SLOT_COUNT        16U  /* maximum supported slots */

/* Convenience mask — kick all three ECG application tasks */
#define WDOG_ECG_APP_MASK   ((1U << WDOG_SLOT_HEARTBEAT) | \
                             (1U << WDOG_SLOT_ECG_ACQ)   | \
                             (1U << WDOG_SLOT_ECG_INF))

/* ── Timing constants ────────────────────────────────────────────────────── */

/* Hardware IWDG timeout: prescaler/64 on 32 kHz LSI → 2 ms/tick → 4 s */
#define WDOG_HW_TIMEOUT_MS       4000U

/* Software check cadence — must be < WDOG_HW_TIMEOUT_MS */
#define WDOG_CHECK_PERIOD_MS     1000U

/* Startup grace period: allow tasks time to reach their first kick */
#define WDOG_STARTUP_DELAY_MS    5000U

/* Consecutive missed software checks before entering safe state */
#define WDOG_MAX_MISSED_CHECKS   3U

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief Initialise and start the hardware IWDG.
 * @warning Call BEFORE vTaskStartScheduler().  IWDG cannot be stopped.
 */
void    wdog_hw_init(void);

/**
 * @brief Register the software watchdog required-slot mask.
 * @param required_slot_mask  Bitmask of slots that MUST be kicked each period.
 *        Pass 0 to disable software watchdog (hardware IWDG still runs).
 */
void    wdog_sw_init(uint32_t required_slot_mask);

/**
 * @brief Called by a critical task to signal it is alive.
 * @param slot_id  Slot constant (WDOG_SLOT_xxx).  Out-of-range IDs are ignored.
 */
void    wdog_task_kick(uint32_t slot_id);

/**
 * @brief Create and start the watchdog service FreeRTOS task.
 * @return OS_ERR_NONE on success, OS_ERR_OP on task creation failure.
 */
int32_t wdog_service_start(void);

/**
 * @brief Return current software kick bitmask (last check snapshot).
 *        Intended for the health monitor / shell diagnostic.
 */
uint32_t wdog_get_kick_mask(void);

/**
 * @brief Return number of consecutive missed check cycles.
 */
uint32_t wdog_get_missed_checks(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_SAFETY_WDOG_H_ */
