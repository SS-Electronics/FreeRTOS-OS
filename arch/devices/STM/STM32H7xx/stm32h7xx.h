/**
 ******************************************************************************
 * @file    stm32h7xx.h
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   CMSIS STM32H7xx Device Peripheral Access Layer Header File.
 *
 *          Umbrella header — selects the correct device sub-header based on
 *          the chip define injected by the build system (-DSTM32H723xx etc.).
 *
 *          Pattern mirrors STM32CubeH7 Drivers/CMSIS/Device/ST/STM32H7xx/
 *          so the stm32h7xx-hal-driver can find this file without modification.
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __STM32H7xx_H
#define __STM32H7xx_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined  (STM32H7)
#define STM32H7
#endif

/* ── Device variant selection ─────────────────────────────────────────────── */
#if defined(STM32H723xx)
#  include "stm32h723xx.h"
#elif defined(STM32H733xx)
#  error "STM32H733xx not yet ported — add stm32_h733xx.h"
#elif defined(STM32H725xx)
#  error "STM32H725xx not yet ported — add stm32_h725xx.h"
#elif defined(STM32H735xx)
#  error "STM32H735xx not yet ported — add stm32_h735xx.h"
#elif defined(STM32H743xx)
#  error "STM32H743xx not yet ported — add stm32_h743xx.h"
#elif defined(STM32H750xx)
#  error "STM32H750xx not yet ported — add stm32_h750xx.h"
#else
#  error "stm32h7xx.h: no STM32H7xx variant defined. Pass -DSTM32H723xx (or similar)."
#endif

/* ── Common STM32 utility macros ──────────────────────────────────────────── */
typedef enum { RESET = 0U, SET = !RESET }         FlagStatus, ITStatus;
typedef enum { DISABLE = 0U, ENABLE = !DISABLE }  FunctionalState;
typedef enum { SUCCESS = 0U, ERROR = !SUCCESS }    ErrorStatus;

#define IS_FUNCTIONAL_STATE(S)  (((S) == DISABLE) || ((S) == ENABLE))

#define SET_BIT(REG, BIT)                ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)              ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)               ((REG) & (BIT))
#define CLEAR_REG(REG)                   ((REG) = (0x0U))
#define WRITE_REG(REG, VAL)              ((REG) = (VAL))
#define READ_REG(REG)                    ((REG))
#define MODIFY_REG(REG, CMASK, SMASK)    WRITE_REG((REG), ((READ_REG(REG) & ~(CMASK)) | (SMASK)))
#define POSITION_VAL(VAL)                (__CLZ(__RBIT(VAL)))

/* Atomic 32-bit register access macros (LDREX/STREX, required by H7 HAL) */
#define ATOMIC_SET_BIT(REG, BIT)                             \
  do {                                                       \
    uint32_t val;                                            \
    do {                                                     \
      val = __LDREXW((__IO uint32_t *)&(REG)) | (BIT);       \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U); \
  } while(0)

#define ATOMIC_CLEAR_BIT(REG, BIT)                           \
  do {                                                       \
    uint32_t val;                                            \
    do {                                                     \
      val = __LDREXW((__IO uint32_t *)&(REG)) & ~(BIT);      \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U); \
  } while(0)

#define ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)                          \
  do {                                                                     \
    uint32_t val;                                                          \
    do {                                                                   \
      val = (__LDREXW((__IO uint32_t *)&(REG)) & ~(CLEARMSK)) | (SETMASK); \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U);               \
  } while(0)

#if defined(USE_HAL_DRIVER)
#  include "stm32h7xx_hal.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32H7xx_H */
