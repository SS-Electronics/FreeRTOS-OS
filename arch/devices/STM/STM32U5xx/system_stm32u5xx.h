/**
 ******************************************************************************
 * @file    system_stm32u5xx.h
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   CMSIS system header — STM32U5xx (Cortex-M33 + TrustZone).
 *
 *          Declares the externally visible CMSIS system entry points:
 *              SystemInit()            — called from Reset_Handler
 *              SystemCoreClockUpdate() — refreshes SystemCoreClock
 *              SystemCoreClock         — current CPU clock in Hz
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __SYSTEM_STM32U5XX_H
#define __SYSTEM_STM32U5XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t SystemCoreClock;

void SystemInit(void);
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_STM32U5XX_H */
