/**
 * @file    hal_adc_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL ADC backend — continuous EOC interrupt
 *
 * @details
 * Implements drv_adc_hal_ops_t for STM32H7 and STM32F4.
 *
 * Before calling drv_adc_register(), the caller must:
 *   hal_adc_stm32_set_config(&handle, ADC1, ADC_CHANNEL_10,
 *                            ADC_RESOLUTION_12B, ADC_SAMPLETIME_64CYCLES_5);
 *
 * This stores channel and sample_time in drv_hw_adc_ctx_t.channel/.sample_time
 * so hw_init() can retrieve them after HAL_ADC_Init() resets the Init struct.
 *
 * IRQ flow:
 *   ADC_IRQHandler → hal_adc_stm32_irq_handler(ADC1)
 *     → reads DR, clears EOC flag
 *     → drv_irq_dispatch_from_isr(IRQ_ID_ADC_DATA_READY(dev_id), &raw, &hpt)
 *
 * @note  This file is part of FreeRTOS-OS Project.
 * @license  GPLv3 — see <https://www.gnu.org/licenses/>.
 */

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#if defined(BOARD_ADC_COUNT) && (BOARD_ADC_COUNT > 0)

#include <device.h>
#include <def_attributes.h>
#include <def_err.h>

#include <drivers/drv_adc.h>
#include <drivers/hal/stm32/hal_adc_stm32.h>
#include <drivers/drv_irq.h>
#include <board/board_config.h>

/* ── H7 / F4 ADC register compatibility shim ─────────────────────────── */
#if defined(STM32H7)
#  define _ADC_DR(inst)      (READ_REG((inst)->DR) & 0x0000FFFFUL)
#  define _ADC_ISR(inst)     READ_REG((inst)->ISR)
#  define ADC_FLAG_EOC       ADC_ISR_EOC
#  define ADC_FLAG_OVR       ADC_ISR_OVR
#  define _ADC_CLR_EOC(inst) WRITE_REG((inst)->ISR, ADC_ISR_EOC | ADC_ISR_EOS)
#  define _ADC_CLR_OVR(inst) WRITE_REG((inst)->ISR, ADC_ISR_OVR)
#else /* STM32F4 */
#  define _ADC_DR(inst)      (READ_REG((inst)->DR) & 0x0000FFFFUL)
#  define _ADC_ISR(inst)     READ_REG((inst)->SR)
#  define ADC_FLAG_EOC       ADC_SR_EOC
#  define ADC_FLAG_OVR       ADC_SR_OVR
#  define _ADC_CLR_EOC(inst) CLEAR_BIT((inst)->SR, ADC_SR_EOC | ADC_SR_STRT)
#  define _ADC_CLR_OVR(inst) CLEAR_BIT((inst)->SR, ADC_SR_OVR)
#endif

/* ── Internal helpers ─────────────────────────────────────────────────── */

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

static drv_adc_handle_t *_find_handle_by_instance(ADC_TypeDef *inst)
{
    for (uint8_t i = 0; i < BOARD_ADC_COUNT; i++) {
        drv_adc_handle_t *h = drv_adc_get_handle(i);
        if (h && h->hw.hadc.Instance == inst)
            return h;
    }
    return NULL;
}

/* ── HAL ops implementations ──────────────────────────────────────────── */

__SECTION_OS __USED
static int32_t _stm32_adc_hw_init(drv_adc_handle_t *h)
{
    ADC_HandleTypeDef *hadc = &h->hw.hadc;

    /* hadc->Instance, h->hw.channel, h->hw.sample_time must be set
     * by hal_adc_stm32_set_config() before this call. */

    hadc->Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc->Init.Resolution            = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait      = DISABLE;
    hadc->Init.ContinuousConvMode    = ENABLE;
    hadc->Init.NbrOfConversion       = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;

#if defined(STM32H7)
    hadc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc->Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc->Init.LeftBitShift          = ADC_LEFTBITSHIFT_NONE;
    hadc->Init.OversamplingMode      = DISABLE;
#else /* F4 */
    hadc->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.DMAContinuousRequests = DISABLE;
#endif

    HAL_StatusTypeDef st = HAL_ADC_Init(hadc);
    if (st != HAL_OK)
        return _hal_to_os_err(st);

    /* ── Channel configuration ── */
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = h->hw.channel;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = h->hw.sample_time;
#if defined(STM32H7)
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    ch.Offset       = 0U;
#endif

    st = HAL_ADC_ConfigChannel(hadc, &ch);
    if (st != HAL_OK)
        return _hal_to_os_err(st);

    /* ── Calibration (H7) ── */
#if defined(STM32H7)
    st = HAL_ADCEx_Calibration_Start(hadc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    if (st != HAL_OK)
        return _hal_to_os_err(st);
#endif

    /* ── NVIC — read priority from board descriptor ── */
    const board_config_t *bc = board_get_config();
    for (uint8_t i = 0; i < bc->adc_count; i++) {
        if (bc->adc_table[i].dev_id == h->dev_id) {
            HAL_NVIC_SetPriority(bc->adc_table[i].irqn,
                                 bc->adc_table[i].irq_priority, 0U);
            HAL_NVIC_EnableIRQ(bc->adc_table[i].irqn);
            break;
        }
    }

    h->initialized = 1;
    return OS_ERR_NONE;
}

__SECTION_OS __USED
static int32_t _stm32_adc_hw_deinit(drv_adc_handle_t *h)
{
    HAL_ADC_Stop_IT(&h->hw.hadc);
    HAL_StatusTypeDef st = HAL_ADC_DeInit(&h->hw.hadc);
    h->initialized = 0;
    return _hal_to_os_err(st);
}

__SECTION_OS __USED
static int32_t _stm32_adc_start(drv_adc_handle_t *h)
{
    __HAL_ADC_ENABLE_IT(&h->hw.hadc, ADC_IT_EOC);
    HAL_StatusTypeDef st = HAL_ADC_Start_IT(&h->hw.hadc);
    return _hal_to_os_err(st);
}

__SECTION_OS __USED
static int32_t _stm32_adc_stop(drv_adc_handle_t *h)
{
    HAL_StatusTypeDef st = HAL_ADC_Stop_IT(&h->hw.hadc);
    return _hal_to_os_err(st);
}

/* ── HAL ops table ────────────────────────────────────────────────────── */
static const drv_adc_hal_ops_t _stm32_adc_ops = {
    .hw_init   = _stm32_adc_hw_init,
    .hw_deinit = _stm32_adc_hw_deinit,
    .start     = _stm32_adc_start,
    .stop      = _stm32_adc_stop,
};

/* ── Public API ───────────────────────────────────────────────────────── */

const drv_adc_hal_ops_t *hal_adc_stm32_get_ops(void)
{
    return &_stm32_adc_ops;
}

void hal_adc_stm32_set_config(drv_adc_handle_t *h,
                              ADC_TypeDef      *instance,
                              uint32_t          channel,
                              uint32_t          resolution,
                              uint32_t          sample_time)
{
    (void)resolution; /* stored in Init.Resolution inside hw_init */
    h->hw.hadc.Instance = instance;
    h->hw.channel       = channel;
    h->hw.sample_time   = sample_time;
}

/* ── IRQ handler ──────────────────────────────────────────────────────── */

void hal_adc_stm32_irq_handler(ADC_TypeDef *instance)
{
    BaseType_t hpt  = pdFALSE;
    uint32_t   isr  = _ADC_ISR(instance);

    if (isr & ADC_FLAG_OVR)
        _ADC_CLR_OVR(instance);

    if (isr & ADC_FLAG_EOC) {
        uint32_t raw = _ADC_DR(instance);   /* read clears EOC on F4 */
        _ADC_CLR_EOC(instance);             /* explicit clear on H7  */

        drv_adc_handle_t *h = _find_handle_by_instance(instance);
        if (h && h->initialized)
            drv_irq_dispatch_from_isr(IRQ_ID_ADC_DATA_READY(h->dev_id),
                                      &raw, &hpt);
    }

    portYIELD_FROM_ISR(hpt);
}

#endif /* BOARD_ADC_COUNT > 0 */

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
