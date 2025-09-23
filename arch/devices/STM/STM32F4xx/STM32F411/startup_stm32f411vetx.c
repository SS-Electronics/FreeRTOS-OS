#include <stdint.h>
#include "../system_stm32f4xx.h"
#include "def_attributes.h"


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

extern int main(void);

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

/* Peripheral IRQ handlers (add more as needed) */
void WWDG_IRQHandler(void)          __attribute__((weak, alias("Default_Handler"),used));
void PVD_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"),used));
void TAMP_STAMP_IRQHandler(void)    __attribute__((weak, alias("Default_Handler"),used));
void RTC_WKUP_IRQHandler(void)      __attribute__((weak, alias("Default_Handler"),used));
void FLASH_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void RCC_IRQHandler(void)           __attribute__((weak, alias("Default_Handler"),used));
void EXTI0_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void EXTI1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void EXTI2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void EXTI3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void EXTI4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));

void DMA1_Stream0_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream6_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void ADC_IRQHandler(void)                   __attribute__((weak, alias("Default_Handler"),used));
void EXTI9_5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM1_BRK_TIM9_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM1_UP_TIM10_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM1_TRG_COM_TIM11_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM1_CC_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C1_EV_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C1_ER_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C2_EV_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C2_ER_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SPI1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SPI2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void USART1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void USART2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void EXTI15_10_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void RTC_Alarm_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void OTG_FS_WKUP_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA1_Stream7_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SDIO_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void TIM5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SPI3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream0_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void OTG_FS_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream6_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void DMA2_Stream7_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void USART6_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C3_EV_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void I2C3_ER_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void FPU_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SPI4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));
void SPI5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler"),used));



// (void (*)(void))&_estack,       /* Initial stack pointer */


/* Vector table (goes into .isr_vector section at FLASH_BOOT origin) */
__attribute__((section(".isr_vector")))
const void* const g_pfnVectors[]  = 
{
    &_estack,
    Reset_Handler,          /* Reset Handler */
    NMI_Handler,            /* NMI Handler */
    HardFault_Handler,      /* Hard Fault Handler */
    MemManage_Handler,      /* MPU Fault Handler */
    BusFault_Handler,       /* Bus Fault Handler */
    UsageFault_Handler,     /* Usage Fault Handler */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    SVC_Handler,            /* SVCall Handler */
    DebugMon_Handler,       /* Debug Monitor Handler */
    0,                      /* Reserved */
    PendSV_Handler,         /* PendSV Handler */
    SysTick_Handler,        /* SysTick Handler */
    /* External Interrupts (fill as needed) */
    WWDG_IRQHandler,        /* Window Watchdog */
    PVD_IRQHandler,         /* PVD */
    TAMP_STAMP_IRQHandler,  /* Tamper/TimeStamp */
    RTC_WKUP_IRQHandler,    /* RTC Wakeup */
    FLASH_IRQHandler,       /* Flash */
    RCC_IRQHandler,         /* RCC */
    EXTI0_IRQHandler,       /* EXTI0 */
    EXTI1_IRQHandler,       /* EXTI1 */
    EXTI2_IRQHandler,       /* EXTI2 */
    EXTI3_IRQHandler,       /* EXTI3 */
    EXTI4_IRQHandler,       /* EXTI4 */
    DMA1_Stream0_IRQHandler,           /* DMA1 Stream 0                */                  
    DMA1_Stream1_IRQHandler,           /* DMA1 Stream 1                */                   
    DMA1_Stream2_IRQHandler,           /* DMA1 Stream 2                */                   
    DMA1_Stream3_IRQHandler,           /* DMA1 Stream 3                */                   
    DMA1_Stream4_IRQHandler,           /* DMA1 Stream 4                */                   
    DMA1_Stream5_IRQHandler,           /* DMA1 Stream 5                */                   
    DMA1_Stream6_IRQHandler,           /* DMA1 Stream 6                */                   
    ADC_IRQHandler,                    /* ADC1, ADC2 and ADC3s         */
    0,               				  /* Reserved                      */                         
    0,             					  /* Reserved                     */                          
    0,                                 /* Reserved                     */                          
    0,                                 /* Reserved                     */                          
    EXTI9_5_IRQHandler,                /* External Line[9:5]s          */                          
    TIM1_BRK_TIM9_IRQHandler,          /* TIM1 Break and TIM9          */         
    TIM1_UP_TIM10_IRQHandler,         /* TIM1 Update and TIM10        */         
    TIM1_TRG_COM_TIM11_IRQHandler,     /* TIM1 Trigger and Commutation and TIM11 */
    TIM1_CC_IRQHandler,                /* TIM1 Capture Compare         */                          
    TIM2_IRQHandler,                   /* TIM2                         */                   
    TIM3_IRQHandler,                   /* TIM3                         */                   
    TIM4_IRQHandler,                   /* TIM4                         */                   
    I2C1_EV_IRQHandler,                /* I2C1 Event                   */                          
    I2C1_ER_IRQHandler,                /* I2C1 Error                   */                          
    I2C2_EV_IRQHandler,                /* I2C2 Event                   */                          
    I2C2_ER_IRQHandler,                /* I2C2 Error                   */                            
    SPI1_IRQHandler ,                  /* SPI1                         */                   
    SPI2_IRQHandler,                   /* SPI2                         */                   
    USART1_IRQHandler ,                /* USART1                       */                   
    USART2_IRQHandler ,                /* USART2                       */                   
    0,               			  /* Reserved                       */                   
    EXTI15_10_IRQHandler,              /* External Line[15:10]s        */                          
    RTC_Alarm_IRQHandler,              /* RTC Alarm (A and B) through EXTI Line */                 
    OTG_FS_WKUP_IRQHandler,            /* USB OTG FS Wakeup through EXTI line */                       
    0,                                 /* Reserved     				  */         
    0,                                 /* Reserved       			  */         
    0,                                 /* Reserved 					  */
    0,                                 /* Reserved                     */                          
    DMA1_Stream7_IRQHandler,           /* DMA1 Stream7                 */                          
    0,                                 /* Reserved                     */                   
    SDIO_IRQHandler,                   /* SDIO                         */                   
    TIM5_IRQHandler,                   /* TIM5                         */                   
    SPI3_IRQHandler,                   /* SPI3                         */                   
    0,                                 /* Reserved                     */                   
    0,                                 /* Reserved                     */                   
    0,                                 /* Reserved                     */                   
    0,                                 /* Reserved                     */
    DMA2_Stream0_IRQHandler,           /* DMA2 Stream 0                */                   
    DMA2_Stream1_IRQHandler,           /* DMA2 Stream 1                */                   
    DMA2_Stream2_IRQHandler,           /* DMA2 Stream 2                */                   
    DMA2_Stream3_IRQHandler,           /* DMA2 Stream 3                */                   
    DMA2_Stream4_IRQHandler,           /* DMA2 Stream 4                */                   
    0,                    			  /* Reserved                     */                   
    0,              					  /* Reserved                     */                     
    0,              					  /* Reserved                     */                          
    0,             					  /* Reserved                     */                          
    0,              					  /* Reserved                     */                          
    0 ,             					  /* Reserved                     */                          
    OTG_FS_IRQHandler  ,               /* USB OTG FS                   */                   
    DMA2_Stream5_IRQHandler,           /* DMA2 Stream 5                */                   
    DMA2_Stream6_IRQHandler ,          /* DMA2 Stream 6                */                   
    DMA2_Stream7_IRQHandler ,          /* DMA2 Stream 7                */                   
    USART6_IRQHandler  ,               /* USART6                       */                    
    I2C3_EV_IRQHandler,                /* I2C3 event                   */                          
    I2C3_ER_IRQHandler ,               /* I2C3 error                   */                          
    0  ,                               /* Reserved                     */                   
    0 ,                                /* Reserved                     */                   
    0 ,                                /* Reserved                     */                         
    0  ,                               /* Reserved                     */                   
    0 ,                                /* Reserved                     */                   
    0  ,                               /* Reserved                     */                   
    0  ,                               /* Reserved                     */
    FPU_IRQHandler ,                   /* FPU                          */
    0   ,                              /* Reserved                     */                   
    0  ,                               /* Reserved                     */
    SPI4_IRQHandler ,                 /* SPI4                         */
    SPI5_IRQHandler  ,                 /* SPI5                         */  
};

/* Reset Handler implementation */
__SECTION_BOOT __attribute__((noreturn)) void Reset_Handler(void)  
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
