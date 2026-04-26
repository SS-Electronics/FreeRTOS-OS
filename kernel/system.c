/*
 * system.c — CMSIS SystemInit entry point
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * SystemInit() is called by Reset_Handler before .data/.bss initialisation.
 * It must not access any RAM outside of .boot_data.
 *
 * Responsibilities:
 *   - Enable the FPU coprocessors (CP10/CP11) on Cortex-M4F targets.
 *   - Optionally relocate the vector table (USER_VECT_TAB_ADDRESS / Kconfig).
 *
 * SystemCoreClockUpdate() and the CMSIS prescaler tables remain in
 * hal_rcc_stm32.c because they read live RCC registers.
 */

#include <board/board_config.h>
#include <def_attributes.h>
#include <device.h>

__SECTION_BOOT void SystemInit(void)
{
    /* Enable FPU coprocessors CP10 and CP11 (full access) */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10U * 2U) | (3UL << 11U * 2U));
#endif

    /* Relocate vector table when explicitly requested via linker/Kconfig */
#if defined(USER_VECT_TAB_ADDRESS)
#  if defined(VECT_TAB_SRAM)
    SCB->VTOR = SRAM_BASE | 0x00000000U;
#  else
    SCB->VTOR = FLASH_BASE | 0x00000000U;
#  endif
#endif
}
