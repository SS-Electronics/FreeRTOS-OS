/**
 ******************************************************************************
 * @file    stm32u5xx.h
 * @author  Subhajit Roy  subhajitroy005@gmail.com
 * @brief   CMSIS STM32U5xx Device Peripheral Access Layer Header File.
 *
 *          Umbrella header for the STM32U5 family (ARM Cortex-M33 + TrustZone).
 *          Selects the correct device sub-header based on the chip define
 *          injected by the build system (-DSTM32U575xx etc.).
 *
 *          Pattern mirrors STM32CubeU5 Drivers/CMSIS/Device/ST/STM32U5xx/ so
 *          the stm32u5xx-hal-driver can find this file unmodified.
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef __STM32U5xx_H
#define __STM32U5xx_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined  (STM32U5)
#define STM32U5
#endif

/* ── Device variant selection ─────────────────────────────────────────────── */
#if   defined(STM32U575xx)
#  include "stm32u575xx.h"
#elif defined(STM32U585xx)
#  error "STM32U585xx not yet ported — add STM32U585/stm32u585xx.h"
#elif defined(STM32U595xx)
#  error "STM32U595xx not yet ported — add STM32U595/stm32u595xx.h"
#elif defined(STM32U599xx)
#  error "STM32U599xx not yet ported — add STM32U599/stm32u599xx.h"
#elif defined(STM32U5A5xx) || defined(STM32U5A9xx)
#  error "STM32U5Ax not yet ported — add the matching variant header"
#else
#  error "stm32u5xx.h: no STM32U5xx variant defined. Pass -DSTM32U575xx (or similar)."
#endif

/* ── Common STM32 utility macros (shared with F4 / H7) ────────────────────── */
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

/* Atomic 32-bit register access macros (LDREX/STREX, required by U5 HAL).
 * Cortex-M33 supports the same exclusives as M7. */
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

/* The HAL header is pulled in via the upstream stm32u5xx-hal-driver submodule.
 * When that submodule is not checked out the build still links the kernel,
 * startup, and trustcore stub — so guard with __has_include so a fresh clone
 * compiles cleanly until the submodule is added. */
#if defined(USE_HAL_DRIVER) && __has_include("stm32u5xx_hal.h")
#  include "stm32u5xx_hal.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32U5xx_H */
