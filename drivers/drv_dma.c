/*
 * drv_dma.c — DMA engine core
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Responsibilities
 * ────────────────
 *  1. Device registry   : dma_register_device()
 *  2. Channel allocation: dma_request_chan() / dma_release_chan()
 *  3. Descriptor pool   : static per-channel pool of DMA_DESC_POOL_SIZE
 *  4. Pending queue     : dmaengine_submit() / dma_async_issue_pending()
 *  5. ISR dispatch      : dma_complete_callback() / dma_error_callback()
 *
 * All HAL-specific work is delegated through dma_chan_hal_ops_t; this file
 * is vendor-agnostic.
 */

#include <drivers/dma/drv_dma.h>
#include <string.h>

/* ── Global device registry ──────────────────────────────────────────────── */

static LIST_NODE_HEAD(_dev_list);
static uint8_t _dev_count = 0;

/* ── Cookie allocator (monotonic, wraps safely) ──────────────────────────── */

static dma_cookie_t _next_cookie = DMA_MIN_COOKIE;

static dma_cookie_t _cookie_alloc(void)
{
    dma_cookie_t c = _next_cookie++;
    if (_next_cookie < DMA_MIN_COOKIE)
        _next_cookie = DMA_MIN_COOKIE;
    return c;
}

/* ── Per-channel descriptor pool ─────────────────────────────────────────── */

static dma_async_tx_descriptor_t *_desc_alloc(dma_chan_t *chan)
{
    for (int i = 0; i < DMA_DESC_POOL_SIZE; i++)
    {
        if (chan->desc_pool[i].state == DMA_DESC_FREE)
        {
            memset(&chan->desc_pool[i], 0, sizeof(dma_async_tx_descriptor_t));
            chan->desc_pool[i].chan  = chan;
            chan->desc_pool[i].state = DMA_DESC_PREPARED;
            return &chan->desc_pool[i];
        }
    }
    return NULL;
}

static void _desc_free(dma_async_tx_descriptor_t *desc)
{
    desc->state = DMA_DESC_FREE;
}

/* ── dma_register_device ─────────────────────────────────────────────────── */

int32_t dma_register_device(dma_device_t *dev)
{
    if (!dev || !dev->ops || _dev_count >= DMA_MAX_DEVICES)
        return OS_ERR_OP;

    for (uint8_t i = 0; i < dev->nr_channels && i < DMA_MAX_CHANNELS; i++)
    {
        dma_chan_t *ch   = &dev->channels[i];
        ch->device       = dev;
        ch->chan_id      = i;
        ch->state        = DMA_CHAN_IDLE;
        ch->cookie       = 0;
        ch->completed_cookie = 0;
        ch->active_desc  = NULL;
        INIT_LIST_NODE(&ch->pending);

        for (int j = 0; j < DMA_DESC_POOL_SIZE; j++)
            ch->desc_pool[j].state = DMA_DESC_FREE;

        if (dev->ops->chan_init)
            dev->ops->chan_init(ch);
    }

    list_add_tail(&dev->list, &_dev_list);
    _dev_count++;
    return OS_ERR_NONE;
}

/* ── dma_request_chan ────────────────────────────────────────────────────── */

dma_chan_t *dma_request_chan(const char *dev_name, uint8_t chan_id)
{
    if (!dev_name)
        return NULL;

    dma_device_t *dev;
    list_for_each_entry(dev, &_dev_list, list)
    {
        if (strcmp(dev->name, dev_name) != 0)
            continue;
        if (chan_id >= dev->nr_channels)
            return NULL;

        dma_chan_t *ch = &dev->channels[chan_id];
        if (ch->state == DMA_CHAN_IDLE)
        {
            ch->state = DMA_CHAN_ALLOCATED;
            return ch;
        }
    }
    return NULL;
}

/* ── dma_release_chan ────────────────────────────────────────────────────── */

void dma_release_chan(dma_chan_t *chan)
{
    if (!chan)
        return;
    dmaengine_terminate_all(chan);
    chan->state = DMA_CHAN_IDLE;
}

/* ── dmaengine_slave_config ──────────────────────────────────────────────── */

int32_t dmaengine_slave_config(dma_chan_t *chan, const dma_slave_config_t *cfg)
{
    if (!chan || !cfg || chan->state == DMA_CHAN_IDLE)
        return OS_ERR_OP;

    chan->slave_cfg = *cfg;

    if (chan->device->ops->slave_config)
        return chan->device->ops->slave_config(chan, cfg);

    return OS_ERR_NONE;
}

/* ── Descriptor preparation ──────────────────────────────────────────────── */

dma_async_tx_descriptor_t *dmaengine_prep_slave_single(
    dma_chan_t *chan, uint32_t buf, uint32_t len,
    dma_transfer_direction_t dir)
{
    if (!chan || chan->state == DMA_CHAN_IDLE || len == 0)
        return NULL;

    dma_async_tx_descriptor_t *desc = _desc_alloc(chan);
    if (!desc)
        return NULL;

    desc->direction = dir;
    desc->mode      = DMA_XFER_NORMAL;
    desc->len       = len;

    if (dir == DMA_MEM_TO_DEV)
    {
        desc->src = buf;
        desc->dst = chan->slave_cfg.dst_addr;
    }
    else /* DEV_TO_MEM */
    {
        desc->src = chan->slave_cfg.src_addr;
        desc->dst = buf;
    }
    return desc;
}

dma_async_tx_descriptor_t *dmaengine_prep_dma_memcpy(
    dma_chan_t *chan, uint32_t dst, uint32_t src, uint32_t len)
{
    if (!chan || chan->state == DMA_CHAN_IDLE || len == 0)
        return NULL;

    dma_async_tx_descriptor_t *desc = _desc_alloc(chan);
    if (!desc)
        return NULL;

    desc->direction = DMA_MEM_TO_MEM;
    desc->mode      = DMA_XFER_NORMAL;
    desc->src       = src;
    desc->dst       = dst;
    desc->len       = len;
    return desc;
}

dma_async_tx_descriptor_t *dmaengine_prep_dma_cyclic(
    dma_chan_t *chan, uint32_t buf_addr, uint32_t buf_len,
    uint32_t period_len, dma_transfer_direction_t dir)
{
    if (!chan || chan->state == DMA_CHAN_IDLE || buf_len == 0 || period_len == 0)
        return NULL;

    dma_async_tx_descriptor_t *desc = _desc_alloc(chan);
    if (!desc)
        return NULL;

    desc->direction  = dir;
    desc->mode       = DMA_XFER_CYCLIC;
    desc->len        = buf_len;
    desc->period_len = period_len;
    desc->src = (dir == DMA_DEV_TO_MEM) ? chan->slave_cfg.src_addr : buf_addr;
    desc->dst = (dir == DMA_MEM_TO_DEV) ? chan->slave_cfg.dst_addr : buf_addr;
    return desc;
}

/* ── dmaengine_submit ────────────────────────────────────────────────────── */

dma_cookie_t dmaengine_submit(dma_async_tx_descriptor_t *desc)
{
    if (!desc || !desc->chan)
        return (dma_cookie_t)(-OS_ERR_OP);

    desc->cookie = _cookie_alloc();
    desc->state  = DMA_DESC_SUBMITTED;
    list_add_tail(&desc->node, &desc->chan->pending);
    desc->chan->cookie = desc->cookie;
    return desc->cookie;
}

/* ── dma_async_issue_pending ─────────────────────────────────────────────── */

void dma_async_issue_pending(dma_chan_t *chan)
{
    if (!chan || chan->state == DMA_CHAN_IDLE)
        return;
    if (chan->active_desc)
        return; /* already running — will auto-chain on complete */

    if (list_empty(&chan->pending))
        return;

    dma_async_tx_descriptor_t *desc = list_first_entry(
        &chan->pending, dma_async_tx_descriptor_t, node);

    list_delete(&desc->node);
    desc->state      = DMA_DESC_IN_PROGRESS;
    chan->active_desc = desc;
    chan->state       = DMA_CHAN_BUSY;

    if (chan->device->ops->start)
        chan->device->ops->start(chan, desc);
}

/* ── Control ops ─────────────────────────────────────────────────────────── */

int32_t dmaengine_terminate_all(dma_chan_t *chan)
{
    if (!chan)
        return OS_ERR_OP;

    int32_t ret = OS_ERR_NONE;
    if (chan->device->ops->terminate_all)
        ret = chan->device->ops->terminate_all(chan);

    while (!list_empty(&chan->pending))
    {
        dma_async_tx_descriptor_t *desc = list_first_entry(
            &chan->pending, dma_async_tx_descriptor_t, node);
        list_delete(&desc->node);
        _desc_free(desc);
    }

    if (chan->active_desc)
    {
        _desc_free(chan->active_desc);
        chan->active_desc = NULL;
    }

    if (chan->state == DMA_CHAN_BUSY)
        chan->state = DMA_CHAN_ALLOCATED;

    return ret;
}

int32_t dmaengine_pause(dma_chan_t *chan)
{
    if (!chan || !chan->device->ops->pause)
        return OS_ERR_OP;
    return chan->device->ops->pause(chan);
}

int32_t dmaengine_resume(dma_chan_t *chan)
{
    if (!chan || !chan->device->ops->resume)
        return OS_ERR_OP;
    return chan->device->ops->resume(chan);
}

uint32_t dmaengine_get_residue(dma_chan_t *chan)
{
    if (!chan || !chan->device->ops->get_residue)
        return 0;
    return chan->device->ops->get_residue(chan);
}

/* ── ISR dispatch — called by HAL backend from interrupt context ─────────── */

void dma_complete_callback(dma_chan_t *chan)
{
    if (!chan)
        return;

    dma_async_tx_descriptor_t *desc = chan->active_desc;
    if (!desc)
        return;

    chan->completed_cookie = desc->cookie;

    if (desc->callback)
        desc->callback(desc->callback_param);

    /* Cyclic: keep descriptor active; HAL re-arms automatically */
    if (desc->mode == DMA_XFER_CYCLIC)
    {
        desc->state = DMA_DESC_IN_PROGRESS;
        return;
    }

    desc->state      = DMA_DESC_COMPLETE;
    _desc_free(desc);
    chan->active_desc = NULL;
    chan->state       = DMA_CHAN_ALLOCATED;

    /* Auto-start next queued descriptor */
    dma_async_issue_pending(chan);
}

void dma_error_callback(dma_chan_t *chan)
{
    if (!chan)
        return;

    dma_async_tx_descriptor_t *desc = chan->active_desc;
    if (!desc)
        return;

    if (desc->error_callback)
        desc->error_callback(desc->callback_param);

    desc->state      = DMA_DESC_ERROR;
    _desc_free(desc);
    chan->active_desc = NULL;
    chan->state       = DMA_CHAN_ALLOCATED;
}
