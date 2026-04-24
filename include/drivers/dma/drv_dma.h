/*
 * drv_dma.h — Linux-style DMA engine API
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Architecture mirrors Linux dmaengine:
 *
 *   dma_device    — registered DMA controller (DMA1 / DMA2)
 *   dma_chan      — one stream/channel inside a controller
 *   dma_async_tx_descriptor — describes a single transfer
 *   dma_slave_config        — peripheral-side parameters
 *
 * Typical usage (peripheral slave transfer):
 *
 *   // 1. Request a channel once at init
 *   dma_chan_t *ch = dma_request_chan("DMA1", 3);
 *
 *   // 2. Configure slave (peripheral address, direction, widths)
 *   dma_slave_config_t cfg = {
 *       .direction      = DMA_MEM_TO_DEV,
 *       .dst_addr       = (uint32_t)&USART2->DR,
 *       .src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
 *       .dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
 *       .src_addr_adj   = DMA_ADDR_INCREMENT,
 *       .dst_addr_adj   = DMA_ADDR_FIXED,
 *       .request        = 4,   // DMA channel 4 → USART2_TX on DMA1 Stream6
 *   };
 *   dmaengine_slave_config(ch, &cfg);
 *
 *   // 3. Prepare descriptor
 *   dma_async_tx_descriptor_t *desc =
 *       dmaengine_prep_slave_single(ch, (uint32_t)buf, len, DMA_MEM_TO_DEV);
 *   desc->callback       = my_tx_done;
 *   desc->callback_param = my_ctx;
 *
 *   // 4. Submit and start
 *   dmaengine_submit(desc);
 *   dma_async_issue_pending(ch);
 *
 *   // 5. Release when done
 *   dma_release_chan(ch);
 */

#ifndef INCLUDE_DRIVERS_DMA_DRV_DMA_H_
#define INCLUDE_DRIVERS_DMA_DRV_DMA_H_

#include <def_std.h>
#include <def_err.h>
#include <mm/list.h>

/* ── Limits ───────────────────────────────────────────────────────────────── */

#define DMA_MAX_DEVICES     2   /* DMA1 + DMA2                        */
#define DMA_MAX_CHANNELS    8   /* streams per controller (STM32F4)   */
#define DMA_DESC_POOL_SIZE  4   /* descriptor pool depth per channel  */

/* ── Transfer direction ───────────────────────────────────────────────────── */

typedef enum {
    DMA_MEM_TO_MEM = 0,
    DMA_MEM_TO_DEV,
    DMA_DEV_TO_MEM,
    DMA_DEV_TO_DEV,
} dma_transfer_direction_t;

/* ── Bus width for peripheral accesses ───────────────────────────────────── */

typedef enum {
    DMA_SLAVE_BUSWIDTH_UNDEFINED = 0,
    DMA_SLAVE_BUSWIDTH_1_BYTE    = 1,
    DMA_SLAVE_BUSWIDTH_2_BYTES   = 2,
    DMA_SLAVE_BUSWIDTH_4_BYTES   = 4,
} dma_slave_buswidth_t;

/* ── Address advancement after each beat ─────────────────────────────────── */

typedef enum {
    DMA_ADDR_INCREMENT = 0,  /* advance pointer — use for memory buffers    */
    DMA_ADDR_FIXED,          /* keep pointer constant — use for peripheral DR */
} dma_addr_adj_t;

/* ── Transfer mode ────────────────────────────────────────────────────────── */

typedef enum {
    DMA_XFER_NORMAL = 0,  /* single-shot; stream stops after len bytes */
    DMA_XFER_CYCLIC,      /* auto-reload; callback fires every period_len */
} dma_xfer_mode_t;

/* ── Descriptor state ─────────────────────────────────────────────────────── */

typedef enum {
    DMA_DESC_FREE = 0,
    DMA_DESC_PREPARED,
    DMA_DESC_SUBMITTED,
    DMA_DESC_IN_PROGRESS,
    DMA_DESC_COMPLETE,
    DMA_DESC_ERROR,
} dma_desc_state_t;

/* ── Channel state ────────────────────────────────────────────────────────── */

typedef enum {
    DMA_CHAN_IDLE = 0,   /* not allocated      */
    DMA_CHAN_ALLOCATED,  /* owned, idle        */
    DMA_CHAN_BUSY,       /* transfer in flight */
} dma_chan_state_t;

/* ── Cookie: transfer ID returned by dmaengine_submit ────────────────────── */

typedef int32_t dma_cookie_t;
#define DMA_MIN_COOKIE  1

/* ── Forward declarations ─────────────────────────────────────────────────── */

typedef struct dma_chan    dma_chan_t;
typedef struct dma_device  dma_device_t;

/* ── Slave configuration ──────────────────────────────────────────────────── */
/*
 * For MEM_TO_DEV (e.g. UART TX):
 *   src_addr_adj = DMA_ADDR_INCREMENT  (walk memory buffer)
 *   dst_addr_adj = DMA_ADDR_FIXED      (peripheral DR stays put)
 *   dst_addr     = &USARTx->DR
 *
 * For DEV_TO_MEM (e.g. UART RX):
 *   src_addr_adj = DMA_ADDR_FIXED
 *   dst_addr_adj = DMA_ADDR_INCREMENT
 *   src_addr     = &USARTx->DR
 *
 * request: HW request line / channel mux index (0–7 for STM32F4)
 */
typedef struct {
    dma_transfer_direction_t  direction;
    uint32_t                  src_addr;       /* peripheral register address */
    uint32_t                  dst_addr;       /* peripheral register address */
    dma_slave_buswidth_t      src_addr_width;
    dma_slave_buswidth_t      dst_addr_width;
    uint32_t                  src_maxburst;   /* beats per burst (1 = no burst) */
    uint32_t                  dst_maxburst;
    dma_addr_adj_t            src_addr_adj;
    dma_addr_adj_t            dst_addr_adj;
    uint8_t                   request;        /* channel mux (DMA_CHANNEL_x) */
} dma_slave_config_t;

/* ── Async transfer descriptor ───────────────────────────────────────────── */

typedef struct dma_async_tx_descriptor {
    dma_chan_t               *chan;
    dma_cookie_t              cookie;
    dma_desc_state_t          state;

    uint32_t                  src;          /* source address               */
    uint32_t                  dst;          /* destination address          */
    uint32_t                  len;          /* total transfer length, bytes */
    uint32_t                  period_len;   /* cyclic period, bytes         */
    dma_transfer_direction_t  direction;
    dma_xfer_mode_t           mode;

    void  (*callback)      (void *param);   /* called on completion         */
    void  (*error_callback)(void *param);   /* called on DMA error          */
    void  *callback_param;

    struct list_node          node;         /* pending-queue linkage        */
} dma_async_tx_descriptor_t;

/* ── HAL operations vtable ───────────────────────────────────────────────── */

typedef struct {
    int32_t  (*chan_init)    (dma_chan_t *chan);
    int32_t  (*chan_deinit)  (dma_chan_t *chan);
    int32_t  (*slave_config) (dma_chan_t *chan, const dma_slave_config_t *cfg);
    int32_t  (*start)        (dma_chan_t *chan, dma_async_tx_descriptor_t *desc);
    int32_t  (*terminate_all)(dma_chan_t *chan);
    int32_t  (*pause)        (dma_chan_t *chan);
    int32_t  (*resume)       (dma_chan_t *chan);
    uint32_t (*get_residue)  (dma_chan_t *chan);  /* bytes remaining in NDTR */
} dma_chan_hal_ops_t;

/* ── DMA channel ─────────────────────────────────────────────────────────── */

struct dma_chan {
    dma_device_t              *device;
    uint8_t                    chan_id;          /* stream index 0..7           */
    dma_chan_state_t            state;
    dma_cookie_t               cookie;           /* last submitted              */
    dma_cookie_t               completed_cookie; /* last completed              */

    dma_slave_config_t         slave_cfg;

    struct list_node           pending;          /* queue of submitted descs    */
    dma_async_tx_descriptor_t *active_desc;      /* currently running           */

    dma_async_tx_descriptor_t  desc_pool[DMA_DESC_POOL_SIZE]; /* static pool   */

    void                      *hw_ctx;           /* vendor HAL context (opaque) */
};

/* ── DMA device (controller) ─────────────────────────────────────────────── */

struct dma_device {
    const char              *name;               /* "DMA1" or "DMA2"            */
    uint8_t                  nr_channels;        /* valid entries in channels[] */
    const dma_chan_hal_ops_t *ops;
    dma_chan_t               channels[DMA_MAX_CHANNELS];
    struct list_node         list;               /* global device-list linkage  */
};

/* ══════════════════════════════════════════════════════════════════════════
 * Public DMA engine API
 * ══════════════════════════════════════════════════════════════════════════ */

#ifdef __cplusplus
extern "C" {
#endif

/* ── Controller registration ─────────────────────────────────────────────── */
int32_t    dma_register_device(dma_device_t *dev);

/* ── Channel allocation ──────────────────────────────────────────────────── */
dma_chan_t *dma_request_chan  (const char *dev_name, uint8_t chan_id);
void        dma_release_chan  (dma_chan_t *chan);

/* ── Slave config ────────────────────────────────────────────────────────── */
int32_t    dmaengine_slave_config(dma_chan_t *chan, const dma_slave_config_t *cfg);

/* ── Descriptor preparation ──────────────────────────────────────────────── */
dma_async_tx_descriptor_t *dmaengine_prep_slave_single(
    dma_chan_t *chan, uint32_t buf, uint32_t len,
    dma_transfer_direction_t dir);

dma_async_tx_descriptor_t *dmaengine_prep_dma_memcpy(
    dma_chan_t *chan, uint32_t dst, uint32_t src, uint32_t len);

dma_async_tx_descriptor_t *dmaengine_prep_dma_cyclic(
    dma_chan_t *chan, uint32_t buf_addr, uint32_t buf_len,
    uint32_t period_len, dma_transfer_direction_t dir);

/* ── Submission and execution ────────────────────────────────────────────── */
dma_cookie_t dmaengine_submit        (dma_async_tx_descriptor_t *desc);
void         dma_async_issue_pending (dma_chan_t *chan);

/* ── Control ─────────────────────────────────────────────────────────────── */
int32_t  dmaengine_terminate_all (dma_chan_t *chan);
int32_t  dmaengine_pause         (dma_chan_t *chan);
int32_t  dmaengine_resume        (dma_chan_t *chan);
uint32_t dmaengine_get_residue   (dma_chan_t *chan);

/* ── ISR callbacks — invoked by the HAL backend from interrupt context ────── */
void dma_complete_callback(dma_chan_t *chan);
void dma_error_callback   (dma_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_DMA_DRV_DMA_H_ */
