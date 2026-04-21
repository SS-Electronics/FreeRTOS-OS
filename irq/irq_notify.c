/*
 * irq_notify.c — pub/sub adapter over the irq_desc chain
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Provides the original irq_notify API (irq_register / irq_notify /
 * irq_notify_from_isr) as a thin adapter over irq_desc.c.
 *
 * The signature mismatch between irq_notify_cb_t (void return) and
 * irq_handler_t (irqreturn_t return) is resolved by a static trampoline pool:
 * each irq_register() call allocates a trampoline that wraps the cb+arg pair
 * and is registered with request_irq() as the irq_handler_t.
 *
 * Dispatch path (ISR):
 *   drv_irq_dispatch_from_isr()
 *     → __do_IRQ_from_isr()
 *        → desc->handle_irq()          (handle_simple_irq by default)
 *           → handle_irq_event()
 *              → _notify_trampoline()  (irq_handler_t)
 *                 → irq_notify_cb_t    (original subscriber)
 */

#include <irq/irq_notify.h>
#include <irq/irq_desc.h>
#include <def_err.h>

/* ── Trampoline pool ──────────────────────────────────────────────────────── */

typedef struct {
    irq_notify_cb_t cb;
    void           *arg;
    uint8_t         used;
} _notify_trampoline_t;

#define _TRAMPOLINE_POOL_SIZE   (IRQ_ID_TOTAL * IRQ_NOTIFY_MAX_SUBS)

static _notify_trampoline_t _trampoline_pool[_TRAMPOLINE_POOL_SIZE];

static _notify_trampoline_t *_alloc_trampoline(irq_notify_cb_t cb, void *arg)
{
    for (unsigned int i = 0; i < _TRAMPOLINE_POOL_SIZE; i++)
    {
        if (!_trampoline_pool[i].used)
        {
            _trampoline_pool[i].cb   = cb;
            _trampoline_pool[i].arg  = arg;
            _trampoline_pool[i].used = 1;
            return &_trampoline_pool[i];
        }
    }
    return NULL;
}

/* ── Trampoline handler (irq_handler_t signature) ────────────────────────── */

static irqreturn_t _notify_trampoline(irq_id_t   irq,
                                      void       *data,
                                      void       *dev_id,
                                      BaseType_t *pxHPT)
{
    _notify_trampoline_t *t = (_notify_trampoline_t *)dev_id;
    if (t != NULL && t->cb != NULL)
    {
        t->cb(irq, data, t->arg, pxHPT);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

int32_t irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg)
{
    if (id >= IRQ_ID_TOTAL || cb == NULL)
        return OS_ERR_OP;

    /* Reject duplicate (id, cb, arg) registrations */
    for (unsigned int i = 0; i < _TRAMPOLINE_POOL_SIZE; i++)
    {
        if (_trampoline_pool[i].used &&
            _trampoline_pool[i].cb  == cb &&
            _trampoline_pool[i].arg == arg)
            return OS_ERR_NONE;
    }

    _notify_trampoline_t *t = _alloc_trampoline(cb, arg);
    if (t == NULL)
        return OS_ERR_MEM_OF;

    int32_t err = request_irq(id, _notify_trampoline, NULL, t);
    if (err != OS_ERR_NONE)
    {
        t->used = 0;
        return err;
    }

    return OS_ERR_NONE;
}

void irq_notify(irq_id_t id, void *data)
{
    __do_IRQ(id, data);
}

void irq_notify_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT)
{
    __do_IRQ_from_isr(id, data, pxHPT);
}
