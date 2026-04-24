/*
File:        drv_dma.h
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Module:      drivers/dma
Info:        DMA engine API, descriptors, channel and device abstraction              
Dependency:  list.h

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
 * @file drv_dma.h
 * @brief Linux-style DMA engine abstraction
 *
 * Provides:
 *  - DMA controller abstraction (dma_device)
 *  - Channel abstraction (dma_chan)
 *  - Transfer descriptors
 *  - Submission and scheduling API
 *
 * Designed to mirror Linux dmaengine framework for portability and familiarity.
 */

#ifndef INCLUDE_DRIVERS_DMA_DRV_DMA_H_
#define INCLUDE_DRIVERS_DMA_DRV_DMA_H_

#include <def_attributes.h>
#include <def_compiler.h>
#include <def_std.h>
#include <def_err.h>
#include <os/kernel.h>
#include <mm/list.h>

/* ── Limits ───────────────────────────────────────────────────────────── */

#define DMA_MAX_DEVICES     2
#define DMA_MAX_CHANNELS    8
#define DMA_DESC_POOL_SIZE  4
#define DMA_MIN_COOKIE      1

/* ── Cookie type ──────────────────────────────────────────────────────── */

typedef int32_t dma_cookie_t;

/* ── Transfer direction ───────────────────────────────────────────────── */

typedef enum {
    DMA_MEM_TO_MEM = 0,
    DMA_MEM_TO_DEV,
    DMA_DEV_TO_MEM,
    DMA_DEV_TO_DEV,
} dma_transfer_direction_t;

/* ── Transfer mode ────────────────────────────────────────────────────── */

typedef enum {
    DMA_XFER_SINGLE = 0,
    DMA_XFER_CYCLIC,
} dma_xfer_mode_t;

/* ── Descriptor state ─────────────────────────────────────────────────── */

typedef enum {
    DMA_DESC_FREE = 0,
    DMA_DESC_PREPARED,
    DMA_DESC_IN_PROGRESS,
    DMA_DESC_COMPLETE,
    DMA_DESC_ERROR,
} dma_desc_state_t;

/* ── Channel state ────────────────────────────────────────────────────── */

typedef enum {
    DMA_CHAN_IDLE = 0,
    DMA_CHAN_ALLOCATED,
    DMA_CHAN_BUSY,
} dma_chan_state_t;

/* ── Forward declarations ─────────────────────────────────────────────── */

typedef struct dma_chan   dma_chan_t;
typedef struct dma_device dma_device_t;

/* ── Descriptor ───────────────────────────────────────────────────────── */

typedef struct dma_async_tx_descriptor {
    dma_chan_t       *chan;
    dma_cookie_t      cookie;
    dma_desc_state_t  state;
    dma_xfer_mode_t   mode;

    uint32_t src;
    uint32_t dst;
    uint32_t len;

    void (*callback)      (void *param);
    void (*error_callback)(void *param);
    void *callback_param;

    struct list_node node;
} dma_async_tx_descriptor_t;

/* ── Slave bus width ──────────────────────────────────────────────────── */

typedef enum {
    DMA_SLAVE_BUSWIDTH_1_BYTE  = 0,
    DMA_SLAVE_BUSWIDTH_2_BYTES,
    DMA_SLAVE_BUSWIDTH_4_BYTES,
} dma_slave_buswidth_t;

/* ── Address adjustment ───────────────────────────────────────────────── */

typedef enum {
    DMA_ADDR_NO_INCREMENT = 0,
    DMA_ADDR_INCREMENT,
} dma_addr_adj_t;

/* ── Slave config ─────────────────────────────────────────────────────── */

typedef struct {
    dma_transfer_direction_t direction;
    uint32_t                 request;       /* peripheral DMA channel mux */
    dma_addr_adj_t           src_addr_adj;
    dma_addr_adj_t           dst_addr_adj;
    dma_slave_buswidth_t     src_addr_width;
    dma_slave_buswidth_t     dst_addr_width;
} dma_slave_config_t;

/* ── HAL ops table ────────────────────────────────────────────────────── */

typedef struct dma_chan_hal_ops {
    int32_t  (*chan_init)     (dma_chan_t *chan);
    int32_t  (*chan_deinit)   (dma_chan_t *chan);
    int32_t  (*slave_config)  (dma_chan_t *chan, const dma_slave_config_t *cfg);
    int32_t  (*start)         (dma_chan_t *chan, dma_async_tx_descriptor_t *desc);
    int32_t  (*terminate_all) (dma_chan_t *chan);
    int32_t  (*pause)         (dma_chan_t *chan);
    int32_t  (*resume)        (dma_chan_t *chan);
    uint32_t (*get_residue)   (dma_chan_t *chan);
} dma_chan_hal_ops_t;

/* ── Channel struct ───────────────────────────────────────────────────── */

struct dma_chan {
    dma_device_t              *device;
    uint8_t                    chan_id;
    dma_chan_state_t           state;
    dma_cookie_t               cookie;
    dma_cookie_t               completed_cookie;
    dma_async_tx_descriptor_t *active_desc;
    struct list_node           pending;
    dma_async_tx_descriptor_t  desc_pool[DMA_DESC_POOL_SIZE];
    void                      *hw_ctx;  /* drv_hw_dma_chan_ctx_t * for STM32 */
};

/* ── Device struct ────────────────────────────────────────────────────── */

struct dma_device {
    const char               *name;
    uint8_t                   nr_channels;
    dma_chan_t                 channels[DMA_MAX_CHANNELS];
    struct list_node           list;
    const dma_chan_hal_ops_t  *ops;
};

/* ── API ─────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

int32_t    dma_register_device   (dma_device_t *dev);
dma_chan_t *dma_request_chan     (const char *dev_name, uint8_t chan_id);
void        dma_release_chan     (dma_chan_t *chan);
void        dmaengine_terminate_all(dma_chan_t *chan);

dma_cookie_t dmaengine_submit       (dma_async_tx_descriptor_t *desc);
void         dma_async_issue_pending(dma_chan_t *chan);

void dma_complete_callback(dma_chan_t *chan);
void dma_error_callback   (dma_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DMA_DRV_DMA_H_ */