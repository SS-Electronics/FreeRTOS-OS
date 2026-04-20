/*
 * device.h — vendor-level device umbrella header
 *
 * Author:      Subhajit Roy
 *              subhajitroy005@gmail.com
 *
 * Module:      arch/devices
 * Info:        Single include that selects the correct CMSIS, HAL, and
 *              board-handle headers based on the MCU vendor/family.
 *              Vendor detection is purely preprocessor-based — the build
 *              system injects one chip-specific symbol via TARGET_SYMBOL_DEF
 *              (e.g. -DSTm32F411xE) which is caught by the vendor block below.
 *
 * Include via:  #include <device.h>
 * Resolved by:  -I$(CURDIR)/arch/devices  (arch/Makefile)
 *
 * Adding a new vendor:
 *   1. Add the chip's preprocessor symbols to the detection block.
 *   2. Add an #elif block with the vendor's CMSIS / SDK includes.
 *   3. Add the vendor's include paths to arch/Makefile.
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 */

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <def_attributes.h>

/* ══════════════════════════════════════════════════════════════════════════
 * Vendor / family detection
 *
 * The chip symbol is injected at build time:
 *   ROOT Makefile:  TARGET_SYMBOL_DEF += -D$(CONFIG_TARGET_MCU)
 *   e.g.  -DSTm32F411xE, -DSTm32F407xx, -DXMC4800
 *
 * Family umbrella symbols (STM32F4, STM32H7 …) are defined inside the
 * vendor's own CMSIS device header once it is included, so we detect
 * the individual chip variant first.
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── STM32 ────────────────────────────────────────────────────────────────
 * Covers: STM32F0/F1/F2/F3/F4/F7/G0/G4/H7/L0/L1/L4/L5/U5/WB
 *         Individual chip variants also listed for direct -D defines.     */
#if defined(STM32F0)  || defined(STM32F1)  || defined(STM32F2)  || \
    defined(STM32F3)  || defined(STM32F4)  || defined(STM32F7)  || \
    defined(STM32G0)  || defined(STM32G4)  || defined(STM32H7)  || \
    defined(STM32L0)  || defined(STM32L1)  || defined(STM32L4)  || \
    defined(STM32L5)  || defined(STM32U5)  || defined(STM32WB)  || \
    /* STM32F4xx individual variants */                               \
    defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F411xE) || \
    defined(STM32F411xC) || defined(STM32F401xC) || defined(STM32F401xE) || \
    defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F427xx) || \
    defined(STM32F429xx) || defined(STM32F437xx) || defined(STM32F439xx) || \
    defined(STM32F446xx) || defined(STM32F469xx) || defined(STM32F479xx)

#define DEVICE_VENDOR_STM32     1

/* CMSIS device header — defines core registers, peripheral base addresses,
 * interrupt numbers; selects the correct sub-header via the chip define.  */
#include <stm32f4xx.h>

/* STM32F4xx sub-family pin/peripheral capability header */
#if   defined(STM32F411xE) || defined(STM32F411xC)
#include <STM32F411/stm32_f411xe.h>
#elif defined(STM32F401xC) || defined(STM32F401xE)
/* #include <STM32F401/stm32_f401xe.h> */   /* add when porting F401 */
#endif

/* STM32 HAL — stm32f4xx_hal.h pulls in all modules enabled in hal_conf.h
 * (generated into arch/devices/device_conf/ by make config-outputs).     */
#include <stm32f4xx_hal_conf.h>
#include <stm32f4xx_hal.h>

/* CMSIS system symbols — defined in drivers/hal/stm32/hal_rcc_stm32.c
 * (replaces the old system_stm32f4xx.c).                                 */
extern uint32_t      SystemCoreClock;    /*!< System Clock Frequency (Core Clock)  */
extern const uint8_t AHBPrescTable[16];  /*!< AHB prescaler shift LUT              */
extern const uint8_t APBPrescTable[8];   /*!< APB prescaler shift LUT              */
void SystemInit(void);
void SystemCoreClockUpdate(void);


/* ── Infineon XMC ─────────────────────────────────────────────────────────
 * Covers: XMC1xxx (Cortex-M0), XMC4xxx (Cortex-M4)                       */
#elif defined(XMC4800) || defined(XMC4700) || defined(XMC4500) || \
      defined(XMC4400) || defined(XMC4200) || defined(XMC4100) || \
      defined(XMC1100) || defined(XMC1200) || defined(XMC1300) || \
      defined(XMC1400)

#define DEVICE_VENDOR_INFINEON  1

/* TODO: include Infineon XMC CMSIS + XMCLib headers                       */
/* #include <XMC4800.h>       */
/* #include <xmc_device.h>    */


/* ── Microchip SAM / PIC32 ───────────────────────────────────────────────
 * Covers: SAM D/E/L/C series (Cortex-M0+/M4), PIC32MX/MZ                 */
#elif defined(__SAMD21__) || defined(__SAMD51__) || defined(__SAME51__) || \
      defined(__SAM4S__)  || defined(__SAM4E__)  || defined(__SAM3X__)  || \
      defined(__PIC32MX__) || defined(__PIC32MZ__)

#define DEVICE_VENDOR_MICROCHIP 1

/* TODO: include Microchip ASF4 / Harmony / CMSIS headers                  */
/* #include <samd21.h>        */
/* #include <system_samd21.h> */


#else
#error "device.h: unrecognised MCU — add the chip define to the vendor block"
#endif /* vendor selection */


/* ── Board-level peripheral handles ──────────────────────────────────────
 * Auto-generated from app/board/<board>.xml by gen_board_config.py.
 * Common to all vendors.                                                   */
#include <board/board_handles.h>


#endif /* __DEVICE_H__ */
