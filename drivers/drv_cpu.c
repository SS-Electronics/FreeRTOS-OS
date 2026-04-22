/*
 * drv_cpu.c — CPU-level driver: NVIC setup, reset, fault handlers, watchdog
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Covers two concerns:
 *   1. CPU / NVIC utilities (interrupt priority, MCU reset, ARM fault handlers)
 *      — no handle needed, call directly.
 *   2. Watchdog (WDG) peripheral — singleton ops-table pattern matching
 *      drv_uart.c.  The vendor HAL backend populates a drv_wdg_hal_ops_t and
 *      calls drv_wdg_register() at startup.
 */

#include <drivers/drv_handle.h>
#include <board/mcu_config.h>
#include <def_err.h>

/* ── CPU / NVIC utilities ─────────────────────────────────────────────── */

void drv_cpu_interrupt_prio_set(void)
{
#if (__ARM_ARCH_7A__ == 0U)
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif
}

void reset_mcu(void)
{
    NVIC_SystemReset();
}

/* ── Watchdog driver ──────────────────────────────────────────────────── */

#if (CONFIG_MCU_WDG_EN == 1)

static drv_wdg_handle_t _wdg_handle;

int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops)
{
    if (ops == NULL)
        return OS_ERR_OP;

    _wdg_handle.ops         = ops;
    _wdg_handle.initialized = 0;
    _wdg_handle.last_err    = OS_ERR_NONE;

    int32_t err = _wdg_handle.ops->hw_init(&_wdg_handle);
    if (err != OS_ERR_NONE)
        return err;

    _wdg_handle.initialized = 1;
    return OS_ERR_NONE;
}

drv_wdg_handle_t *drv_wdg_get_handle(void)
{
    return &_wdg_handle;
}

void drv_wdg_refresh(void)
{
    if (!_wdg_handle.initialized || _wdg_handle.ops == NULL)
        return;
    _wdg_handle.ops->refresh(&_wdg_handle);
}

#else

int32_t drv_wdg_register(const drv_wdg_hal_ops_t *ops)
{
    (void)ops;
    return OS_ERR_NONE;
}

drv_wdg_handle_t *drv_wdg_get_handle(void) { return NULL; }
void              drv_wdg_refresh(void)     {}

#endif /* CONFIG_MCU_WDG_EN */

/* Fault handlers are defined in drivers/hal/stm32/stm32_exceptions.c */

/* ── FreeRTOS static allocation hooks ────────────────────────────────────── */

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    static StaticTask_t _idle_tcb;
    static StackType_t  _idle_stack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &_idle_tcb;
    *ppxIdleTaskStackBuffer = _idle_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    static StaticTask_t _timer_tcb;
    static StackType_t  _timer_stack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer   = &_timer_tcb;
    *ppxTimerTaskStackBuffer = _timer_stack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
