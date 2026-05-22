/*
 * trustcore.c — Trust Core implementation for the STM32U575ZI example.
 *
 * Author:      Subhajit Roy  subhajitroy005@gmail.com
 * Module:      examples/stm32u575/trustcore
 *
 * See trustcore.h for the calling-order contract and why this module exists.
 *
 * This file is part of FreeRTOS-OS Project.
 * FreeRTOS-OS is free software; see <https://www.gnu.org/licenses/>.
 */

#include "trustcore.h"

#include <stdint.h>
#include <string.h>

#include <os/kernel_syscall.h>    /* printk()                              */

/* STM32 96-bit Unique Device ID factory-programmed location on U5. RM0456
 * Table 7 (System memory map): UID base 0x0BFA0700. The same constants are
 * exposed by the vendor stm32u575xx.h once the upstream CMSIS header is
 * dropped in — until then we define them locally so this file compiles
 * against the stub. */
#ifndef UID_BASE
#  define UID_BASE   0x0BFA0700UL
#endif

/* Linker-provided range of code we want to fingerprint. The .trustcore_text
 * section is reserved for secure-callable entries; here we just include it
 * in the hash. */
extern uint32_t _sidata;         /* end of FLASH .text region (LMA marker) */
extern uint32_t __trustcore_text_start;
extern uint32_t __trustcore_text_end;

/* Vector table base — placed at FLASH origin by the linker script. */
extern const void *g_pfnVectors[];

/* ── .noinit baseline — survives soft resets ─────────────────────────────
 * On first boot we capture the expected CRC32 here. On subsequent boots we
 * recompute and compare. .noinit is placed outside __bss_start__/__bss_end__
 * in the linker script so the Reset_Handler BSS clear loop does not wipe it. */
__attribute__((section(".noinit")))
static struct {
    uint32_t magic;
    uint32_t expected_crc;
    uint32_t boot_count;
} _trustcore_baseline;

#define TRUSTCORE_NOINIT_MAGIC   0x54525354UL   /* 'TRST' */

/* ── State ───────────────────────────────────────────────────────────────── */

static trustcore_status_t _status        = TRUSTCORE_STATUS_UNCHECKED;
static bool               _secure_world  = false;
static trustcore_uid_t    _cached_uid    = {0, 0, 0};

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* CRC32 — IEEE 802.3 polynomial, byte-at-a-time. Slow but tiny; runs once
 * per boot over ~50 KB so cost is negligible (~few ms at 160 MHz). The U5
 * CRC peripheral could be used instead — left for the integrity-hardening
 * follow-up so this stays vendor-HAL-agnostic. */
static uint32_t _crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    crc = ~crc;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
            uint32_t mask = -(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static uint32_t _hash_code_region(void)
{
    const uint8_t *start = (const uint8_t *)g_pfnVectors;
    const uint8_t *end   = (const uint8_t *)&_sidata;  /* LMA of .data */
    if (end <= start) {
        return 0u;
    }
    uint32_t crc = _crc32_update(0u, start, (uint32_t)(end - start));

    /* Fold in the .trustcore_text range so a tampered SAU entry is caught. */
    const uint8_t *tc_s = (const uint8_t *)&__trustcore_text_start;
    const uint8_t *tc_e = (const uint8_t *)&__trustcore_text_end;
    if (tc_e > tc_s) {
        crc = _crc32_update(crc, tc_s, (uint32_t)(tc_e - tc_s));
    }
    return crc;
}

static void _read_uid(trustcore_uid_t *out)
{
    /* The UID lives in factory-programmed system memory. Reading is
     * unconditional — three contiguous 32-bit words. */
    volatile const uint32_t *uid = (const uint32_t *)UID_BASE;
    out->word0 = uid[0];
    out->word1 = uid[1];
    out->word2 = uid[2];
}

/* ── Secure-world hook (early Reset_Handler) ─────────────────────────────── */

void trustcore_init_secure_world(void)
{
    /* On the single-image example the SAU is left in its reset state
     * (ALLNS=1 → whole map is non-secure for the CPU). When the user
     * sets CONFIG_U5_TRUSTZONE_ENABLED=y and links the matching secure
     * image, this function is the place to:
     *
     *     SAU->RNR  = 0;
     *     SAU->RBAR = NS_REGION_BASE;
     *     SAU->RLAR = NS_REGION_LIMIT | (1u << 0);          // ENABLE
     *     SAU->CTRL = SAU_CTRL_ENABLE_Msk;                  // SAU on
     *     SCB->NSACR |= 0x3u << 10 | 0x3u << 11;            // FPU NS access
     *
     * Capture the build-time identity here too — running before main() means
     * the UID has *always* been read before the application can touch the
     * MCU's identity registers, so any application-side spoofing is harder. */
    _read_uid(&_cached_uid);

    /* Reserved for future TZ programming. Today we sit in non-secure world. */
    _secure_world = false;
}

/* ── Runtime attestation (post-printk) ────────────────────────────────────── */

trustcore_status_t trustcore_attest_runtime(void)
{
    if (_cached_uid.word0 == 0u && _cached_uid.word1 == 0u &&
        _cached_uid.word2 == 0u) {
        /* trustcore_init_secure_world wasn't called or the chip is silicon
         * with a zeroed UID (engineering samples). Re-read just in case. */
        _read_uid(&_cached_uid);
    }

    if (_cached_uid.word0 == 0u && _cached_uid.word1 == 0u &&
        _cached_uid.word2 == 0u) {
        _status = TRUSTCORE_STATUS_IDENTITY_FAIL;
        printk("[trustcore] FAIL: device UID reads all-zero — refusing to attest\n");
        return _status;
    }

    uint32_t computed = _hash_code_region();

    /* First boot — establish the baseline. Without an external secure
     * provisioning step there is no way to know the "right" CRC at this
     * point, so we trust it on the first boot and detect change-after-first
     * across subsequent soft resets. A production flow would replace this
     * with a signed hash baked into a sealed region. */
    if (_trustcore_baseline.magic != TRUSTCORE_NOINIT_MAGIC) {
        _trustcore_baseline.magic        = TRUSTCORE_NOINIT_MAGIC;
        _trustcore_baseline.expected_crc = computed;
        _trustcore_baseline.boot_count   = 1u;
        _status = TRUSTCORE_STATUS_FIRST_BOOT;
    } else if (_trustcore_baseline.expected_crc != computed) {
        _trustcore_baseline.boot_count++;
        _status = TRUSTCORE_STATUS_INTEGRITY_FAIL;
    } else {
        _trustcore_baseline.boot_count++;
        _status = TRUSTCORE_STATUS_OK;
    }

    /* ── Boot banner ──────────────────────────────────────────────────── */

    printk("[trustcore] ───────────────────────────────────────────────\n");
    printk("[trustcore]  STM32U575ZI — Cortex-M33 + TrustZone (U5)\n");
    printk("[trustcore]  Device UID : %08lx-%08lx-%08lx\n",
           (unsigned long)_cached_uid.word2,
           (unsigned long)_cached_uid.word1,
           (unsigned long)_cached_uid.word0);
    printk("[trustcore]  World      : %s\n",
           _secure_world ? "SECURE" : "non-secure (single-image)");
    printk("[trustcore]  Code CRC32 : 0x%08lx\n", (unsigned long)computed);
    printk("[trustcore]  Boot count : %lu\n",
           (unsigned long)_trustcore_baseline.boot_count);

    switch (_status) {
    case TRUSTCORE_STATUS_OK:
        printk("[trustcore]  Status     : OK — integrity verified\n");
        break;
    case TRUSTCORE_STATUS_FIRST_BOOT:
        printk("[trustcore]  Status     : FIRST_BOOT — baseline captured\n");
        break;
    case TRUSTCORE_STATUS_INTEGRITY_FAIL:
        printk("[trustcore]  Status     : INTEGRITY_FAIL — code region changed\n");
        printk("[trustcore]               expected=0x%08lx  got=0x%08lx\n",
               (unsigned long)_trustcore_baseline.expected_crc,
               (unsigned long)computed);
        break;
    default:
        printk("[trustcore]  Status     : %d\n", (int)_status);
        break;
    }
    printk("[trustcore] ───────────────────────────────────────────────\n");

    return _status;
}

/* ── Accessors ───────────────────────────────────────────────────────────── */

trustcore_status_t trustcore_get_status(void)
{
    return _status;
}

void trustcore_get_uid(trustcore_uid_t *out)
{
    if (!out) return;
    if (_cached_uid.word0 == 0u && _cached_uid.word1 == 0u &&
        _cached_uid.word2 == 0u) {
        _read_uid(&_cached_uid);
    }
    *out = _cached_uid;
}

bool trustcore_is_secure_world(void)
{
    return _secure_world;
}
