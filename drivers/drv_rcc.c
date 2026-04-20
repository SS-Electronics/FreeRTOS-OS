/*
 * drv_rcc.c — Reset and Clock Control portable driver
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Vendor selection is compile-time: drv_rcc_clock_init() calls the
 * appropriate _hal_rcc_*_register() shim, which populates the ops table.
 * All HAL calls remain isolated in their respective hal_rcc_*.c files.
 */

#include <stddef.h>
#include <drivers/drv_rcc.h>
#include <board/mcu_config.h>
#include <def_err.h>

/* Forward declarations — each vendor backend exposes exactly one symbol */
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
void _hal_rcc_stm32_register(const drv_rcc_hal_ops_t **ops_out);
#endif

/* ── Singleton ops table ─────────────────────────────────────────────────── */

static const drv_rcc_hal_ops_t *_ops;

int32_t drv_rcc_register(const drv_rcc_hal_ops_t *ops)
{
    if (ops == NULL)
        return OS_ERR_OP;
    _ops = ops;
    return OS_ERR_NONE;
}

/* ── Driver API ──────────────────────────────────────────────────────────── */

int32_t drv_rcc_clock_init(void)
{
#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
    _hal_rcc_stm32_register(&_ops);
#endif

    if (_ops == NULL || _ops->clock_init == NULL)
        return OS_ERR_OP;

    return _ops->clock_init();
}

uint32_t drv_rcc_get_sysclk_hz(void)
{
    if (_ops && _ops->get_sysclk_hz)
        return _ops->get_sysclk_hz();
    return 0U;
}

uint32_t drv_rcc_get_apb1_hz(void)
{
    if (_ops && _ops->get_apb1_hz)
        return _ops->get_apb1_hz();
    return 0U;
}

uint32_t drv_rcc_get_apb2_hz(void)
{
    if (_ops && _ops->get_apb2_hz)
        return _ops->get_apb2_hz();
    return 0U;
}
