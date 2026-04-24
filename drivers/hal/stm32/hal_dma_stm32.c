/**
 * @file    hal_dma_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 DMA engine HAL backend implementation
 *
 * @details
 * This file implements the DMA hardware abstraction layer (HAL) backend
 * for STM32F4 devices. It maps the generic dma_chan_hal_ops_t interface
 * onto STM32 HAL DMA primitives.
 *
 * Each DMA channel is associated with a drv_hw_dma_chan_ctx_t context,
 * which wraps:
 * - DMA_HandleTypeDef (HAL driver handle)
 * - IRQ number for the DMA stream
 *
 * The implementation supports:
 * - Slave DMA configuration
 * - Memory-to-memory and peripheral transfers
 * - Interrupt-driven completion and error handling
 * - Channel lifecycle management (init, start, pause, resume, terminate)
 *
 * -----------------------------------------------------------------------------
 * @section dma_irq ISR Routing
 *
 * The weak DMAx_StreamY_IRQHandler symbols are overridden externally.
 * Each handler calls HAL_DMA_IRQHandler(), which triggers:
 *
 * @code
 * HAL_DMA_IRQHandler()
 *     → XferCpltCallback  → _hal_cplt_cb()  → dma_complete_callback()
 *     → XferErrorCallback → _hal_error_cb() → dma_error_callback()
 * @endcode
 *
 * -----------------------------------------------------------------------------
 * @section dma_note Parent Pointer Usage
 *
 * The DMA_HandleTypeDef.Parent field is used to store dma_chan_t*.
 * This enables callback dispatch back into the DMA engine.
 *
 * @warning
 * This conflicts with STM32 HAL peripheral DMA usage
 * (e.g., HAL_UART_Transmit_DMA). Do NOT mix both on the same stream.
 *
 * -----------------------------------------------------------------------------
 *
 * @dependencies
 * hal_dma_stm32.h, def_err.h
 *
 * @note
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
 * GNU General Public License v3 or later
 */

#include <drivers/hal/stm32/hal_dma_stm32.h>
#include <def_err.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)
#ifdef HAL_DMA_MODULE_ENABLED

/* ────────────────────────────────────────────────────────────────────────── */
/* Stream / IRQ tables                                                       */
/* ────────────────────────────────────────────────────────────────────────── */

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

/* ────────────────────────────────────────────────────────────────────────── */
/* Hardware contexts                                                         */
/* ────────────────────────────────────────────────────────────────────────── */
__SECTION_OS_DATA __USED
static drv_hw_dma_chan_ctx_t _dma1_ctx[8];

__SECTION_OS_DATA __USED
static drv_hw_dma_chan_ctx_t _dma2_ctx[8];

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL → DMA engine callbacks                                                */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief DMA transfer complete callback (HAL → engine)
 */
__SECTION_OS __USED
static void _hal_cplt_cb(DMA_HandleTypeDef *hdma)
{
    dma_complete_callback((dma_chan_t *)hdma->Parent);
}

/**
 * @brief DMA error callback (HAL → engine)
 */
__SECTION_OS __USED
static void _hal_error_cb(DMA_HandleTypeDef *hdma)
{
    dma_error_callback((dma_chan_t *)hdma->Parent);
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Helper functions                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Convert generic DMA direction to HAL constant
 */
__SECTION_OS __USED
static uint32_t _hal_dir(dma_transfer_direction_t d)
{
    switch (d) {
    case DMA_MEM_TO_DEV: return DMA_MEMORY_TO_PERIPH;
    case DMA_DEV_TO_MEM: return DMA_PERIPH_TO_MEMORY;
    case DMA_MEM_TO_MEM: return DMA_MEMORY_TO_MEMORY;
    default:             return DMA_PERIPH_TO_MEMORY;
    }
}

/**
 * @brief Convert bus width to HAL alignment
 */
__SECTION_OS __USED
static uint32_t _hal_align(dma_slave_buswidth_t w)
{
    switch (w) {
    case DMA_SLAVE_BUSWIDTH_2_BYTES: return DMA_PDATAALIGN_HALFWORD;
    case DMA_SLAVE_BUSWIDTH_4_BYTES: return DMA_PDATAALIGN_WORD;
    default:                         return DMA_PDATAALIGN_BYTE;
    }
}

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL ops implementations                                                   */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize DMA channel
 */
__SECTION_OS __USED
static int32_t _chan_init(dma_chan_t *chan)
{
    (void)chan;
    return OS_ERR_NONE;
}

/**
 * @brief Deinitialize DMA channel
 */
__SECTION_OS __USED
static int32_t _chan_deinit(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;

    if (ctx->hdma.State != HAL_DMA_STATE_RESET)
        HAL_DMA_DeInit(&ctx->hdma);

    HAL_NVIC_DisableIRQ(ctx->irqn);
    return OS_ERR_NONE;
}

/**
 * @brief Configure DMA slave parameters
 */
__SECTION_OS __USED
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

    HAL_DMA_DeInit(h);
    if (HAL_DMA_Init(h) != HAL_OK)
        return OS_ERR_OP;

    h->XferCpltCallback  = _hal_cplt_cb;
    h->XferErrorCallback = _hal_error_cb;
    h->Parent            = (void *)chan;

    HAL_NVIC_SetPriority(ctx->irqn, 5, 0);
    HAL_NVIC_EnableIRQ(ctx->irqn);

    return OS_ERR_NONE;
}

/**
 * @brief Start DMA transfer
 */
__SECTION_OS __USED
static int32_t _dma_start(dma_chan_t *chan, dma_async_tx_descriptor_t *desc)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    DMA_HandleTypeDef     *h   = &ctx->hdma;

    uint32_t count = desc->len;

    if (HAL_DMA_Start_IT(h, desc->src, desc->dst, count) != HAL_OK)
        return OS_ERR_OP;

    return OS_ERR_NONE;
}

/**
 * @brief Terminate all DMA transfers
 */
__SECTION_OS __USED
static int32_t _terminate_all(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    HAL_DMA_Abort(&ctx->hdma);
    return OS_ERR_NONE;
}

/**
 * @brief Pause DMA channel
 */
__SECTION_OS __USED
static int32_t _pause(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    ctx->hdma.Instance->CR &= ~DMA_SxCR_EN;
    return OS_ERR_NONE;
}

/**
 * @brief Resume DMA channel
 */
__SECTION_OS __USED
static int32_t _resume(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    ctx->hdma.Instance->CR |= DMA_SxCR_EN;
    return OS_ERR_NONE;
}

/**
 * @brief Get remaining bytes in DMA transfer
 */
__SECTION_OS __USED
static uint32_t _get_residue(dma_chan_t *chan)
{
    drv_hw_dma_chan_ctx_t *ctx = (drv_hw_dma_chan_ctx_t *)chan->hw_ctx;
    return ctx->hdma.Instance->NDTR;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Ops table                                                                 */
/* ────────────────────────────────────────────────────────────────────────── */

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

/* ────────────────────────────────────────────────────────────────────────── */
/* Device instances                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

__SECTION_OS_DATA __USED
dma_device_t hal_dma1_device = {
    .name        = "DMA1",
    .nr_channels = 8,
    .ops         = &_stm32_ops,
};

__SECTION_OS_DATA __USED
dma_device_t hal_dma2_device = {
    .name        = "DMA2",
    .nr_channels = 8,
    .ops         = &_stm32_ops,
};

/* ────────────────────────────────────────────────────────────────────────── */
/* Initialization                                                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize STM32 DMA subsystem
 */
__SECTION_OS __USED
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

/**
 * @brief DMA IRQ dispatcher
 *
 * @param ctrl_idx   DMA controller index (1 or 2)
 * @param stream_idx DMA stream index (0–7)
 */
__SECTION_OS __USED
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