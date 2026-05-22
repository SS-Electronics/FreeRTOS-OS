/*
 * trustcore.h — Trust Core for the STM32U575ZI example.
 *
 * Author:      Subhajit Roy  subhajitroy005@gmail.com
 * Module:      examples/stm32u575/trustcore
 *
 * Role
 * ────
 *   Anchors the trusted boot of the STM32U5 example. On the M33 + TrustZone
 *   capable STM32U575 the trust core is the place that:
 *
 *     1. Programs the Security Attribution Unit (SAU) so the address map
 *        is split into S / NS regions as desired. The default build is
 *        single-image (non-secure) — the SAU is left in its reset
 *        ALLNS=1 state and trustcore is only the *attestation* anchor.
 *
 *     2. Reads the factory-programmed STM32 96-bit Unique Device ID from
 *        the system memory area and exposes it as the boot identity.
 *
 *     3. Computes a CRC32 over the vector table + .text/.trustcore_text
 *        ranges at boot and compares against a build-time expected value
 *        (computed at first boot and stashed in the .noinit region so
 *        it survives soft resets).
 *
 *     4. Emits a single attestation banner over the debug UART summarising
 *        device identity + integrity status — the application can then
 *        gate its main loop on trustcore_get_status() before doing any
 *        sensor reads or actuator drives.
 *
 *   Calling order
 *   ─────────────
 *     trustcore_init_secure_world()   — from Reset_Handler, BEFORE main().
 *                                       Programs SAU. Strong override of the
 *                                       weak symbol in the startup file.
 *     trustcore_attest_runtime()      — from app_main() AFTER printk is up.
 *                                       Logs the banner, runs the integrity
 *                                       check, sets trustcore_get_status().
 *
 *   Why two phases?
 *   ───────────────
 *     SAU/GTZC programming MUST happen before any RAM access if the secure
 *     image owns SRAM regions — otherwise .data copy / .bss clear writes
 *     non-secure addresses that have already been claimed by the secure
 *     side. The attestation banner needs printk → it has to wait until
 *     the OS UART service is online.
 *
 *  This file is part of FreeRTOS-OS Project.
 *  FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 */

#ifndef EXAMPLES_STM32U575_TRUSTCORE_H_
#define EXAMPLES_STM32U575_TRUSTCORE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Trust status codes ──────────────────────────────────────────────────── */

typedef enum {
    TRUSTCORE_STATUS_UNCHECKED = 0,  /**< trustcore_attest_runtime() not yet run */
    TRUSTCORE_STATUS_OK,             /**< Identity read + integrity check passed */
    TRUSTCORE_STATUS_FIRST_BOOT,     /**< Expected hash baseline captured on this boot */
    TRUSTCORE_STATUS_INTEGRITY_FAIL, /**< Integrity hash mismatch */
    TRUSTCORE_STATUS_IDENTITY_FAIL,  /**< UID read returned zeroes / sentinel */
} trustcore_status_t;

/* ── Device identity (STM32 96-bit UID, formatted) ───────────────────────── */

typedef struct {
    uint32_t word0;   /**< UID[31:0]   — lot wafer X/Y                       */
    uint32_t word1;   /**< UID[63:32]  — lot LOT_NUM (low)                   */
    uint32_t word2;   /**< UID[95:64]  — lot LOT_NUM (high)                  */
} trustcore_uid_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief  Secure-world bring-up hook (strong override of the weak symbol in
 *         startup_stm32u575zitxq.c). Called from Reset_Handler BEFORE the
 *         C-runtime is initialised — therefore must not use static initialised
 *         data or call into stdio / printf / RTOS APIs.
 */
void trustcore_init_secure_world(void);

/**
 * @brief  Runtime attestation. Called from app_main() AFTER printk is up.
 *         Logs the boot banner, runs the integrity check, and sets the
 *         status that trustcore_get_status() returns.
 *
 * @return Final trust status (also retrievable via trustcore_get_status()).
 */
trustcore_status_t trustcore_attest_runtime(void);

/**
 * @brief  Current trust status. Defaults to TRUSTCORE_STATUS_UNCHECKED
 *         until trustcore_attest_runtime() has run.
 */
trustcore_status_t trustcore_get_status(void);

/**
 * @brief  Fetch the formatted device UID. Always succeeds on STM32U5;
 *         the same value would be returned across resets.
 */
void trustcore_get_uid(trustcore_uid_t *out);

/**
 * @brief  Returns true when the firmware is in the secure-image partition.
 *         For the single-image (non-secure) example this is always false.
 */
bool trustcore_is_secure_world(void);

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLES_STM32U575_TRUSTCORE_H_ */
