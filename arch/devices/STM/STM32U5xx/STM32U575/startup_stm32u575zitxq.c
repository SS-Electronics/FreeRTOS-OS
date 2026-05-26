/**
 ******************************************************************************
 * @file    startup_stm32u575zitxq.c
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   Reset_Handler + vector table for STM32U575ZITxQ (Nucleo-U575ZI-Q).
 *
 *          Cortex-M33 with TrustZone, single-image (non-secure-only) layout.
 *          Mirrors the H723 startup file structure but adapted for M33:
 *            - SCB_VTOR write done in SystemInit() (not here, by convention)
 *            - No I/D cache (M33 has no integrated cache; the U5 ICACHE
 *              peripheral is brought up later by HAL_RCC).
 *            - Trustcore hook (trustcore_init_secure_world) called from
 *              Reset_Handler so the secure-side setup runs BEFORE main().
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include <stdint.h>
#include "def_attributes.h"

/* CMSIS device header — pulls in core_cm33.h via stm32u5xx.h. */
#include <stm32u5xx.h>

void SystemInit(void);

/* Weak trustcore hook. The example app supplies a strong override that
 * configures SAU / GTZC. When the symbol is not linked in, this no-op
 * default is used. The hook is called before .data copy / .bss clear so
 * the secure-side runtime decisions happen before C runtime is live. */
__attribute__((weak)) void trustcore_init_secure_world(void) { }

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern void main(void);

__SECTION_BOOT void __libc_init_array(void);

__SECTION_BOOT __attribute__((used)) void Default_Handler(void)
{
    while (1) { }
}

/* Cortex-M33 exception handlers (weak aliases to Default_Handler).
 * SecureFault is M33-specific — added on top of the M7 set. */
void Reset_Handler(void);
void NMI_Handler(void)              __attribute__((weak, alias("Default_Handler"), used));
void HardFault_Handler(void)        __attribute__((weak, alias("Default_Handler"), used));
void MemManage_Handler(void)        __attribute__((weak, alias("Default_Handler"), used));
void BusFault_Handler(void)         __attribute__((weak, alias("Default_Handler"), used));
void UsageFault_Handler(void)       __attribute__((weak, alias("Default_Handler"), used));
void SecureFault_Handler(void)      __attribute__((weak, alias("Default_Handler"), used));
void SVC_Handler(void)              __attribute__((weak, alias("Default_Handler"), used));
void DebugMon_Handler(void)         __attribute__((weak, alias("Default_Handler"), used));
void PendSV_Handler(void)           __attribute__((weak, alias("Default_Handler"), used));
void SysTick_Handler(void)          __attribute__((weak, alias("Default_Handler"), used));

/* Peripheral IRQ handler weak forward declarations (AUTO-GENERATED). */
#include <board/irq_periph_handlers_generated.h>

__attribute__((section(".isr_vector"), used))
const void* const g_pfnVectors[] =
{
    /* ── System / Cortex-M33 core handlers ─────────────────────────────── */
    &_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    SecureFault_Handler,     /* M33-only — slot 7, was Reserved on M4/M7    */
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    /* ── Peripheral IRQ vectors (AUTO-GENERATED from irq_table.xml) ────── */
#include <board/irq_periph_vectors_generated.inc>
};

__SECTION_BOOT __attribute__((noreturn)) void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* Stack pointer init (MSP = first entry of the vector table). */
    __asm volatile (
        "ldr r0, =g_pfnVectors\n\t"
        "ldr sp, [r0]"
    );

    /* TrustZone setup hook. Runs before C-runtime init so the secure side
     * can program SAU / GTZC / IDAU before .data / .bss touch SRAM regions
     * that may be configured non-secure. When the application doesn't link
     * a strong override this collapses to a no-op. */
    trustcore_init_secure_world();

    /* CMSIS system init (VTOR, FPU access). */
    SystemInit();

    /* Copy .data section from Flash to SRAM1. */
    for (dst = &_sdata, src = &_sidata; dst < &_edata; )
    {
        *dst++ = *src++;
    }

    /* Zero initialise .bss section. */
    for (dst = &_sbss; dst < &_ebss; )
    {
        *dst++ = 0;
    }

    /* Run static constructors (C++ support). */
    __libc_init_array();

    main();

    while (1) { }
}
