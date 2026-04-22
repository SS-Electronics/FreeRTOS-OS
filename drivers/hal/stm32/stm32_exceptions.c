/**
 * @file    stm32_exceptions.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   Cortex-M4 core exception handlers for STM32
 *
 * @details
 * Strong definitions for Cortex-M core exception handlers that override the
 * weak symbols in startup_stm32f411vetx.c.
 *
 * Fault handler chain (HardFault / MemManage / BusFault / UsageFault):
 *
 *   CPU fires exception
 *        │
 *        ▼  naked trampoline (_Handler)
 *   determine active SP from EXC_RETURN bit[2]:
 *     bit[2]==0 → MSP was active  (running in handler or MSP task)
 *     bit[2]==1 → PSP was active  (running in a FreeRTOS task)
 *        │
 *        ▼  _fault_handler_c(frame*, exc_return, fault_type)
 *   capture hardware exception frame (r0–r3, r12, lr, pc, xpsr)
 *   capture SCB->HFSR, CFSR, BFAR, MMFAR
 *   write g_fault_info  (visible in debugger / crash dump)
 *   emit diagnostic text directly via USART2 (no ring buffer, no RTOS)
 *   halt in spin loop
 *
 * @note
 * This module is compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM.
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <stdint.h>
#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

/* ── Fault type identifiers ─────────────────────────────────────────────── */

#define FAULT_TYPE_HARD     0U
#define FAULT_TYPE_MEM      1U
#define FAULT_TYPE_BUS      2U
#define FAULT_TYPE_USAGE    3U

/* ── Hardware exception frame pushed by the Cortex-M4 CPU ──────────────── */

typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;   /* link register of the faulting context */
    uint32_t pc;   /* program counter at the fault */
    uint32_t xpsr;
} fault_frame_t;

/* ── Fault diagnostic record — visible in debugger / RAM dump ───────────── */

typedef struct
{
    fault_frame_t frame;
    uint32_t      hfsr;        /* SCB->HFSR  */
    uint32_t      cfsr;        /* SCB->CFSR  (UFSR|BFSR|MMFSR) */
    uint32_t      bfar;        /* SCB->BFAR  (valid when BFARVALID) */
    uint32_t      mmfar;       /* SCB->MMFAR (valid when MMARVALID) */
    uint32_t      exc_return;  /* LR value inside handler (EXC_RETURN) */
    uint32_t      fault_type;  /* FAULT_TYPE_* */
} fault_info_t;

volatile fault_info_t g_fault_info;

/* ── Direct USART2 output — safe in fault context ───────────────────────── */
/*
 * USART2 APB1 base: 0x40004400
 * SR  offset 0x00  — bit 7 TXE, bit 6 TC
 * DR  offset 0x04
 */

#define USART2_BASE  0x40004400UL
#define USART2_SR    (*(volatile uint32_t *)(USART2_BASE + 0x00U))
#define USART2_DR    (*(volatile uint32_t *)(USART2_BASE + 0x04U))
#define USART2_TXE   (1UL << 7)
#define USART2_TC    (1UL << 6)

static void _fault_putchar(char c)
{
    while (!(USART2_SR & USART2_TXE))
        ;
    USART2_DR = (uint32_t)(uint8_t)c;
}

static void _fault_puts(const char *s)
{
    while (*s)
        _fault_putchar(*s++);
}

static void _fault_print_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    _fault_putchar('0');
    _fault_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        _fault_putchar(hex[(v >> i) & 0xFU]);
}

static void _fault_print_dec(uint32_t v)
{
    if (v == 0) { _fault_putchar('0'); return; }
    char buf[10];
    int  n = 0;
    while (v) { buf[n++] = (char)('0' + v % 10U); v /= 10U; }
    while (n > 0) { n--; _fault_putchar(buf[n]); }
}

/* ── CFSR / HFSR bit decoders ───────────────────────────────────────────── */

static void _decode_mmfsr(uint32_t cfsr)
{
    uint32_t m = cfsr & 0xFFU;
    if (!m) return;
    _fault_puts("  MMFSR:");
    if (m & (1U << 7)) _fault_puts(" MMARVALID");
    if (m & (1U << 5)) _fault_puts(" MLSPERR");
    if (m & (1U << 4)) _fault_puts(" MSTKERR");
    if (m & (1U << 3)) _fault_puts(" MUNSTKERR");
    if (m & (1U << 1)) _fault_puts(" DACCVIOL");
    if (m & (1U << 0)) _fault_puts(" IACCVIOL");
    _fault_puts("\r\n");
}

static void _decode_bfsr(uint32_t cfsr)
{
    uint32_t b = (cfsr >> 8U) & 0xFFU;
    if (!b) return;
    _fault_puts("  BFSR:");
    if (b & (1U << 7)) _fault_puts(" BFARVALID");
    if (b & (1U << 5)) _fault_puts(" LSPERR");
    if (b & (1U << 4)) _fault_puts(" STKERR");
    if (b & (1U << 3)) _fault_puts(" UNSTKERR");
    if (b & (1U << 2)) _fault_puts(" IMPRECISERR");
    if (b & (1U << 1)) _fault_puts(" PRECISERR");
    if (b & (1U << 0)) _fault_puts(" IBUSERR");
    _fault_puts("\r\n");
}

static void _decode_ufsr(uint32_t cfsr)
{
    uint32_t u = (cfsr >> 16U) & 0xFFFFU;
    if (!u) return;
    _fault_puts("  UFSR:");
    if (u & (1U << 9)) _fault_puts(" DIVBYZERO");
    if (u & (1U << 8)) _fault_puts(" UNALIGNED");
    if (u & (1U << 3)) _fault_puts(" NOCP");
    if (u & (1U << 2)) _fault_puts(" INVPC");
    if (u & (1U << 1)) _fault_puts(" INVSTATE");
    if (u & (1U << 0)) _fault_puts(" UNDEFINSTR");
    _fault_puts("\r\n");
}

static void _decode_hfsr(uint32_t hfsr)
{
    _fault_puts("  HFSR:");
    if (hfsr & (1U << 31)) _fault_puts(" DEBUGEVT");
    if (hfsr & (1U << 30)) _fault_puts(" FORCED");
    if (hfsr & (1U <<  1)) _fault_puts(" VECTBL");
    _fault_puts("\r\n");
}

/* ── Central fault C handler ─────────────────────────────────────────────── */

static const char *_fault_names[] = {
    "HardFault", "MemManage", "BusFault", "UsageFault"
};

void __attribute__((used, noinline)) _fault_handler_c(uint32_t *frame, uint32_t exc_return, uint32_t fault_type)
{
    /* Capture SCB fault registers before anything can modify them */
    uint32_t hfsr  = *(volatile uint32_t *)0xE000ED2CUL;
    uint32_t cfsr  = *(volatile uint32_t *)0xE000ED28UL;
    uint32_t mmfar = *(volatile uint32_t *)0xE000ED34UL;
    uint32_t bfar  = *(volatile uint32_t *)0xE000ED38UL;

    /* Fill diagnostic record */
    g_fault_info.frame.r0       = frame[0];
    g_fault_info.frame.r1       = frame[1];
    g_fault_info.frame.r2       = frame[2];
    g_fault_info.frame.r3       = frame[3];
    g_fault_info.frame.r12      = frame[4];
    g_fault_info.frame.lr       = frame[5];
    g_fault_info.frame.pc       = frame[6];
    g_fault_info.frame.xpsr     = frame[7];
    g_fault_info.hfsr           = hfsr;
    g_fault_info.cfsr           = cfsr;
    g_fault_info.bfar           = bfar;
    g_fault_info.mmfar          = mmfar;
    g_fault_info.exc_return     = exc_return;
    g_fault_info.fault_type     = fault_type;

    /* Emit diagnostic over USART2 */
    _fault_puts("\r\n\r\n*** ");
    _fault_puts(fault_type < 4U ? _fault_names[fault_type] : "Fault");
    _fault_puts(" ***\r\n");

    _fault_puts("PC   : "); _fault_print_hex32(frame[6]); _fault_puts("\r\n");
    _fault_puts("LR   : "); _fault_print_hex32(frame[5]); _fault_puts("\r\n");
    _fault_puts("SP   : "); _fault_print_hex32((uint32_t)frame); _fault_puts("\r\n");
    _fault_puts("R0   : "); _fault_print_hex32(frame[0]); _fault_puts("\r\n");
    _fault_puts("R1   : "); _fault_print_hex32(frame[1]); _fault_puts("\r\n");
    _fault_puts("R2   : "); _fault_print_hex32(frame[2]); _fault_puts("\r\n");
    _fault_puts("R3   : "); _fault_print_hex32(frame[3]); _fault_puts("\r\n");
    _fault_puts("R12  : "); _fault_print_hex32(frame[4]); _fault_puts("\r\n");
    _fault_puts("xPSR : "); _fault_print_hex32(frame[7]); _fault_puts("\r\n");

    _fault_puts("HFSR : "); _fault_print_hex32(hfsr); _fault_puts("\r\n");
    _fault_puts("CFSR : "); _fault_print_hex32(cfsr); _fault_puts("\r\n");

    _decode_hfsr(hfsr);
    _decode_mmfsr(cfsr);
    _decode_bfsr(cfsr);
    _decode_ufsr(cfsr);

    if ((cfsr >> 8U) & (1U << 7)) /* BFARVALID */
    {
        _fault_puts("BFAR : "); _fault_print_hex32(bfar); _fault_puts("\r\n");
    }
    if (cfsr & (1U << 7)) /* MMARVALID */
    {
        _fault_puts("MMFAR: "); _fault_print_hex32(mmfar); _fault_puts("\r\n");
    }

    _fault_puts("EXC_RETURN: "); _fault_print_hex32(exc_return);
    _fault_puts((exc_return & 4U) ? "  (PSP)\r\n" : "  (MSP)\r\n");

    /* Wait for last byte to drain */
    while (!(USART2_SR & USART2_TC))
        ;

    for (;;)
        ;  /* halt — attach debugger, inspect g_fault_info */
}

/* ── NMI ────────────────────────────────────────────────────────────────── */

void NMI_Handler(void)
{
    _fault_puts("\r\n*** NMI ***\r\n");
    while (!(USART2_SR & USART2_TC))
        ;
    for (;;)
        ;
}

/* ── Fault trampolines ───────────────────────────────────────────────────── */
/*
 * Each naked trampoline:
 *   1. Tests EXC_RETURN bit[2] to pick MSP or PSP as the exception frame ptr.
 *   2. Passes it as r0, EXC_RETURN as r1, fault type constant as r2.
 *   3. Branches (not calls) to _fault_handler_c — it never returns.
 */

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst  lr, #4          \n"
        "ite  eq              \n"
        "mrseq r0, msp        \n"
        "mrsne r0, psp        \n"
        "mov  r1, lr          \n"
        "mov  r2, %0          \n"
        "b    _fault_handler_c\n"
        :
        : "i" (FAULT_TYPE_HARD)
    );
}

__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst  lr, #4          \n"
        "ite  eq              \n"
        "mrseq r0, msp        \n"
        "mrsne r0, psp        \n"
        "mov  r1, lr          \n"
        "mov  r2, %0          \n"
        "b    _fault_handler_c\n"
        :
        : "i" (FAULT_TYPE_MEM)
    );
}

__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile(
        "tst  lr, #4          \n"
        "ite  eq              \n"
        "mrseq r0, msp        \n"
        "mrsne r0, psp        \n"
        "mov  r1, lr          \n"
        "mov  r2, %0          \n"
        "b    _fault_handler_c\n"
        :
        : "i" (FAULT_TYPE_BUS)
    );
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile(
        "tst  lr, #4          \n"
        "ite  eq              \n"
        "mrseq r0, msp        \n"
        "mrsne r0, psp        \n"
        "mov  r1, lr          \n"
        "mov  r2, %0          \n"
        "b    _fault_handler_c\n"
        :
        : "i" (FAULT_TYPE_USAGE)
    );
}

/* ── Debug Monitor ───────────────────────────────────────────────────────── */

void DebugMon_Handler(void)
{
    for (;;)
        ;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
