/*
File:        drv_dma.c
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Module:      drivers/dma
Info:        DMA engine core implementation (device registry, channel management,
             descriptor handling, scheduling, and ISR dispatch)              
Dependency:  drv_dma.h, list.h

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation, either version 
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>. 
*/

/**
 * @file drv_dma.c
 * @brief Generic DMA engine core (vendor-agnostic)
 *
 * Responsibilities:
 *  - Device registry
 *  - Channel allocation and lifecycle management
 *  - Descriptor pool management
 *  - Transfer submission and scheduling
 *  - ISR completion/error dispatch
 *
 * All hardware-specific operations are delegated via dma_chan_hal_ops_t.
 */
#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <drivers/drv_dma.h>
#include <os/kernel.h>
#include <mm/list.h>


/* ── Global device registry ──────────────────────────────────────────────── */

/** @brief Global list of registered DMA devices */
__SECTION_OS_DATA __USED
static LIST_NODE_HEAD(_dev_list);

/** @brief Number of registered DMA devices */
__SECTION_OS_DATA __USED
static uint8_t _dev_count = 0;

/* ── Cookie allocator ───────────────────────────────────────────────────── */

/** @brief Next cookie value */
__SECTION_OS_DATA __USED
static dma_cookie_t _next_cookie = DMA_MIN_COOKIE;

/**
 * @brief Allocate a unique DMA cookie
 * @return Cookie identifier
 */
__SECTION_OS __USED
static dma_cookie_t _cookie_alloc(void)
{
    dma_cookie_t c = _next_cookie++;
    if (_next_cookie < DMA_MIN_COOKIE)
        _next_cookie = DMA_MIN_COOKIE;
    return c;
}

/* ── Descriptor pool ────────────────────────────────────────────────────── */

/**
 * @brief Allocate descriptor from channel pool
 */
__SECTION_OS __USED
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

/**
 * @brief Free descriptor back to pool
 */
__SECTION_OS __USED
static void _desc_free(dma_async_tx_descriptor_t *desc)
{
    desc->state = DMA_DESC_FREE;
}

/* ── Device registration ────────────────────────────────────────────────── */

/**
 * @brief Register a DMA controller
 *
 * @param dev Pointer to DMA device structure
 * @return OS_ERR_NONE on success
 */
__SECTION_OS __USED
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

/* ── Channel allocation ─────────────────────────────────────────────────── */

/**
 * @brief Request a DMA channel by name and ID
 */
__SECTION_OS __USED
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

/**
 * @brief Release DMA channel
 */
__SECTION_OS __USED
void dma_release_chan(dma_chan_t *chan)
{
    if (!chan)
        return;

    dmaengine_terminate_all(chan);
    chan->state = DMA_CHAN_IDLE;
}

__SECTION_OS __USED
void dmaengine_terminate_all(dma_chan_t *chan)
{
    if (!chan || !chan->device || !chan->device->ops->terminate_all)
        return;
    chan->device->ops->terminate_all(chan);
    chan->active_desc = NULL;
}

__SECTION_OS __USED
void dma_async_issue_pending(dma_chan_t *chan)
{
    if (!chan || !chan->device || !chan->device->ops->start)
        return;

    if (list_empty(&chan->pending))
        return;

    dma_async_tx_descriptor_t *desc =
        list_first_entry(&chan->pending, dma_async_tx_descriptor_t, node);

    list_delete(&desc->node);
    chan->active_desc = desc;
    desc->state       = DMA_DESC_IN_PROGRESS;
    chan->state       = DMA_CHAN_BUSY;
    chan->device->ops->start(chan, desc);
}

/* ── ISR callbacks ─────────────────────────────────────────────────────── */

/**
 * @brief DMA transfer complete callback (called from ISR)
 */
__SECTION_OS __USED
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

    if (desc->mode == DMA_XFER_CYCLIC)
    {
        desc->state = DMA_DESC_IN_PROGRESS;
        return;
    }

    desc->state = DMA_DESC_COMPLETE;
    _desc_free(desc);

    chan->active_desc = NULL;
    chan->state = DMA_CHAN_ALLOCATED;

    dma_async_issue_pending(chan);
}

/**
 * @brief DMA error callback (called from ISR)
 */
__SECTION_OS __USED
void dma_error_callback(dma_chan_t *chan)
{
    if (!chan)
        return;

    dma_async_tx_descriptor_t *desc = chan->active_desc;
    if (!desc)
        return;

    if (desc->error_callback)
        desc->error_callback(desc->callback_param);

    desc->state = DMA_DESC_ERROR;
    _desc_free(desc);

    chan->active_desc = NULL;
    chan->state = DMA_CHAN_ALLOCATED;
}