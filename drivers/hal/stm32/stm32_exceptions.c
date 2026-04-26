/**
 * @file    stm32_exceptions.c
 * @author  Subhajit Roy
 * @brief   Cortex-M4 core exception and fault handling for STM32 MCUs
 *
 * @details
 * This module provides strong implementations of ARM Cortex-M4 exception
 * handlers, overriding the weak default handlers from the startup file
 * (e.g., startup_stm32f411vetx.c).
 *
 * It implements a minimal, deterministic, and RTOS-safe fault handling
 * mechanism suitable for both bare-metal and FreeRTOS-based systems.
 *
 * The design focuses on:
 *   - Reliable crash diagnostics
 *   - Zero dependency on RTOS or interrupts during fault
 *   - Direct hardware access for debug output
 *
 * ---------------------------------------------------------------------------
 * @section fault_flow Fault Handling Flow
 * ---------------------------------------------------------------------------
 *
 * When a fault occurs (HardFault / MemManage / BusFault / UsageFault):
 *
 * @code
 *   CPU triggers exception
 *        │
 *        ▼
 *   Naked handler (assembly trampoline)
 *        │
 *        ├─ Determine active stack pointer using EXC_RETURN (LR)
 *        │     bit[2] == 0 → MSP (Handler mode / MSP context)
 *        │     bit[2] == 1 → PSP (Thread mode / FreeRTOS task)
 *        │
 *        ▼
 *   _fault_handler_c(frame, exc_return, fault_type)
 *        │
 *        ├─ Capture stacked registers (r0–r3, r12, lr, pc, xPSR)
 *        ├─ Capture SCB fault registers (HFSR, CFSR, BFAR, MMFAR)
 *        ├─ Store diagnostics into @ref g_fault_info
 *        ├─ Decode fault cause (MMFSR / BFSR / UFSR / HFSR)
 *        ├─ Emit debug output via USART2 (polling mode)
 *        ▼
 *   Infinite loop (system halt)
 * @endcode
 *
 * ---------------------------------------------------------------------------
 * @section design Design Characteristics
 * ---------------------------------------------------------------------------
 *
 * - Fully independent of RTOS in fault context
 * - No dynamic memory usage
 * - No interrupt reliance
 * - Safe polling-based UART output
 * - Deterministic execution path
 * - Suitable for post-mortem debugging
 *
 * ---------------------------------------------------------------------------
 * @section captured_data Captured Fault Data
 * ---------------------------------------------------------------------------
 *
 * The following information is captured:
 *
 * - CPU context (auto-stacked registers):
 *     - r0, r1, r2, r3, r12, lr, pc, xPSR
 *
 * - System Control Block (SCB) registers:
 *     - HFSR  (HardFault Status Register)
 *     - CFSR  (Configurable Fault Status Register)
 *     - BFAR  (Bus Fault Address Register)
 *     - MMFAR (Memory Management Fault Address Register)
 *
 * - EXC_RETURN value (stack origin identification)
 * - Fault type (Hard / Mem / Bus / Usage)
 *
 * All captured data is stored in:
 *
 * @ref g_fault_info
 *
 * This structure can be inspected via debugger or memory dump.
 *
 * ---------------------------------------------------------------------------
 * @section fault_types Supported Fault Types
 * ---------------------------------------------------------------------------
 *
 * - HardFault
 * - MemManage Fault
 * - BusFault
 * - UsageFault
 * - NMI (minimal handling)
 *
 * ---------------------------------------------------------------------------
 * @section uart_debug UART Debug Output
 * ---------------------------------------------------------------------------
 *
 * Debug output is transmitted via USART2 using direct register access:
 *
 * - No HAL / LL drivers
 * - No buffering
 * - No interrupts
 *
 * This ensures reliability even in corrupted system states.
 *
 * @note
 * USART2 must be initialized before any fault occurs.
 *
 * ---------------------------------------------------------------------------
 * @section usage Usage Guidelines
 * ---------------------------------------------------------------------------
 *
 * - Include this file in the build to override weak handlers
 * - Ensure early initialization of USART2
 * - On fault:
 *     1. System halts in infinite loop
 *     2. Attach debugger
 *     3. Inspect:
 *          - g_fault_info.frame.pc
 *          - g_fault_info.cfsr
 *          - g_fault_info.hfsr
 *
 * ---------------------------------------------------------------------------
 * @section limitations Limitations
 * ---------------------------------------------------------------------------
 *
 * - No stack unwinding
 * - No symbol resolution
 * - No persistent logging (e.g., flash)
 * - No recovery mechanism (system halt only)
 *
 * ---------------------------------------------------------------------------
 * @section build Build Conditions
 * ---------------------------------------------------------------------------
 *
 * This module is compiled only when:
 *
 * @code
 * CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 * @endcode
 *
 * ---------------------------------------------------------------------------
 * @note
 * This file is part of the FreeRTOS-OS Project.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

/* ────────────────────────────────────────────────────────────────────────── */
/*                           Fault Type Identifiers                           */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Fault type identifiers used internally.
 */
#define FAULT_TYPE_HARD     0U  /**< HardFault */
#define FAULT_TYPE_MEM      1U  /**< MemManage Fault */
#define FAULT_TYPE_BUS      2U  /**< BusFault */
#define FAULT_TYPE_USAGE    3U  /**< UsageFault */

/* ────────────────────────────────────────────────────────────────────────── */
/*                          Hardware Exception Frame                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief CPU-stacked exception frame (auto-pushed by Cortex-M4).
 */
typedef struct
{
    uint32_t r0;     /**< General-purpose register R0 */
    uint32_t r1;     /**< General-purpose register R1 */
    uint32_t r2;     /**< General-purpose register R2 */
    uint32_t r3;     /**< General-purpose register R3 */
    uint32_t r12;    /**< General-purpose register R12 */
    uint32_t lr;     /**< Link register at time of fault */
    uint32_t pc;     /**< Program counter at fault */
    uint32_t xpsr;   /**< Program status register */
} fault_frame_t;

/* ────────────────────────────────────────────────────────────────────────── */
/*                          Fault Diagnostic Record                           */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Persistent fault diagnostic structure.
 *
 * @details
 * Captures CPU state and SCB fault registers during exception.
 * Intended for debugger inspection or post-mortem analysis.
 */
typedef struct
{
    fault_frame_t frame;   /**< CPU register snapshot */
    uint32_t hfsr;         /**< HardFault Status Register */
    uint32_t cfsr;         /**< Configurable Fault Status Register */
    uint32_t bfar;         /**< Bus Fault Address Register */
    uint32_t mmfar;        /**< MemManage Fault Address Register */
    uint32_t exc_return;   /**< EXC_RETURN value (stack origin info) */
    uint32_t fault_type;   /**< Fault type identifier */
} fault_info_t;

/**
 * @brief Global fault information (retained after crash).
 */
__SECTION_OS_DATA __USED
volatile fault_info_t g_fault_info;

/* ────────────────────────────────────────────────────────────────────────── */
/*                             USART2 Debug Output                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief USART2 base address and registers (direct access).
 */
#define USART2_BASE  0x40004400UL
#define USART2_SR    (*(volatile uint32_t *)(USART2_BASE + 0x00U))
#define USART2_DR    (*(volatile uint32_t *)(USART2_BASE + 0x04U))
#define USART2_TXE   (1UL << 7) /**< Transmit data register empty */
#define USART2_TC    (1UL << 6) /**< Transmission complete */

/**
 * @brief Send a single character over USART2.
 */
__SECTION_OS __USED
static void _fault_putchar(char c)
{
    while (!(USART2_SR & USART2_TXE));
    USART2_DR = (uint32_t)(uint8_t)c;
}

/**
 * @brief Send a string over USART2.
 */
__SECTION_OS __USED
static void _fault_puts(const char *s)
{
    while (*s) _fault_putchar(*s++);
}

/**
 * @brief Print 32-bit value in hexadecimal.
 */
__SECTION_OS __USED
static void _fault_print_hex32(uint32_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    _fault_putchar('0');
    _fault_putchar('x');
    for (int i = 28; i >= 0; i -= 4)
        _fault_putchar(hex[(v >> i) & 0xFU]);
}

/**
 * @brief Print 32-bit value in decimal.
 */
__SECTION_OS __USED
static void _fault_print_dec(uint32_t v)
{
    if (v == 0) { _fault_putchar('0'); return; }
    char buf[10];
    int n = 0;
    while (v) { buf[n++] = '0' + (v % 10U); v /= 10U; }
    while (n--) _fault_putchar(buf[n]);
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                           Fault Register Decoders                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Decode MemManage fault bits.
 */
__SECTION_OS __USED
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

/**
 * @brief Decode BusFault bits.
 */
__SECTION_OS __USED
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

/**
 * @brief Decode UsageFault bits.
 */
__SECTION_OS __USED
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

/**
 * @brief Decode HardFault status.
 */
__SECTION_OS __USED
static void _decode_hfsr(uint32_t hfsr)
{
    _fault_puts("  HFSR:");
    if (hfsr & (1U << 31)) _fault_puts(" DEBUGEVT");
    if (hfsr & (1U << 30)) _fault_puts(" FORCED");
    if (hfsr & (1U << 1)) _fault_puts(" VECTBL");
    _fault_puts("\r\n");
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                           Central Fault Handler                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Central fault handler (C implementation).
 */
__SECTION_OS
void __attribute__((used, noinline))
_fault_handler_c(uint32_t *frame, uint32_t exc_return, uint32_t fault_type)
{
    uint32_t hfsr  = *(volatile uint32_t *)0xE000ED2C;
    uint32_t cfsr  = *(volatile uint32_t *)0xE000ED28;
    uint32_t mmfar = *(volatile uint32_t *)0xE000ED34;
    uint32_t bfar  = *(volatile uint32_t *)0xE000ED38;

    g_fault_info.frame.r0 = frame[0];
    g_fault_info.frame.r1 = frame[1];
    g_fault_info.frame.r2 = frame[2];
    g_fault_info.frame.r3 = frame[3];
    g_fault_info.frame.r12 = frame[4];
    g_fault_info.frame.lr = frame[5];
    g_fault_info.frame.pc = frame[6];
    g_fault_info.frame.xpsr = frame[7];

    g_fault_info.hfsr = hfsr;
    g_fault_info.cfsr = cfsr;
    g_fault_info.bfar = bfar;
    g_fault_info.mmfar = mmfar;
    g_fault_info.exc_return = exc_return;
    g_fault_info.fault_type = fault_type;

    _fault_puts("\r\n*** FAULT ***\r\n");

    _decode_hfsr(hfsr);
    _decode_mmfsr(cfsr);
    _decode_bfsr(cfsr);
    _decode_ufsr(cfsr);

    while (!(USART2_SR & USART2_TC));

    for (;;);
}

/* ────────────────────────────────────────────────────────────────────────── */
/*                             Exception Handlers                             */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief HardFault handler (assembly trampoline).
 */
__SECTION_OS __USED
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "mov r2, %0\n"
        "b _fault_handler_c\n"
        : : "i"(FAULT_TYPE_HARD));
}

/**
 * @brief MemManage handler.
 */
__SECTION_OS __USED
__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "mov r2, %0\n"
        "b _fault_handler_c\n"
        : : "i"(FAULT_TYPE_MEM));
}

/**
 * @brief BusFault handler.
 */
__SECTION_OS __USED
__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "mov r2, %0\n"
        "b _fault_handler_c\n"
        : : "i"(FAULT_TYPE_BUS));
}

/**
 * @brief UsageFault handler.
 */
__SECTION_OS __USED
__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "mov r2, %0\n"
        "b _fault_handler_c\n"
        : : "i"(FAULT_TYPE_USAGE));
}

/**
 * @brief NMI handler.
 */
__SECTION_OS __USED
void NMI_Handler(void)
{
    for (;;);
}

/**
 * @brief Debug monitor handler.
 */
__SECTION_OS __USED
void DebugMon_Handler(void)
{
    for (;;);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */