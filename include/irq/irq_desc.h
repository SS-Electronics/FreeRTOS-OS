/*
 * irq_desc.h — Linux-style IRQ descriptor chain for FreeRTOS/bare-metal
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Adapts the Linux IRQ descriptor model to a single-core bare-metal system:
 *   - SMP / percpu / cpumask features removed
 *   - PM sleep / proc-fs / sparse-IRQ fields removed
 *   - raw_spinlock_t → not needed (single-core, ISR entry is atomic)
 *   - wait_queue_head_t → FreeRTOS tasks use direct task-notification instead
 *   - All struct and typedef names are kept faithful to Linux for readability
 *
 * Call chain (from hardware to driver):
 *
 *   CPU vector (startup)
 *     → irq_periph_dispatch_generated.c (USART1_IRQHandler etc.)
 *        → hal_xxx_stm32_irq_handler()
 *           → drv_irq_dispatch_from_isr(irq_id, data, pxHPT)
 *              → __do_IRQ_from_isr(irq_id, data, pxHPT)
 *                 → irq_to_desc(irq_id)           — O(1) array lookup
 *                 → desc->handle_irq(desc, …)      — flow handler
 *                    → handle_irq_event(desc, …)  — walk irqaction chain
 *                       → action->handler(irq, data, dev_id, pxHPT)
 *
 * Registration (driver init, task context):
 *   request_irq(IRQ_ID_UART_RX(0), my_rx_handler, "uart0_rx", ctx);
 */

#ifndef INCLUDE_IRQ_IRQ_DESC_H_
#define INCLUDE_IRQ_IRQ_DESC_H_

#include <def_std.h>
#include <FreeRTOS.h>
#include <irq/irq_notify.h>   /* irq_id_t, IRQ_ID_TOTAL, IRQ_NOTIFY_MAX_SUBS */
#include <mm/list.h>          /* struct list_node, list_for_each_entry, ... */

#ifdef __cplusplus
extern "C" {
#endif

/* ── IRQ handler return values ───────────────────────────────────────────── */

#define IRQ_NONE    0   /* handler did not service this IRQ */
#define IRQ_HANDLED 1   /* handler serviced this IRQ       */

typedef int irqreturn_t;

/* ── Forward declarations ────────────────────────────────────────────────── */

struct irq_data;
struct irq_chip;
struct irq_desc;
struct irqaction;

/* ── irq_chip — interrupt controller (chip) operations ──────────────────── */

struct irq_chip {
    const char *name;

    void (*irq_enable)(struct irq_data *data);
    void (*irq_disable)(struct irq_data *data);
    void (*irq_ack)(struct irq_data *data);       /* start of new interrupt  */
    void (*irq_mask)(struct irq_data *data);
    void (*irq_mask_ack)(struct irq_data *data);
    void (*irq_unmask)(struct irq_data *data);
    void (*irq_eoi)(struct irq_data *data);        /* end of interrupt        */
    int  (*irq_retrigger)(struct irq_data *data);
    int  (*irq_set_type)(struct irq_data *data, unsigned int flow_type);
    void (*irq_set_affinity)(struct irq_data *data, uint32_t priority);

    unsigned long flags;
};

/* ── irq_common_data — state shared across chip callbacks ────────────────── */

struct irq_common_data {
    unsigned int  state_use_accessors;
    void         *handler_data;    /* optional per-desc payload */
};

/* ── irq_data — per-IRQ chip-facing data ─────────────────────────────────── */

struct irq_data {
    irq_id_t         irq;       /* software IRQ ID (index into irq_desc_table) */
    int32_t          hwirq;     /* hardware IRQ number (NVIC IRQn), -1 = none  */
    struct irq_chip *chip;      /* bound interrupt controller                  */
    struct irq_desc *desc;      /* back-pointer to descriptor                  */
    void            *chip_data; /* chip-private per-IRQ data                   */
};

/* ── irq_handler_t — driver ISR signature ────────────────────────────────── */

/**
 * @param irq    Software IRQ ID that fired.
 * @param data   Event payload (e.g. pointer to received byte, error code).
 * @param dev_id Opaque context supplied to request_irq().
 * @param pxHPT  FreeRTOS yield accumulator; pass to portYIELD_FROM_ISR() after
 *               the dispatch chain returns.  NULL in task context.
 * @return IRQ_HANDLED or IRQ_NONE.
 */
typedef irqreturn_t (*irq_handler_t)(irq_id_t   irq,
                                     void       *data,
                                     void       *dev_id,
                                     BaseType_t *pxHPT);

/* ── irqaction — per-driver handler node (singly-linked list) ────────────── */

struct irqaction {
    irq_handler_t      handler;  /* driver ISR                        */
    void              *dev_id;   /* caller context                    */
    const char        *name;     /* device name (for debug/irq_table) */
    struct list_node   node;     /* linkage in desc->action_list      */
};

/* ── irq_flow_handler_t — chip-level flow handler ────────────────────────── */

typedef void (*irq_flow_handler_t)(struct irq_desc *desc,
                                    void           *data,
                                    BaseType_t     *pxHPT);

/* ── irq_desc — per-IRQ descriptor (embedded-stripped Linux irq_desc) ────── */

struct irq_desc {
    struct irq_common_data  irq_common_data;
    struct irq_data         irq_data;

    irq_flow_handler_t      handle_irq;         /* chip flow handler          */
    struct list_node        action_list;        /* head sentinel of irqaction chain */

    unsigned int            status_use_accessors;
    unsigned int            depth;              /* nested disable nesting     */
    unsigned int            tot_count;          /* total fires ever           */
    unsigned int            irq_count;          /* fires in current window    */
    unsigned long           last_unhandled;     /* tick of last unhandled     */
    unsigned int            irqs_unhandled;     /* fires with no handler      */

    const char             *name;               /* IRQ name from irq_table    */
};

/* ── Flow handlers ────────────────────────────────────────────────────────── */

/**
 * handle_simple_irq  — no chip operations, just walk the action chain.
 *                       Used for software IRQ IDs (UART_RX, I2C_TX_DONE, ...).
 */
void handle_simple_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT);

/**
 * handle_edge_irq    — ack chip (clears pending), then walk action chain.
 *                       Used for GPIO EXTI and peripherals with edge-mode.
 */
void handle_edge_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT);

/**
 * handle_level_irq   — mask → ack → walk chain → unmask.
 *                       Used for level-sensitive interrupt lines.
 */
void handle_level_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT);

/* ── Internal: action chain walker ───────────────────────────────────────── */

void handle_irq_event(struct irq_desc *desc, void *data, BaseType_t *pxHPT);

/* ── Descriptor lookup ────────────────────────────────────────────────────── */

/** O(1) lookup — returns NULL for out-of-range IRQ IDs. */
struct irq_desc *irq_to_desc(irq_id_t irq);

/* ── Central dispatch ─────────────────────────────────────────────────────── */

/**
 * __do_IRQ_from_isr — called from HAL ISR handlers.
 *   Increments irq_count, calls desc->handle_irq().
 *   portYIELD_FROM_ISR(*pxHPT) must be called by the caller after return.
 */
void __do_IRQ_from_isr(irq_id_t irq, void *data, BaseType_t *pxHPT);

/**
 * __do_IRQ — task-context variant (pxHPT = NULL).
 */
void __do_IRQ(irq_id_t irq, void *data);

/* ── Driver registration ──────────────────────────────────────────────────── */

/**
 * request_irq — register a driver ISR for a software IRQ ID.
 *
 * May be called multiple times with different dev_id values to chain handlers.
 * Handlers are called in registration order.
 *
 * @return 0 on success, OS_ERR_OP / OS_ERR_MEM_OF on failure.
 */
int request_irq(irq_id_t irq, irq_handler_t handler,
                const char *name, void *dev_id);

/**
 * free_irq — remove the handler registered with @dev_id from the chain.
 */
void free_irq(irq_id_t irq, void *dev_id);

/* ── Chip and flow-handler binding ────────────────────────────────────────── */

void irq_set_chip(irq_id_t irq, struct irq_chip *chip);
void irq_set_handler(irq_id_t irq, irq_flow_handler_t handle);
void irq_set_chip_and_handler(irq_id_t irq, struct irq_chip *chip,
                               irq_flow_handler_t handle);

/* ── Disable / enable ─────────────────────────────────────────────────────── */

/** irq_disable / irq_enable increment / decrement the depth counter.
    Dispatch is skipped while depth > 0. */
void irq_disable(irq_id_t irq);
void irq_enable(irq_id_t irq);

/* ── Table initialisation (call once at boot before any request_irq) ─────── */

void irq_desc_init_all(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_IRQ_IRQ_DESC_H_ */
