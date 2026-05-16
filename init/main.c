/**
 * @file    main.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  core
 * @brief   Primary OS boot entry point — medical device grade boot state machine
 *
 * @details
 * Boot sequence is implemented as an explicit state machine so every phase
 * has a named identity traceable to IEC 62304 software unit design.
 *
 * At entry from Reset_Handler (after .data copy / .bss clear):
 *
 *  BOOT_HAL_INIT       — vendor HAL_Init(), enable FPU/cache
 *  BOOT_CLK_INIT       — generic clock driver, PLL / HSI
 *  BOOT_PERIPH_CLK     — global peripheral bus clock enable
 *  BOOT_IRQ_INIT       — NVIC group config, IRQ descriptor table
 *  BOOT_IPC_INIT       — IPC message queue subsystem
 *  BOOT_SLOG_INIT      — structured logger (ring buffer, pre-scheduler)
 *  BOOT_PREV_FAULT_CHK — check .noinit for fault / safe-state record
 *  BOOT_SERVICES_INIT  — register all OS service threads
 *  BOOT_WDOG_HW_INIT   — start hardware IWDG (irreversible)
 *  BOOT_APP_INIT       — call app_main() (creates app tasks)
 *  BOOT_SCHEDULER      — hand off to vTaskStartScheduler()
 *
 * On any failure: log via slog (ERROR level), enter safe state.
 *
 * Application integration:
 *  - Override app_main() with __WEAK semantics
 *  - app_main() creates tasks, configures software watchdog, returns 0
 *  - Scheduler starts only after app_main() returns successfully
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <device.h>
#include <board/board_config.h>
#include <board/irq_hw_init_generated.h>

#include <drivers/drv_cpu.h>
#include <drivers/drv_irq.h>
#include <drivers/drv_rcc.h>

#include <os/kernel.h>
#include <os/kernel_services.h>
#include <ipc/mqueue.h>

#include <log/slog.h>
#include <safety/safe_state.h>
#include <safety/fault_handler.h>

#if defined(CONFIG_INC_SERVICE_WDOG) && (CONFIG_INC_SERVICE_WDOG == 1)
#  include <safety/wdog.h>
#endif

/* ── Boot phase enumeration ───────────────────────────────────────────────── */

typedef enum
{
    BOOT_HAL_INIT       = 0,
    BOOT_CLK_INIT       = 1,
    BOOT_PERIPH_CLK     = 2,
    BOOT_IRQ_INIT       = 3,
    BOOT_IPC_INIT       = 4,
    BOOT_SLOG_INIT      = 5,
    BOOT_PREV_FAULT_CHK = 6,
    BOOT_SERVICES_INIT  = 7,
    BOOT_WDOG_HW_INIT   = 8,
    BOOT_APP_INIT       = 9,
    BOOT_SCHEDULER      = 10,
} boot_phase_t;

/* Current boot phase — readable from debugger */
static volatile boot_phase_t _boot_phase = BOOT_HAL_INIT;

/* ── Weak application entry point ─────────────────────────────────────────── */

__WEAK
int app_main(void)
{
    return OS_ERR_NONE;
}

/* ── Boot helper: log phase entry, update tracker ────────────────────────── */

static void _boot_enter(boot_phase_t phase, const char *name)
{
    _boot_phase = phase;
    LOG_I("BOOT", "Phase %d: %s", (int)phase, name);
}

/* ── OS boot entry point ──────────────────────────────────────────────────── */

__SECTION_BOOT __USED
void main(void)
{
    int32_t st;

    /* ── BOOT_HAL_INIT ──────────────────────────────────────────────────── */
    _boot_enter(BOOT_HAL_INIT, "HAL_INIT");

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    if (HAL_Init() == HAL_ERROR)
        safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);
#endif

    /* ── BOOT_CLK_INIT ──────────────────────────────────────────────────── */
    _boot_enter(BOOT_CLK_INIT, "CLK_INIT");
    st = drv_rcc_clock_init();
    if (st != OS_ERR_NONE)
        safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);

    /* ── BOOT_PERIPH_CLK ────────────────────────────────────────────────── */
    _boot_enter(BOOT_PERIPH_CLK, "PERIPH_CLK");
    board_clk_enable();

    /* ── BOOT_IRQ_INIT ──────────────────────────────────────────────────── */
    _boot_enter(BOOT_IRQ_INIT, "IRQ_INIT");
    drv_cpu_interrupt_prio_set();
    irq_hw_init_all();

    /* ── BOOT_IPC_INIT ──────────────────────────────────────────────────── */
    _boot_enter(BOOT_IPC_INIT, "IPC_INIT");
    st = ipc_mqueue_init();
    if (st != OS_ERR_NONE)
        safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);

    /* ── BOOT_SLOG_INIT — must run before any LOG_x() below ─────────────── */
    _boot_enter(BOOT_SLOG_INIT, "SLOG_INIT");
    slog_init();
    LOG_I("BOOT", "FreeRTOS-OS medical-grade boot");

    /* ── BOOT_PREV_FAULT_CHK ────────────────────────────────────────────── */
    _boot_enter(BOOT_PREV_FAULT_CHK, "PREV_FAULT_CHK");

    if (safety_was_safe_state_reset())
    {
        safe_state_reason_t r = safety_get_last_reason();
        LOG_W("BOOT", "Previous session ended in SAFE STATE reason=%d", (int)r);
        safety_clear_record();
    }

    if (fault_handler_was_fault_reset())
    {
        uint32_t pc, lr, cfsr, hfsr, ftype, tick;
        fault_handler_get_fields(&pc, &lr, &cfsr, &hfsr, &ftype, &tick);
        LOG_W("BOOT", "Previous FAULT type=%u PC=0x%08X LR=0x%08X CFSR=0x%08X tick=%u",
              (unsigned)ftype, (unsigned)pc, (unsigned)lr,
              (unsigned)cfsr, (unsigned)tick);
        fault_handler_clear();
    }

    /* ── BOOT_SERVICES_INIT ─────────────────────────────────────────────── */
    _boot_enter(BOOT_SERVICES_INIT, "SERVICES_INIT");
    st = os_kernel_thread_register();
    if (st != OS_ERR_NONE)
    {
        LOG_E("BOOT", "Service registration failed (status=%d)", (int)st);
        safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);
    }

    /* ── BOOT_WDOG_HW_INIT — start IWDG (cannot be stopped after this) ─── */
    _boot_enter(BOOT_WDOG_HW_INIT, "WDOG_HW_INIT");
#if defined(CONFIG_INC_SERVICE_WDOG) && (CONFIG_INC_SERVICE_WDOG == 1)
    wdog_hw_init();
#endif

    /* ── BOOT_APP_INIT ──────────────────────────────────────────────────── */
    _boot_enter(BOOT_APP_INIT, "APP_INIT");
    st = app_main();
    if (st != OS_ERR_NONE)
    {
        LOG_E("BOOT", "app_main() failed (status=%d)", (int)st);
        safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);
    }

    /* ── BOOT_SCHEDULER ─────────────────────────────────────────────────── */
    _boot_enter(BOOT_SCHEDULER, "SCHEDULER_START");
    LOG_I("BOOT", "Handing off to FreeRTOS scheduler");

    vTaskStartScheduler();

    /* vTaskStartScheduler() should never return (only if heap exhausted) */
    LOG_E("BOOT", "vTaskStartScheduler returned — heap likely exhausted");
    safety_enter_safe_state(SAFE_REASON_BOOT_FAIL);
}
