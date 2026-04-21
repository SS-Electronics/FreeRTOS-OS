/*
 * irq_desc.c — Linux-style IRQ descriptor chain for FreeRTOS/bare-metal
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Implements the full irq_desc dispatch chain:
 *
 *   __do_IRQ_from_isr(irq_id, data, pxHPT)
 *     → irq_to_desc(irq_id)               — O(1) table lookup
 *     → depth check (disabled?)
 *     → desc->handle_irq(desc, data, pxHPT)  — flow handler
 *        → handle_irq_event(desc, data, pxHPT)  — walk irqaction chain
 *           → action->handler(irq, data, dev_id, pxHPT)
 *
 * Memory:
 *   irqaction nodes are allocated from the FreeRTOS heap via kmaloc/kfree.
 *   Each desc->action_list is a doubly-circular sentinel (mm/list.h).
 *   irq_desc_table is a flat array indexed by irq_id_t.
 */

#include <irq/irq_desc.h>
#include <irq/irq_table.h>
#include <os/kernel_mem.h>
#include <def_err.h>

/* ── Descriptor table ─────────────────────────────────────────────────────── */

static struct irq_desc _irq_desc_table[IRQ_ID_TOTAL];

/* ── Dynamic irqaction allocator ──────────────────────────────────────────── */

static struct irqaction *_alloc_action(void)
{
    struct irqaction *a = (struct irqaction *)kmaloc(sizeof(struct irqaction));
    if (a == NULL)
        return NULL;
    a->handler = NULL;
    a->dev_id  = NULL;
    a->name    = NULL;
    list_init(&a->node);
    return a;
}

static void _free_action(struct irqaction *a)
{
    kfree(a);
}

/* ── Descriptor lookup ────────────────────────────────────────────────────── */

struct irq_desc *irq_to_desc(irq_id_t irq)
{
    if (irq >= IRQ_ID_TOTAL)
        return NULL;
    return &_irq_desc_table[irq];
}

/* ── Action chain walker ──────────────────────────────────────────────────── */

void handle_irq_event(struct irq_desc *desc, void *data, BaseType_t *pxHPT)
{
    if (desc == NULL)
        return;

    struct irqaction *action;
    irqreturn_t       res = IRQ_NONE;

    list_for_each_entry(action, &desc->action_list, node)
    {
        if (action->handler != NULL)
            res |= action->handler(desc->irq_data.irq, data,
                                   action->dev_id, pxHPT);
    }

    if (res == IRQ_NONE)
    {
        desc->irqs_unhandled++;
        desc->last_unhandled = desc->irq_count; /* snapshot for debug */
    }
}

/* ── Flow handlers ────────────────────────────────────────────────────────── */

void handle_simple_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT)
{
    if (desc == NULL)
        return;
    handle_irq_event(desc, data, pxHPT);
}

void handle_edge_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT)
{
    if (desc == NULL)
        return;

    struct irq_chip *chip = desc->irq_data.chip;

    if (chip != NULL && chip->irq_ack != NULL)
        chip->irq_ack(&desc->irq_data);

    handle_irq_event(desc, data, pxHPT);
}

void handle_level_irq(struct irq_desc *desc, void *data, BaseType_t *pxHPT)
{
    if (desc == NULL)
        return;

    struct irq_chip *chip = desc->irq_data.chip;

    if (chip != NULL && chip->irq_mask_ack != NULL)
        chip->irq_mask_ack(&desc->irq_data);
    else if (chip != NULL)
    {
        if (chip->irq_mask != NULL)
            chip->irq_mask(&desc->irq_data);
        if (chip->irq_ack != NULL)
            chip->irq_ack(&desc->irq_data);
    }

    handle_irq_event(desc, data, pxHPT);

    if (chip != NULL && chip->irq_unmask != NULL)
        chip->irq_unmask(&desc->irq_data);
}

/* ── Central dispatch ─────────────────────────────────────────────────────── */

void __do_IRQ_from_isr(irq_id_t irq, void *data, BaseType_t *pxHPT)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc == NULL)
        return;

    desc->irq_count++;
    desc->tot_count++;

    if (desc->depth > 0)
    {
        desc->irqs_unhandled++;
        return;
    }

    if (desc->handle_irq != NULL)
        desc->handle_irq(desc, data, pxHPT);
}

void __do_IRQ(irq_id_t irq, void *data)
{
    struct irq_desc *desc = irq_to_desc(irq);
    if (desc == NULL)
        return;

    desc->irq_count++;
    desc->tot_count++;

    if (desc->depth > 0)
    {
        desc->irqs_unhandled++;
        return;
    }

    if (desc->handle_irq != NULL)
        desc->handle_irq(desc, data, NULL);
}

/* ── Driver registration ──────────────────────────────────────────────────── */

int request_irq(irq_id_t irq, irq_handler_t handler,
                const char *name, void *dev_id)
{
    if (irq >= IRQ_ID_TOTAL || handler == NULL)
        return OS_ERR_OP;

    struct irq_desc *desc = &_irq_desc_table[irq];

    struct irqaction *new_action = _alloc_action();
    if (new_action == NULL)
        return OS_ERR_MEM_OF;

    new_action->handler = handler;
    new_action->dev_id  = dev_id;
    new_action->name    = name;

    list_add_tail(&new_action->node, &desc->action_list);

    return OS_ERR_NONE;
}

void free_irq(irq_id_t irq, void *dev_id)
{
    if (irq >= IRQ_ID_TOTAL)
        return;

    struct irq_desc  *desc = &_irq_desc_table[irq];
    struct irqaction *cur;

    list_for_each_entry(cur, &desc->action_list, node)
    {
        if (cur->dev_id == dev_id)
        {
            list_delete(&cur->node);
            _free_action(cur);
            return;
        }
    }
}

/* ── Chip and flow-handler binding ────────────────────────────────────────── */

void irq_set_chip(irq_id_t irq, struct irq_chip *chip)
{
    if (irq >= IRQ_ID_TOTAL)
        return;
    _irq_desc_table[irq].irq_data.chip = chip;
}

void irq_set_handler(irq_id_t irq, irq_flow_handler_t handle)
{
    if (irq >= IRQ_ID_TOTAL)
        return;
    _irq_desc_table[irq].handle_irq = handle;
}

void irq_set_chip_and_handler(irq_id_t irq, struct irq_chip *chip,
                               irq_flow_handler_t handle)
{
    if (irq >= IRQ_ID_TOTAL)
        return;
    _irq_desc_table[irq].irq_data.chip = chip;
    _irq_desc_table[irq].handle_irq    = handle;
}

/* ── Disable / enable ─────────────────────────────────────────────────────── */

void irq_disable(irq_id_t irq)
{
    if (irq >= IRQ_ID_TOTAL)
        return;

    struct irq_desc *desc = &_irq_desc_table[irq];
    desc->depth++;

    struct irq_chip *chip = desc->irq_data.chip;
    if (chip != NULL && chip->irq_disable != NULL)
        chip->irq_disable(&desc->irq_data);
    else if (chip != NULL && chip->irq_mask != NULL)
        chip->irq_mask(&desc->irq_data);
}

void irq_enable(irq_id_t irq)
{
    if (irq >= IRQ_ID_TOTAL)
        return;

    struct irq_desc *desc = &_irq_desc_table[irq];
    if (desc->depth > 0)
        desc->depth--;

    if (desc->depth == 0)
    {
        struct irq_chip *chip = desc->irq_data.chip;
        if (chip != NULL && chip->irq_enable != NULL)
            chip->irq_enable(&desc->irq_data);
        else if (chip != NULL && chip->irq_unmask != NULL)
            chip->irq_unmask(&desc->irq_data);
    }
}

/* ── Table initialisation ─────────────────────────────────────────────────── */

void irq_desc_init_all(void)
{
    for (irq_id_t i = 0; i < IRQ_ID_TOTAL; i++)
    {
        struct irq_desc *desc = &_irq_desc_table[i];

        desc->irq_common_data.state_use_accessors = 0;
        desc->irq_common_data.handler_data        = NULL;

        desc->irq_data.irq       = i;
        desc->irq_data.hwirq     = -1;
        desc->irq_data.chip      = NULL;
        desc->irq_data.desc      = desc;
        desc->irq_data.chip_data = NULL;

        desc->handle_irq            = handle_simple_irq;
        list_init(&desc->action_list);
        desc->status_use_accessors  = 0;
        desc->depth                 = 0;
        desc->tot_count             = 0;
        desc->irq_count             = 0;
        desc->last_unhandled        = 0;
        desc->irqs_unhandled        = 0;
        desc->name                  = irq_table_get_name(i);
    }
}
