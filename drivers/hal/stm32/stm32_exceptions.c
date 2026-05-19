/**
 * @file        stm32_exceptions.c
 * @brief       Cortex-M exception and fault handling — medical device grade
 * @ingroup     hal_stm32
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      STM32 HAL
 * @info        STM32-specific HAL backend implementing the generic driver vtables for STM32F4 / STM32H7.
 * @dependency  stm32f4xx-hal-driver, stm32h7xx-hal-driver
 *
 * @details
 * Provides strong overrides of ARM Cortex-M exception handlers for
 * STM32F4 and STM32H7 targets.  Key medical-grade features:
 *
 *  1. **Persistent fault record** — stored in `.noinit` RAM so the reason
 *     survives the subsequent NVIC soft-reset and can be logged at next boot.
 *
 *  2. **Direct-register UART output** — no HAL, no RTOS, no ring buffer.
 *     Works even in severely corrupted system states.
 *     H7 target: USART3 (PD8/PD9, VCP) — ISR/TDR register layout.
 *     F4 target: USART2 (PA2/PA3)       — SR/DR register layout.
 *
 *  3. **NVIC system reset** after flushing the fault message so the device
 *     attempts recovery rather than hanging indefinitely.
 *
 * Fault flow:
 * @code
 *   CPU exception trigger
 *        ↓
 *   naked handler (assembly trampoline)
 *     — selects MSP or PSP via EXC_RETURN bit 2
 *        ↓
 *   _fault_handler_c(frame*, exc_return, fault_type)
 *     — captures stacked regs + SCB fault status
 *     — stores fault_record_t in .noinit (_g_fault_record)
 *     — prints register dump over UART
 *        ↓
 *   NVIC_SystemReset (via SCB AIRCR)
 * @endcode
 *
 * Boot-time check:
 * @code
 *   if (fault_handler_was_fault_reset()) {
 *       const fault_record_t *r = fault_handler_get_last();
 *       // log r->pc, r->cfsr, r->fault_type …
 *       fault_handler_clear();
 *   }
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

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>
#include <safety/fault_handler.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

/* ── Fault type identifiers ──────────────────────────────────────────────── */

#define FAULT_TYPE_HARD    0U
#define FAULT_TYPE_MEM     1U
#define FAULT_TYPE_BUS     2U
#define FAULT_TYPE_USAGE   3U

/* ── Persistent fault record (survives soft reset) ───────────────────────── */

#define FAULT_RECORD_MAGIC  0xDEADC0DEu

typedef struct
{
    /* Validation */
    uint32_t magic;         /* FAULT_RECORD_MAGIC when valid */

    /* Stacked exception frame (auto-pushed by CPU at exception entry) */
    uint32_t r0, r1, r2, r3;
    uint32_t r12, lr, pc, xpsr;

    /* SCB fault status registers */
    uint32_t cfsr;    /* Configurable Fault Status  (MMFSR | BFSR | UFSR) */
    uint32_t hfsr;    /* HardFault Status                                   */
    uint32_t dfsr;    /* Debug Fault Status                                  */
    uint32_t mmar;    /* MemManage Address                                   */
    uint32_t bfar;    /* BusFault Address                                    */

    /* Runtime context */
    uint32_t tick;          /* HAL_GetTick() at fault time */
    uint32_t exc_return;    /* EXC_RETURN in LR at exception entry */
    uint32_t fault_type;    /* FAULT_TYPE_xxx */
} fault_record_t;

/* Placed in .noinit — NOT cleared by startup .bss zero loop */
static volatile fault_record_t _g_fault_record __SECTION_NOINIT;

/* ── UART direct-register output ─────────────────────────────────────────── */
/* Supports two targets:
 *   STM32H7 — USART3 at APB1 base 0x40004800, H7 ISR/TDR register map
 *   STM32F4 — USART2 at APB1 base 0x40004400, F4 SR/DR register map   */

#if defined(STM32H7)
#  define _FLT_UART_BASE   0x40004800UL   /* USART3 H7 VCP             */
#  define _FLT_UART_ISR    (*(volatile uint32_t *)(_FLT_UART_BASE + 0x1CU))
#  define _FLT_UART_TDR    (*(volatile uint32_t *)(_FLT_UART_BASE + 0x28U))
#  define _FLT_UART_TXE    (1UL << 7)
#  define _FLT_UART_TC     (1UL << 6)
static void _flt_putchar(char c)
{
    while (!(_FLT_UART_ISR & _FLT_UART_TXE)) {}
    _FLT_UART_TDR = (uint32_t)(uint8_t)c;
}
static void _flt_flush(void)
{
    while (!(_FLT_UART_ISR & _FLT_UART_TC)) {}
}
#else  /* STM32F4 */
#  define _FLT_UART_BASE   0x40004400UL   /* USART2 F4 debug/shell port */
#  define _FLT_UART_SR     (*(volatile uint32_t *)(_FLT_UART_BASE + 0x00U))
#  define _FLT_UART_DR     (*(volatile uint32_t *)(_FLT_UART_BASE + 0x04U))
#  define _FLT_UART_TXE    (1UL << 7)
#  define _FLT_UART_TC     (1UL << 6)
static void _flt_putchar(char c)
{
    while (!(_FLT_UART_SR & _FLT_UART_TXE)) {}
    _FLT_UART_DR = (uint32_t)(uint8_t)c;
}
static void _flt_flush(void)
{
    while (!(_FLT_UART_SR & _FLT_UART_TC)) {}
}
#endif /* STM32H7 */

static void _flt_puts(const char *s)
{
    while (*s) _flt_putchar(*s++);
}

static void _flt_hex32(uint32_t v)
{
    static const char _h[] = "0123456789ABCDEF";
    _flt_putchar('0'); _flt_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        _flt_putchar(_h[(v >> i) & 0xFU]);
}

/* ── SCB fault register bit decoders ────────────────────────────────────── */

static void _decode_hfsr(uint32_t hfsr)
{
    _flt_puts("  HFSR="); _flt_hex32(hfsr);
    _flt_puts("  [");
    if (hfsr & (1U << 31)) _flt_puts(" DEBUGEVT");
    if (hfsr & (1U << 30)) _flt_puts(" FORCED");
    if (hfsr & (1U <<  1)) _flt_puts(" VECTTBL");
    _flt_puts(" ]\r\n");
}

static void _decode_cfsr(uint32_t cfsr)
{
    _flt_puts("  CFSR="); _flt_hex32(cfsr); _flt_puts("\r\n");

    /* MMFSR (bits [7:0]) */
    uint32_t mm = cfsr & 0xFFU;
    if (mm)
    {
        _flt_puts("  MMFSR:[");
        if (mm & (1U << 7)) _flt_puts(" MMARVALID");
        if (mm & (1U << 5)) _flt_puts(" MLSPERR");
        if (mm & (1U << 4)) _flt_puts(" MSTKERR");
        if (mm & (1U << 3)) _flt_puts(" MUNSTKERR");
        if (mm & (1U << 1)) _flt_puts(" DACCVIOL");
        if (mm & (1U << 0)) _flt_puts(" IACCVIOL");
        _flt_puts(" ]\r\n");
    }

    /* BFSR (bits [15:8]) */
    uint32_t bfs = (cfsr >> 8U) & 0xFFU;
    if (bfs)
    {
        _flt_puts("  BFSR: [");
        if (bfs & (1U << 7)) _flt_puts(" BFARVALID");
        if (bfs & (1U << 5)) _flt_puts(" LSPERR");
        if (bfs & (1U << 4)) _flt_puts(" STKERR");
        if (bfs & (1U << 3)) _flt_puts(" UNSTKERR");
        if (bfs & (1U << 2)) _flt_puts(" IMPRECISERR");
        if (bfs & (1U << 1)) _flt_puts(" PRECISERR");
        if (bfs & (1U << 0)) _flt_puts(" IBUSERR");
        _flt_puts(" ]\r\n");
    }

    /* UFSR (bits [31:16]) */
    uint32_t ufs = (cfsr >> 16U) & 0xFFFFU;
    if (ufs)
    {
        _flt_puts("  UFSR: [");
        if (ufs & (1U << 9)) _flt_puts(" DIVBYZERO");
        if (ufs & (1U << 8)) _flt_puts(" UNALIGNED");
        if (ufs & (1U << 3)) _flt_puts(" NOCP");
        if (ufs & (1U << 2)) _flt_puts(" INVPC");
        if (ufs & (1U << 1)) _flt_puts(" INVSTATE");
        if (ufs & (1U << 0)) _flt_puts(" UNDEFINSTR");
        _flt_puts(" ]\r\n");
    }
}

/* ── Central C fault handler ─────────────────────────────────────────────── */
/* Called from naked trampoline — arguments in R0, R1, R2. */

__SECTION_OS __USED
void __attribute__((used, noinline))
_fault_handler_c(uint32_t *frame, uint32_t exc_return, uint32_t fault_type)
{
    /* Read SCB fault registers */
    uint32_t cfsr  = *(volatile uint32_t *)0xE000ED28U;
    uint32_t hfsr  = *(volatile uint32_t *)0xE000ED2CU;
    uint32_t dfsr  = *(volatile uint32_t *)0xE000ED30U;
    uint32_t mmar  = *(volatile uint32_t *)0xE000ED34U;
    uint32_t bfar  = *(volatile uint32_t *)0xE000ED38U;

    /* Persist full record in .noinit — survives the upcoming soft-reset */
    _g_fault_record.r0         = frame[0];
    _g_fault_record.r1         = frame[1];
    _g_fault_record.r2         = frame[2];
    _g_fault_record.r3         = frame[3];
    _g_fault_record.r12        = frame[4];
    _g_fault_record.lr         = frame[5];
    _g_fault_record.pc         = frame[6];
    _g_fault_record.xpsr       = frame[7];
    _g_fault_record.cfsr       = cfsr;
    _g_fault_record.hfsr       = hfsr;
    _g_fault_record.dfsr       = dfsr;
    _g_fault_record.mmar       = mmar;
    _g_fault_record.bfar       = bfar;
    _g_fault_record.tick       = HAL_GetTick();
    _g_fault_record.exc_return = exc_return;
    _g_fault_record.fault_type = fault_type;
    _g_fault_record.magic      = FAULT_RECORD_MAGIC;   /* mark valid last */

    /* Print minimal diagnostic over UART (direct register, no HAL) */
    _flt_puts("\r\n*** FAULT");
    if (fault_type == FAULT_TYPE_HARD)   _flt_puts(" HARD");
    if (fault_type == FAULT_TYPE_MEM)    _flt_puts(" MEM");
    if (fault_type == FAULT_TYPE_BUS)    _flt_puts(" BUS");
    if (fault_type == FAULT_TYPE_USAGE)  _flt_puts(" USAGE");
    _flt_puts(" ***\r\n");

    _flt_puts("  PC=");  _flt_hex32(_g_fault_record.pc);
    _flt_puts("  LR=");  _flt_hex32(_g_fault_record.lr);
    _flt_puts("  PSR="); _flt_hex32(_g_fault_record.xpsr);
    _flt_puts("\r\n");
    _flt_puts("  R0=");  _flt_hex32(_g_fault_record.r0);
    _flt_puts("  R1=");  _flt_hex32(_g_fault_record.r1);
    _flt_puts("  R2=");  _flt_hex32(_g_fault_record.r2);
    _flt_puts("  R3=");  _flt_hex32(_g_fault_record.r3);
    _flt_puts("\r\n");
    _flt_puts("  tick="); _flt_hex32(_g_fault_record.tick);
    _flt_puts("\r\n");

    _decode_hfsr(hfsr);
    _decode_cfsr(cfsr);

    if (cfsr & (1U <<  7)) { _flt_puts("  MMAR="); _flt_hex32(mmar); _flt_puts("\r\n"); }
    if (cfsr & (1U << 15)) { _flt_puts("  BFAR="); _flt_hex32(bfar); _flt_puts("\r\n"); }

    _flt_puts("  Resetting...\r\n");
    _flt_flush();

    /* NVIC system reset — DSB + write SYSRESETREQ to AIRCR */
    __asm volatile ("dsb 0xF" ::: "memory");
    *(volatile uint32_t *)0xE000ED0CU = (0x05FA0000UL | (1UL << 2));
    __asm volatile ("dsb 0xF" ::: "memory");
    for (;;) {}   /* unreachable */
}

/* ── Boot-time public API ────────────────────────────────────────────────── */

uint32_t fault_handler_was_fault_reset(void)
{
    return (_g_fault_record.magic == FAULT_RECORD_MAGIC) ? 1U : 0U;
}

const volatile fault_record_t *fault_handler_get_last(void)
{
    return &_g_fault_record;
}

void fault_handler_get_fields(uint32_t *out_pc,
                               uint32_t *out_lr,
                               uint32_t *out_cfsr,
                               uint32_t *out_hfsr,
                               uint32_t *out_fault_type,
                               uint32_t *out_tick)
{
    if (out_pc)         *out_pc         = _g_fault_record.pc;
    if (out_lr)         *out_lr         = _g_fault_record.lr;
    if (out_cfsr)       *out_cfsr       = _g_fault_record.cfsr;
    if (out_hfsr)       *out_hfsr       = _g_fault_record.hfsr;
    if (out_fault_type) *out_fault_type = _g_fault_record.fault_type;
    if (out_tick)       *out_tick       = _g_fault_record.tick;
}

void fault_handler_clear(void)
{
    _g_fault_record.magic = 0U;
}

/* ── Naked trampolines — ARM Cortex-M exception handlers ─────────────────── */

__SECTION_OS __USED
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst    lr, #4          \n"
        "ite    eq              \n"
        "mrseq  r0, msp         \n"
        "mrsne  r0, psp         \n"
        "mov    r1, lr          \n"
        "mov    r2, %0          \n"
        "b      _fault_handler_c\n"
        : : "i"(FAULT_TYPE_HARD));
}

__SECTION_OS __USED
__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst    lr, #4          \n"
        "ite    eq              \n"
        "mrseq  r0, msp         \n"
        "mrsne  r0, psp         \n"
        "mov    r1, lr          \n"
        "mov    r2, %0          \n"
        "b      _fault_handler_c\n"
        : : "i"(FAULT_TYPE_MEM));
}

__SECTION_OS __USED
__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile(
        "tst    lr, #4          \n"
        "ite    eq              \n"
        "mrseq  r0, msp         \n"
        "mrsne  r0, psp         \n"
        "mov    r1, lr          \n"
        "mov    r2, %0          \n"
        "b      _fault_handler_c\n"
        : : "i"(FAULT_TYPE_BUS));
}

__SECTION_OS __USED
__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile(
        "tst    lr, #4          \n"
        "ite    eq              \n"
        "mrseq  r0, msp         \n"
        "mrsne  r0, psp         \n"
        "mov    r1, lr          \n"
        "mov    r2, %0          \n"
        "b      _fault_handler_c\n"
        : : "i"(FAULT_TYPE_USAGE));
}

__SECTION_OS __USED
void NMI_Handler(void)
{
    _flt_puts("\r\n*** NMI ***\r\n");
    _flt_flush();
    for (;;) {}
}

__SECTION_OS __USED
void DebugMon_Handler(void)
{
    for (;;) {}
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
