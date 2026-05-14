/**
 * @file    drv_adc.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   Generic ADC driver with HAL abstraction
 *
 * @note  This file is part of FreeRTOS-OS Project.
 * @license  GPLv3 — see <https://www.gnu.org/licenses/>.
 */

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>

#include <drivers/drv_adc.h>
#include <board/board_config.h>

#if defined(BOARD_ADC_COUNT) && (BOARD_ADC_COUNT > 0)

/* ── Handle storage ───────────────────────────────────────────────────── */
__SECTION_OS_DATA __USED
static drv_adc_handle_t _adc_handles[BOARD_ADC_COUNT];

/* ── Registration ─────────────────────────────────────────────────────── */

/**
 * @brief Register ADC device and initialize hardware.
 *
 * Called by adc_mgmt_thread at startup.
 */
__SECTION_OS __USED
int32_t drv_adc_register(uint8_t dev_id,
                         const drv_adc_hal_ops_t *ops)
{
    if (dev_id >= BOARD_ADC_COUNT || ops == NULL)
        return OS_ERR_OP;

    drv_adc_handle_t *h = &_adc_handles[dev_id];

    h->dev_id      = dev_id;
    h->ops         = ops;
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;

    int32_t err = h->ops->hw_init(h);
    if (err != OS_ERR_NONE)
        return err;

    return OS_ERR_NONE;
}

/* ── Handle accessor ──────────────────────────────────────────────────── */

__SECTION_OS __USED
drv_adc_handle_t *drv_adc_get_handle(uint8_t dev_id)
{
    if (dev_id >= BOARD_ADC_COUNT)
        return NULL;
    return &_adc_handles[dev_id];
}

/* ── Start / Stop ─────────────────────────────────────────────────────── */

__SECTION_OS __USED
int32_t drv_adc_start(uint8_t dev_id)
{
    drv_adc_handle_t *h = drv_adc_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    return h->ops->start(h);
}

__SECTION_OS __USED
int32_t drv_adc_stop(uint8_t dev_id)
{
    drv_adc_handle_t *h = drv_adc_get_handle(dev_id);
    if (h == NULL || !h->initialized || h->ops == NULL)
        return OS_ERR_OP;
    return h->ops->stop(h);
}

#endif /* BOARD_ADC_COUNT > 0 */
