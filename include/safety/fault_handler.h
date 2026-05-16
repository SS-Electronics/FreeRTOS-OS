/**
 * @file   fault_handler.h
 * @author Subhajit Roy (subhajitroy005@gmail.com)
 * @brief  Boot-time fault record API from stm32_exceptions.c
 *
 * @details
 * After an ARM fault (HardFault, MemManage, BusFault, UsageFault), the
 * fault handler persists a full diagnostic record in .noinit RAM and
 * triggers NVIC_SystemReset.  This header exposes the API to check and
 * print that record on the subsequent boot.
 */

#ifndef FREERTOS_OS_INCLUDE_SAFETY_FAULT_HANDLER_H_
#define FREERTOS_OS_INCLUDE_SAFETY_FAULT_HANDLER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Fault record (opaque — layout in stm32_exceptions.c) ───────────────── */
/* Forward-declared as void* for header consumers that only need the pointer. */

/**
 * @brief Check if the previous reset was caused by an ARM fault.
 * @return 1 if a valid fault record is present in .noinit, 0 otherwise.
 */
uint32_t fault_handler_was_fault_reset(void);

/**
 * @brief Return pointers to key fields from the last fault record.
 *
 * @param[out] out_pc          Faulting PC value (NULL to skip)
 * @param[out] out_lr          LR at fault (NULL to skip)
 * @param[out] out_cfsr        Configurable Fault Status Register (NULL to skip)
 * @param[out] out_hfsr        HardFault Status Register (NULL to skip)
 * @param[out] out_fault_type  0=Hard, 1=Mem, 2=Bus, 3=Usage (NULL to skip)
 * @param[out] out_tick        HAL_GetTick() at fault time (NULL to skip)
 */
void fault_handler_get_fields(uint32_t *out_pc,
                               uint32_t *out_lr,
                               uint32_t *out_cfsr,
                               uint32_t *out_hfsr,
                               uint32_t *out_fault_type,
                               uint32_t *out_tick);

/**
 * @brief Invalidate the fault record so it is not reported again next boot.
 */
void fault_handler_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_OS_INCLUDE_SAFETY_FAULT_HANDLER_H_ */
