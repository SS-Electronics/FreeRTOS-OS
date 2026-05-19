/**
 * @file        safe_state.h
 * @brief       Safe-state manager — coordinated system shutdown and reset
 * @ingroup     safety
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Safety
 * @info        Medical-grade safety stack: IWDG, per-task software watchdog, safe-state shutdown.
 * @dependency  STM32 HAL IWDG, .noinit RAM section
 *
 * @details
 * When a non-recoverable safety condition is detected (stack overflow,
 * watchdog miss, hard fault, malloc failure, …), the system must stop
 * producing patient-critical outputs and reset to a known-good state.
 *
 * safety_enter_safe_state() does the following atomically:
 *   1. Disables all interrupts (__disable_irq)
 *   2. Persists the reason code in .noinit RAM (survives NVIC soft-reset)
 *   3. Emits a minimal direct-register UART message (no HAL, no RTOS)
 *   4. Issues NVIC system reset via SCB->AIRCR
 *
 * On the next boot, main() calls safety_was_safe_state_reset() to detect
 * whether the previous session ended in safe state and logs the reason.
 *
 * Thread safety: safety_enter_safe_state() may be called from any context
 * (task, ISR, fault handler) — it disables interrupts on entry.
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

#ifndef FREERTOS_OS_INCLUDE_SAFETY_SAFE_STATE_H_
#define FREERTOS_OS_INCLUDE_SAFETY_SAFE_STATE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Safe-state reason codes ─────────────────────────────────────────────── */

typedef enum
{
    SAFE_REASON_NONE        = 0,  /* no safe state (normal boot)      */
    SAFE_REASON_WATCHDOG    = 1,  /* software task watchdog missed     */
    SAFE_REASON_STACK_OVF   = 2,  /* FreeRTOS stack overflow hook      */
    SAFE_REASON_MALLOC_FAIL = 3,  /* FreeRTOS malloc failed hook       */
    SAFE_REASON_ASSERT      = 4,  /* configASSERT() failed             */
    SAFE_REASON_BOOT_FAIL   = 5,  /* fatal error during boot sequence  */
    SAFE_REASON_HW_FAULT    = 6,  /* hardware fault (HardFault etc.)   */
    SAFE_REASON_MANUAL      = 7,  /* explicit application request      */
} safe_state_reason_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief Enter safe state.  This function never returns.
 *
 * Disables interrupts, persists reason in .noinit, prints UART message,
 * then triggers NVIC system reset.
 *
 * @param reason  Why safe state was entered (stored across reset).
 */
__attribute__((noreturn))
void safety_enter_safe_state(safe_state_reason_t reason);

/**
 * @brief Check whether the previous reset originated from safe state.
 * @return 1 if yes, 0 if normal boot / power-on reset.
 */
uint32_t safety_was_safe_state_reset(void);

/**
 * @brief Return the reason code from the previous safe-state session.
 *        Valid only when safety_was_safe_state_reset() == 1.
 */
safe_state_reason_t safety_get_last_reason(void);

/**
 * @brief Clear the persistent safe-state record.
 *        Call after logging the previous reason at boot.
 */
void safety_clear_record(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_SAFETY_SAFE_STATE_H_ */
