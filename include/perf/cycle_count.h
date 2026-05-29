/**
 * @file    cycle_count.h
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 * @brief   Cortex-M DWT cycle-counter helpers for cycle-accurate latency timing.
 *
 * @details
 * Header-only, no dependency on CMSIS or the OS. Uses the Data Watchpoint and
 * Trace (DWT) unit present on Cortex-M3/M4/M7/M33. CYCCNT is a free-running
 * 32-bit counter incrementing once per CPU clock; at the H723's 550 MHz it wraps
 * every ~7.8 s, which bounds a single timed region — fine for per-inference
 * benchmarking (sub-second). Convert cycles → microseconds with the core clock.
 *
 * Register addresses (ARMv7-M / ARMv8-M debug block):
 *   CoreDebug->DEMCR  0xE000EDFC   bit24 TRCENA enables the trace subsystem
 *   DWT->CTRL         0xE0001000   bit0  CYCCNTENA enables the cycle counter
 *   DWT->CYCCNT       0xE0001004   the 32-bit cycle count
 *
 * Usage:
 * @code
 *   cycle_count_enable();
 *   uint32_t t0 = cycle_count_get();
 *   work();
 *   uint32_t cycles = cycle_count_get() - t0;   // wrap-safe (mod 2^32)
 *   uint32_t us = cycle_count_to_us(cycles, SystemCoreClock);
 * @endcode
 */

#ifndef OS_PERF_CYCLE_COUNT_H_
#define OS_PERF_CYCLE_COUNT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYCCNT_DEMCR    (*(volatile uint32_t *)0xE000EDFCUL)
#define CYCCNT_DWT_CTRL (*(volatile uint32_t *)0xE0001000UL)
#define CYCCNT_DWT_CYC  (*(volatile uint32_t *)0xE0001004UL)

#define CYCCNT_TRCENA    (1UL << 24)
#define CYCCNT_CYCCNTENA (1UL << 0)

/** @brief Enable the DWT cycle counter (idempotent). */
static inline void cycle_count_enable(void)
{
    CYCCNT_DEMCR    |= CYCCNT_TRCENA;
    CYCCNT_DWT_CTRL |= CYCCNT_CYCCNTENA;
}

/** @brief Reset CYCCNT to zero. */
static inline void cycle_count_reset(void)
{
    CYCCNT_DWT_CYC = 0U;
}

/** @brief Current cycle count. Differences are wrap-safe modulo 2^32. */
static inline uint32_t cycle_count_get(void)
{
    return CYCCNT_DWT_CYC;
}

/**
 * @brief Convert a cycle count to microseconds.
 * @param cycles    Elapsed CPU cycles.
 * @param cpu_hz    Core clock in Hz (e.g. SystemCoreClock).
 * @return Elapsed microseconds (0 if cpu_hz is 0).
 */
static inline uint32_t cycle_count_to_us(uint32_t cycles, uint32_t cpu_hz)
{
    if (cpu_hz == 0U)
        return 0U;
    /* (cycles * 1e6) / cpu_hz, kept in 64-bit to avoid overflow. */
    return (uint32_t)(((uint64_t)cycles * 1000000ULL) / (uint64_t)cpu_hz);
}

#ifdef __cplusplus
}
#endif

#endif /* OS_PERF_CYCLE_COUNT_H_ */
