/**
 * @file        system.c
 * @brief       system.c — CMSIS SystemInit entry point
 * @ingroup     kernel_glue
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Kernel Glue
 * @info        C-runtime / newlib shim: malloc/free wrappers, syscalls, FreeRTOS kernel adapters.
 * @dependency  newlib-nano, FreeRTOS-Kernel
 *
 * @details
 * system.c — CMSIS SystemInit entry point
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

#include <board/board_config.h>
#include <def_attributes.h>
#include <device.h>

/* H7 uses SystemInit() from system_stm32h7xx.c (handles D1/D2/D3 domains + FPU). */
#if !defined(STM32H7)
__SECTION_BOOT
void SystemInit(void)
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
#endif /* !STM32H7 */
