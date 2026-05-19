/**
 * @file        safe_state.c
 * @brief       Safe-state manager implementation
 * @ingroup     safety
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Safety
 * @info        Medical-grade safety stack: IWDG, per-task software watchdog, safe-state shutdown.
 * @dependency  STM32 HAL IWDG, .noinit RAM section
 *
 * @details
 * Uses a .noinit structure to persist the reason code across NVIC soft-reset.
 * Uses direct UART register access — no HAL, no RTOS — for the shutdown
 * message so it works even in severely corrupted system states.
 *
 * H7 VCP:  USART3 at 0x40004800 (APB1), H7 ISR/TDR register layout.
 * F4 VCP:  USART2 at 0x40004400 (APB1), F4 SR/DR register layout.
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

#include <safety/safe_state.h>
#include <def_attributes.h>
#include <board/board_config.h>
#include <device.h>

/* ── Persistent record in .noinit RAM ────────────────────────────────────── */

#define SAFE_STATE_MAGIC   0xC0DEBEEFu

typedef struct
{
    uint32_t            magic;
    safe_state_reason_t reason;
    uint32_t            tick;      /* HAL_GetTick() at safe-state entry */
} _safe_state_record_t;

static _safe_state_record_t _record __SECTION_NOINIT;

/* ── Minimal UART register direct access ─────────────────────────────────── */

#if defined(STM32H7)
/* USART3 on H7, APB1 at 0x40004800. H7 uses ISR/TDR register names. */
#  define _SS_UART_BASE   0x40004800UL
#  define _SS_UART_ISR    (*(volatile uint32_t *)(_SS_UART_BASE + 0x1CU))
#  define _SS_UART_TDR    (*(volatile uint32_t *)(_SS_UART_BASE + 0x28U))
#  define _SS_UART_TXE    (1UL << 7)
#  define _SS_UART_TC     (1UL << 6)

static void _ss_putchar(char c)
{
    while (!(_SS_UART_ISR & _SS_UART_TXE)) {}
    _SS_UART_TDR = (uint32_t)(uint8_t)c;
}
static void _ss_flush(void)
{
    while (!(_SS_UART_ISR & _SS_UART_TC)) {}
}

#else  /* STM32F4 */
/* USART2 on F4, APB1 at 0x40004400. F4 uses SR/DR. */
#  define _SS_UART_BASE   0x40004400UL
#  define _SS_UART_SR     (*(volatile uint32_t *)(_SS_UART_BASE + 0x00U))
#  define _SS_UART_DR     (*(volatile uint32_t *)(_SS_UART_BASE + 0x04U))
#  define _SS_UART_TXE    (1UL << 7)
#  define _SS_UART_TC     (1UL << 6)

static void _ss_putchar(char c)
{
    while (!(_SS_UART_SR & _SS_UART_TXE)) {}
    _SS_UART_DR = (uint32_t)(uint8_t)c;
}
static void _ss_flush(void)
{
    while (!(_SS_UART_SR & _SS_UART_TC)) {}
}
#endif /* STM32H7 */

static void _ss_puts(const char *s)
{
    while (*s) _ss_putchar(*s++);
}

static void _ss_print_hex8(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    _ss_putchar('0'); _ss_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        _ss_putchar(hex[(v >> i) & 0xFU]);
}

/* ── Reason string table ─────────────────────────────────────────────────── */

static const char * const _reason_str[] = {
    "NONE",          /* 0 */
    "WATCHDOG",      /* 1 */
    "STACK_OVF",     /* 2 */
    "MALLOC_FAIL",   /* 3 */
    "ASSERT",        /* 4 */
    "BOOT_FAIL",     /* 5 */
    "HW_FAULT",      /* 6 */
    "MANUAL",        /* 7 */
};
#define _REASON_STR_COUNT  (sizeof(_reason_str) / sizeof(_reason_str[0]))

/* ── SCB AIRCR reset — direct register, no CMSIS stdlib dependency ─────── */

static void _sys_reset(void)
{
    /* DSB + ISB before the reset write */
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb");
    /* Write SYSRESETREQ with VECTKEY = 0x05FA */
    *(volatile uint32_t *)0xE000ED0CUL = (0x05FA0000UL | (1UL << 2));
    __asm volatile ("dsb 0xF" ::: "memory");
    for (;;) {}   /* wait for reset; unreachable */
}

/* ── Public API ──────────────────────────────────────────────────────────── */

__attribute__((noreturn))
void safety_enter_safe_state(safe_state_reason_t reason)
{
    /* Step 1: disable all interrupts — no RTOS, no HAL required from here */
    __asm volatile ("cpsid i" ::: "memory");

    /* Step 2: persist reason in .noinit so it survives the upcoming reset */
    _record.tick   = HAL_GetTick();
    _record.reason = reason;
    _record.magic  = SAFE_STATE_MAGIC;

    /* Step 3: flush a terse message to the VCP UART */
    _ss_puts("\r\n*** SAFE STATE: ");
    const char *rs = (reason < (safe_state_reason_t)_REASON_STR_COUNT)
                     ? _reason_str[(int)reason] : "UNKNOWN";
    _ss_puts(rs);
    _ss_puts(" (tick=");
    _ss_print_hex8(_record.tick);
    _ss_puts(") ***\r\n");
    _ss_flush();

    /* Step 4: reset */
    _sys_reset();
}

uint32_t safety_was_safe_state_reset(void)
{
    return (_record.magic == SAFE_STATE_MAGIC) ? 1U : 0U;
}

safe_state_reason_t safety_get_last_reason(void)
{
    return _record.reason;
}

void safety_clear_record(void)
{
    _record.magic  = 0U;
    _record.reason = SAFE_REASON_NONE;
    _record.tick   = 0U;
}
