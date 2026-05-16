/**
 * @file   wdog.c
 * @author Subhajit Roy (subhajitroy005@gmail.com)
 * @brief  Hardware IWDG + multi-task software watchdog implementation
 *
 * @details
 * Hardware IWDG:
 *   STM32H7 → IWDG1, LSI/64, reload 2000 → ~4 s timeout
 *   STM32F4 → IWDG,  LSI/64, reload 2000 → ~4 s timeout
 *   The IWDG starts immediately in wdog_hw_init() and cannot be stopped.
 *   wdog_service_thread() kicks it every WDOG_CHECK_PERIOD_MS iff all
 *   required software slots have been updated since the last check.
 *
 * Software task watchdog:
 *   _kick_mask  — volatile bitmask; each task ORs in its bit via wdog_task_kick()
 *   _req_mask   — set by wdog_sw_init(); bits that must be set each period
 *   _missed     — counter of consecutive periods where _req_mask was not fully met
 *   On WDOG_MAX_MISSED_CHECKS consecutive missed periods → safety_enter_safe_state()
 */

#include <safety/wdog.h>
#include <safety/safe_state.h>
#include <log/slog.h>
#include <os/kernel_syscall.h>
#include <os/kernel.h>
#include <conf_os.h>
#include <def_attributes.h>

#include <FreeRTOS.h>
#include <task.h>

#include <device.h>  /* HAL_IWDG_Init, HAL_IWDG_Refresh */

/* ── Module state ─────────────────────────────────────────────────────────── */

static volatile uint32_t _kick_mask = 0U;   /* task kick bitmask (atomic write) */
static volatile uint32_t _req_mask  = 0U;   /* required slots bitmask           */
static volatile uint32_t _missed    = 0U;   /* consecutive missed check count    */
static volatile uint32_t _last_mask = 0U;   /* snapshot at last check (debug)   */

static IWDG_HandleTypeDef _hiwdg;

/* ── Hardware IWDG init ──────────────────────────────────────────────────── */

void wdog_hw_init(void)
{
    /* IWDG clocked by LSI (~32 kHz).
     * Prescaler /64 → 32000/64 = 500 Hz → tick = 2 ms.
     * Reload = 2000 → timeout = 2000 × 2 ms = 4000 ms. */
#if defined(STM32H7)
    _hiwdg.Instance       = IWDG1;
    _hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    _hiwdg.Init.Reload    = 2000U;
    _hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;
#else
    _hiwdg.Instance       = IWDG;
    _hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    _hiwdg.Init.Reload    = 2000U;
#endif

    HAL_IWDG_Init(&_hiwdg);
    LOG_I("WDOG", "IWDG started (timeout ~%ums)", WDOG_HW_TIMEOUT_MS);
}

/* ── Software watchdog init ──────────────────────────────────────────────── */

void wdog_sw_init(uint32_t required_slot_mask)
{
    _req_mask  = required_slot_mask;
    _kick_mask = 0U;
    _missed    = 0U;
    LOG_I("WDOG", "SW watchdog mask=0x%02X", (unsigned)required_slot_mask);
}

/* ── Task kick (called from task context) ────────────────────────────────── */

void wdog_task_kick(uint32_t slot_id)
{
    if (slot_id < WDOG_SLOT_COUNT)
    {
        /* Single-word OR is atomic on Cortex-M without critical section.
         * On CM4/CM7 with single-issue bus, this is effectively atomic for
         * a single STRD/STR instruction — safer than a RMW sequence.        */
        __disable_irq();
        _kick_mask |= (1U << slot_id);
        __enable_irq();
    }
}

/* ── Diagnostic accessors ────────────────────────────────────────────────── */

uint32_t wdog_get_kick_mask(void)    { return _last_mask; }
uint32_t wdog_get_missed_checks(void) { return _missed; }

/* ── Watchdog service thread ─────────────────────────────────────────────── */

static void wdog_service_thread(void *arg)
{
    (void)arg;

    /* Startup grace period: allow critical tasks to reach their first kick */
    vTaskDelay(pdMS_TO_TICKS(WDOG_STARTUP_DELAY_MS));
    LOG_I("WDOG", "Watchdog service active, req_mask=0x%02X", (unsigned)_req_mask);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(WDOG_CHECK_PERIOD_MS));

        /* Snapshot and clear atomically */
        __disable_irq();
        uint32_t snapshot = _kick_mask;
        _kick_mask        = 0U;
        __enable_irq();

        _last_mask = snapshot;

        /* Check: all required slots kicked? */
        if ((snapshot & _req_mask) == _req_mask)
        {
            /* All tasks alive — kick hardware IWDG */
            HAL_IWDG_Refresh(&_hiwdg);
            _missed = 0U;
        }
        else
        {
            _missed++;
            uint32_t missing = _req_mask & ~snapshot;
            LOG_W("WDOG", "Missed check #%u, missing slots=0x%02X",
                  (unsigned)_missed, (unsigned)missing);

            if (_missed >= WDOG_MAX_MISSED_CHECKS)
            {
                LOG_E("WDOG", "Max missed checks reached — entering safe state");
                safety_enter_safe_state(SAFE_REASON_WATCHDOG);
                /* safety_enter_safe_state never returns */
            }
            else
            {
                /* Still kick IWDG to prevent premature hardware reset while
                 * the software watchdog is tracking the miss count.  When
                 * WDOG_MAX_MISSED_CHECKS is reached we enter safe state
                 * before the IWDG times out. */
                HAL_IWDG_Refresh(&_hiwdg);
            }
        }
    }
}

/* ── Service start ───────────────────────────────────────────────────────── */

int32_t wdog_service_start(void)
{
    BaseType_t result = xTaskCreate(
        wdog_service_thread,
        "WDOG",
        PROC_SERVICE_WDOG_STACK_SIZE,
        NULL,
        PROC_SERVICE_WDOG_PRIORITY,
        NULL);

    if (result != pdPASS)
    {
        LOG_E("WDOG", "Failed to create watchdog service task");
        return OS_ERR_OP;
    }

    return OS_ERR_NONE;
}
