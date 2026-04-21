#include <stdint.h>
#include "def_attributes.h"

/* SystemInit is implemented in drivers/hal/stm32/hal_rcc_stm32.c */
void SystemInit(void);


/* Symbols from linker script */
extern uint32_t _estack;  /* Stack pointer end */

extern uint32_t _sdata_boot;            /* start of boot .data in RAM */
extern uint32_t _edata_boot;            /* end of boot .data in RAM */
extern uint32_t __boot_data_lma;        /* start of boot .data in FLASH */
extern uint32_t __boot_data_lma_end;    /* end of boot .data in FLASH */

extern uint32_t _sdata_os;              /* start of os .data in RAM */
extern uint32_t _edata_os;              /* end of os .data in RAM */
extern uint32_t __os_data_lma;          /* start of os .data in FLASH */
extern uint32_t __os_data_lma_end;      /* end of os .data in FLASH */

extern uint32_t _sdata_app;             /* start of app .data in RAM */
extern uint32_t _edata_app;             /* end of app .data in RAM */
extern uint32_t __app_data_lma;         /* start app os .data in FLASH */
extern uint32_t __app_data_lma_end;     /* end of app .data in FLASH */


extern uint32_t _sidata;  /* start of init values for .data (LMA in flash) */
extern uint32_t _sdata;   /* start of .data in RAM */
extern uint32_t _edata;   /* end of .data in RAM */
extern uint32_t _sbss;    /* start of .bss in RAM */
extern uint32_t _ebss;    /* end of .bss in RAM */

extern void main(void);

__SECTION_BOOT void __libc_init_array(void);

/* Default handler */
__SECTION_BOOT __attribute__((used)) void Default_Handler(void) 
{
    while (1) 
    {

    }
}

/* Cortex-M exception handlers (weak aliases to Default_Handler) */
void Reset_Handler(void);
void NMI_Handler(void)              __attribute__((weak, alias("Default_Handler"),used));
void HardFault_Handler(void)        __attribute__((weak, alias("Default_Handler"),used));
void MemManage_Handler(void)        __attribute__((weak, alias("Default_Handler"),used));
void BusFault_Handler(void)         __attribute__((weak, alias("Default_Handler"),used));
void UsageFault_Handler(void)       __attribute__((weak, alias("Default_Handler"),used));
void SVC_Handler(void)              __attribute__((weak, alias("Default_Handler"),used));
void DebugMon_Handler(void)         __attribute__((weak, alias("Default_Handler"),used));
void PendSV_Handler(void)           __attribute__((weak, alias("Default_Handler"),used));
void SysTick_Handler(void)          __attribute__((weak, alias("Default_Handler"),used));

/* Peripheral IRQ handler weak forward declarations (AUTO-GENERATED).
 * Default_Handler must be visible in this translation unit — include only here. */
#include <board/irq_periph_handlers_generated.h>

/* Vector table (goes into .isr_vector section at FLASH_BOOT origin) */
__attribute__((section(".isr_vector")))
const void* const g_pfnVectors[] =
{
    /* ── System / Cortex-M4 core handlers ───────────────────────────────────── */
    &_estack,
    Reset_Handler,          /* Reset                  */
    NMI_Handler,            /* NMI                    */
    HardFault_Handler,      /* Hard Fault             */
    MemManage_Handler,      /* MPU Fault              */
    BusFault_Handler,       /* Bus Fault              */
    UsageFault_Handler,     /* Usage Fault            */
    0,                      /* Reserved               */
    0,                      /* Reserved               */
    0,                      /* Reserved               */
    0,                      /* Reserved               */
    SVC_Handler,            /* SVCall                 */
    DebugMon_Handler,       /* Debug Monitor          */
    0,                      /* Reserved               */
    PendSV_Handler,         /* PendSV                 */
    SysTick_Handler,        /* SysTick                */

    /* ── Peripheral IRQ vectors (AUTO-GENERATED from irq_table.xml) ──────────── */
#include <board/irq_periph_vectors_generated.inc>
};

/* Reset Handler implementation */
__SECTION_BOOT __USED __attribute__((noreturn)) void Reset_Handler(void)  
{
    uint32_t *src, *dst;

    /* Stack pointer init  */
__asm volatile (
    "ldr r0, =g_pfnVectors\n\t"
    "ldr sp, [r0]"
    );

    /* System init (clocks, etc.) */
    SystemInit();

    // SystemCoreClockUpdate();

    /* copy boot sec data LMA to VMA */
    for (dst = &_sdata_boot, src = &__boot_data_lma; dst < &_edata_boot; )
    {
        /* VMA = LMA */
        *dst++ = *src++;
    }
    
    /* copy OS sec data LMA to VMA */
    for (dst = &_sdata_os, src = &__os_data_lma; dst < &_edata_os; )
    {
        /* VMA = LMA */
        *dst++ = *src++;
    }

    /* copy app sec data LMA to VMA */
    for (dst = &_sdata_app, src = &__app_data_lma; dst < &_edata_app; )
    {
        /* VMA = LMA */
        *dst++ = *src++;
    }

    /* Copy .data section from flash (FLASH_OS or FLASH_APP depending on build) to RAM */
    for (dst = &_sdata, src = &_sidata; dst < &_edata; )
    {
        /* VMA = LMA */
        *dst++ = *src++;
    }


    /* Zero initialize .bss section */
    for (dst = &_sbss; dst < &_ebss; )
    {
        /* VMA = 0 */
        *dst++ = 0;
    }


    /* Run static constructors (C++ support) */
    __libc_init_array();

    /* Call main */
    main();

    /* If main ever returns, trap CPU */
    while (1) 
    {

    }
}
