/*
 * hal_dma_stm32.c — STM32F4 DMA engine HAL backend
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Maps the dma_chan_hal_ops_t vtable onto STM32 HAL DMA primitives.
 * Each dma_chan_t.hw_ctx points to a drv_hw_dma_chan_ctx_t that wraps a
 * DMA_HandleTypeDef plus the stream IRQ number.
 *
 * ISR routing
 * ───────────
 * The 16 weak DMAx_StreamY_IRQHandler symbols from startup_stm32f411vetx.s
 * are overridden here.  Each handler just calls HAL_DMA_IRQHandler() which
 * then fires _hal_cplt_cb / _hal_error_cb → dma_complete_callback /
 * dma_error_callback in the engine core.
 *
 * Note on DMA_HandleTypeDef.Parent
 * ─────────────────────────────────
 * We store the dma_chan_t * in hdma.Parent.  This field is normally used by
 * the STM32 UART/SPI HAL to back-link to the peripheral handle, so this DMA
 * framework must NOT be mixed with HAL_UART_Transmit_DMA() / HAL_SPI_..._DMA().
 * Use either the framework or the HAL peripheral DMA path — not both on the
 * same stream.
 */

#include <drivers/dma/hal/stm32/hal_dma_stm32.h>
#include <def_err.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#ifdef HAL_DMA_MODULE_ENABLED

/* ── Stream / IRQn tables ─────────────────────────────────────────────────── */

static DMA_Stream_TypeDef * const _dma1_stream[8] = {
    DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3,
    DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7,
};

static DMA_Stream_TypeDef * const _dma2_stream[8] = {
    DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3,
    DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7,
};

static const IRQn_Type _dma1_irqn[8] = {
    DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn,
    DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
};

static const IRQn_Type _dma2_irqn[8] = {
    DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
    DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn,
};

/* ── Per-channel hardware contexts ───────────────────────────────────────── */

static drv_hw_dma_chan_ctx_t _dma1_ctx[8];
static drv_hw_dma_chan_ctx_t _dma2_ctx[8];

/* ── HAL → engine callback forwarders ────────────────────────────────────── */

static void _hal_cplt_cb(DMA_HandleTypeDef *hdma)
{
    dma_complete_callback((dma_chan_t *)hdma->Parent);
}

static void _hal_error_cb(DMA_HandleTypeDef *hdma)
{
    dma_error_callback((dma_chan_t *)hdma->Parent);
}

/* ── Helper: direction enum → STM32 HAL constant ─────────────────────────── */

static uint32_t _hal_dir(dma_transfer_direction_t d)
{
    switch (d) {
    case DMA_MEM_TO_DEV: return DMA_MEMORY_TO_PERIPH;
    case DMA_DEV_TO_MEM: return DMA_PERIPH_TO_MEMORY;
    case DMA_MEM_TO_MEM: return DMA_MEMORY_TO_MEMORY;
    default:             return DMA_PERIPH_TO_MEMORY;
    }
}

/* ── Helper: bus-width enum → STM32 alignment constant ───────────────────── */

static uint32_t _hal_align(dma_slave_buswidth_t w)
{
    switch (w) {
    case DMA_SLAVE_BUSWIDTH_2_BYTES: return DMA_PDATAALIGN_HALFWORD;
    case DMA_SLAVE_BUSWIDTH_4_BYTES: return DMA_PDATAALIGN_WORD;
    default:                         return DMA_PDATAALIGN_BYTE;
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * HAL ops implementations
 * ══════════════════════════════════════════════════════════════════════════ */

static int32_t _chan_init(dma_chan_t *chan)
{
    (void)chan;
    /* hw_ctx already wired by hal_dma_stm32_init() before registration */
    return OS_ERR_NONE;
}

static int32_t _chan_deinit(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    if (ctx->hdma.State != HAL_DMA_STATE_RESET)
        HAL_DMA_DeInit(&ctx->hdma);
    HAL_NVIC_DisableIRQ(ctx->irqn);
    return OS_ERR_NONE;
}

static int32_t _slave_config(dma_chan_t *chan, const dma_slave_config_t *cfg)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    DMA_HandleTypeDef     *h   = &ctx->hdma;

    h->Init.Direction           = _hal_dir(cfg->direction);
    h->Init.Channel             = (uint32_t)cfg->request << DMA_SxCR_CHSEL_Pos;
    h->Init.PeriphInc           = (cfg->src_addr_adj == DMA_ADDR_INCREMENT)
                                    ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
    h->Init.MemInc              = (cfg->dst_addr_adj == DMA_ADDR_INCREMENT)
                                    ? DMA_MINC_ENABLE : DMA_MINC_DISABLE;
    h->Init.PeriphDataAlignment = _hal_align(cfg->src_addr_width);
    h->Init.MemDataAlignment    = _hal_align(cfg->dst_addr_width);
    h->Init.Mode                = DMA_NORMAL;
    h->Init.Priority            = DMA_PRIORITY_HIGH;
    h->Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    h->Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    h->Init.MemBurst            = DMA_MBURST_SINGLE;
    h->Init.PeriphBurst         = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(h);
    if (HAL_DMA_Init(h) != HAL_OK)
        return OS_ERR_OP;

    h->XferCpltCallback  = _hal_cplt_cb;
    h->XferErrorCallback = _hal_error_cb;
    h->Parent            = (void *)chan; /* back-link for ISR dispatch */

    HAL_NVIC_SetPriority(ctx->irqn, 5, 0);
    HAL_NVIC_EnableIRQ(ctx->irqn);

    return OS_ERR_NONE;
}

static int32_t _dma_start(dma_chan_t *chan, dma_async_tx_descriptor_t *desc)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    DMA_HandleTypeDef     *h   = &ctx->hdma;

    /* Switch mode if needed */
    uint32_t mode = (desc->mode == DMA_XFER_CYCLIC) ? DMA_CIRCULAR : DMA_NORMAL;
    if (h->Init.Mode != mode)
    {
        h->Init.Mode = mode;
        HAL_DMA_Init(h);
        h->XferCpltCallback  = _hal_cplt_cb;
        h->XferErrorCallback = _hal_error_cb;
        h->Parent            = (void *)chan;
    }

    /* Compute element count from byte length and peripheral width */
    const dma_slave_config_t *cfg   = &chan->slave_cfg;
    uint8_t                   width = (uint8_t)(cfg->src_addr_width
                                       ? cfg->src_addr_width : 1U);
    uint32_t                  count = desc->len / width;

    if (desc->direction == DMA_MEM_TO_MEM)
    {
        /* memcpy: both sides increment with byte width */
        h->Init.PeriphInc           = DMA_PINC_ENABLE;
        h->Init.MemInc              = DMA_MINC_ENABLE;
        h->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        h->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        HAL_DMA_Init(h);
        h->XferCpltCallback  = _hal_cplt_cb;
        h->XferErrorCallback = _hal_error_cb;
        h->Parent            = (void *)chan;
        count = desc->len;
    }

    if (HAL_DMA_Start_IT(h, desc->src, desc->dst, count) != HAL_OK)
        return OS_ERR_OP;

    return OS_ERR_NONE;
}

static int32_t _terminate_all(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    HAL_DMA_Abort(&ctx->hdma);
    return OS_ERR_NONE;
}

static int32_t _pause(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    ctx->hdma.Instance->CR &= ~DMA_SxCR_EN;
    return OS_ERR_NONE;
}

static int32_t _resume(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    ctx->hdma.Instance->CR |= DMA_SxCR_EN;
    return OS_ERR_NONE;
}

static uint32_t _get_residue(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    uint8_t width = (uint8_t)(chan->slave_cfg.src_addr_width
                               ? chan->slave_cfg.src_addr_width : 1U);
    return ctx->hdma.Instance->NDTR * width;
}

/* ── ops table ───────────────────────────────────────────────────────────── */

static const dma_chan_hal_ops_t _stm32_ops = {
    .chan_init     = _chan_init,
    .chan_deinit   = _chan_deinit,
    .slave_config  = _slave_config,
    .start         = _dma_start,
    .terminate_all = _terminate_all,
    .pause         = _pause,
    .resume        = _resume,
    .get_residue   = _get_residue,
};

/* ── Device instances ────────────────────────────────────────────────────── */

dma_device_t hal_dma1_device = {
    .name        = "DMA1",
    .nr_channels = 8,
    .ops         = &_stm32_ops,
};

dma_device_t hal_dma2_device = {
    .name        = "DMA2",
    .nr_channels = 8,
    .ops         = &_stm32_ops,
};

/* ── hal_dma_stm32_init ──────────────────────────────────────────────────── */

void hal_dma_stm32_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    for (uint8_t i = 0; i < 8; i++)
    {
        _dma1_ctx[i].hdma.Instance = _dma1_stream[i];
        _dma1_ctx[i].irqn          = _dma1_irqn[i];
        hal_dma1_device.channels[i].hw_ctx = &_dma1_ctx[i];

        _dma2_ctx[i].hdma.Instance = _dma2_stream[i];
        _dma2_ctx[i].irqn          = _dma2_irqn[i];
        hal_dma2_device.channels[i].hw_ctx = &_dma2_ctx[i];
    }

    dma_register_device(&hal_dma1_device);
    dma_register_device(&hal_dma2_device);
}

/*
 * hal_dma_stm32_irq_dispatch — call from the IRQ dispatch generated file.
 *
 * Add a <dma> entry to your irq_table.xml for each stream in use, e.g.:
 *
 *   <dma controller="1" stream="6" channel="4" priority="5"
 *        dispatch="hal_dma_stm32_irq_dispatch(1, 6)"
 *        dispatch_include="&lt;drivers/dma/hal/stm32/hal_dma_stm32.h&gt;"/>
 *
 * Parameters:
 *   ctrl_idx   — 1 for DMA1, 2 for DMA2
 *   stream_idx — stream number 0–7
 */
void hal_dma_stm32_irq_dispatch(uint8_t ctrl_idx, uint8_t stream_idx)
{
    if (stream_idx >= 8)
        return;

    if (ctrl_idx == 1)
        HAL_DMA_IRQHandler(&_dma1_ctx[stream_idx].hdma);
    else if (ctrl_idx == 2)
        HAL_DMA_IRQHandler(&_dma2_ctx[stream_idx].hdma);
}

#endif /* HAL_DMA_MODULE_ENABLED */
#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
