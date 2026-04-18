/*
File:        fsm.c
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      lib/fsm
Info:        Generic event-driven Finite State Machine — standalone,
             no OS dependency.

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.
*/

#include "fsm.h"


/* ================================================================== */
/*  Internal: lock / unlock wrappers                                   */
/* ================================================================== */

/* These macros collapse to nothing when thread safety is disabled,
   producing zero overhead for bare-metal single-context builds.      */

#if (FSM_THREAD_SAFE == 1)

#define FSM_LOCK(fsm)           do { if ((fsm)->lock)     (fsm)->lock();     } while(0)
#define FSM_UNLOCK(fsm)         do { if ((fsm)->unlock)   (fsm)->unlock();   } while(0)
#define FSM_LOCK_ISR(fsm)       do { if ((fsm)->lock_isr) (fsm)->lock_isr(); } while(0)
#define FSM_UNLOCK_ISR(fsm)     do { if ((fsm)->unlock_isr)(fsm)->unlock_isr();} while(0)

#else

#define FSM_LOCK(fsm)           (void)(fsm)
#define FSM_UNLOCK(fsm)         (void)(fsm)
#define FSM_LOCK_ISR(fsm)       (void)(fsm)
#define FSM_UNLOCK_ISR(fsm)     (void)(fsm)

#endif /* FSM_THREAD_SAFE */


/* ================================================================== */
/*  Internal: event queue (intrusive singly-linked FIFO)               */
/* ================================================================== */

/**
 * @brief Append an event to the tail of the internal queue.
 *
 * Caller must hold the appropriate lock before calling.
 */
static void queue_push(fsm_ctx_t *fsm, fsm_event_t *event)
{
    event->next = NULL;

    if (fsm->queue_tail == NULL)
    {
        /* Queue was empty */
        fsm->queue_head = event;
        fsm->queue_tail = event;
    }
    else
    {
        fsm->queue_tail->next = event;
        fsm->queue_tail       = event;
    }
}


/**
 * @brief Remove and return the event at the head of the queue.
 *
 * Returns NULL if the queue is empty.
 * Caller must hold the appropriate lock before calling.
 */
static fsm_event_t *queue_pop(fsm_ctx_t *fsm)
{
    fsm_event_t *e = fsm->queue_head;

    if (e != NULL)
    {
        fsm->queue_head = e->next;

        if (fsm->queue_head == NULL)
        {
            fsm->queue_tail = NULL;
        }

        e->next = NULL;
    }

    return e;
}


/* ================================================================== */
/*  Internal: HSM helper — hierarchy traversal                         */
/* ================================================================== */

/**
 * @brief Walk up the parent chain calling on_exit until 'stop'.
 *
 * 'stop' is NOT exited (it is the LCA — common ancestor of source
 * and target states, or NULL to exit all the way to the root).
 */
static void fsm_exit_up_to(fsm_ctx_t *fsm, const fsm_state_t *stop)
{
    fsm_state_t *s = fsm->current;

    while (s != NULL && s != stop)
    {
        if (s->on_exit != NULL)
        {
            s->on_exit(s, fsm);
        }
        s = s->parent;
    }
}


/**
 * @brief Walk down from 'ancestor' to 'target' calling on_entry.
 *
 * Entry handlers are fired top-down so that outer (parent) states
 * are always entered before inner (child) states.
 */
static void fsm_enter_down_from(fsm_ctx_t         *fsm,
                                const fsm_state_t *ancestor,
                                fsm_state_t       *target)
{
    /* Build the path from target up to (not including) ancestor */
    fsm_state_t *stack[FSM_HIERARCHY_DEPTH_MAX];
    uint32_t     depth = 0U;
    fsm_state_t *s     = target;

    while (s != NULL && s != ancestor)
    {
        if (depth < FSM_HIERARCHY_DEPTH_MAX)
        {
            stack[depth++] = s;
        }
        s = s->parent;
    }

    /* Replay entry handlers from ancestor downwards */
    while (depth > 0U)
    {
        --depth;
        if (stack[depth]->on_entry != NULL)
        {
            stack[depth]->on_entry(stack[depth], fsm);
        }
    }
}


/**
 * @brief Find the Lowest Common Ancestor (LCA) of states a and b.
 *
 * Walks the parent chain of 'a' and for each ancestor checks whether
 * it also appears in the ancestry of 'b'.
 *
 * Returns NULL when there is no common ancestor (flat, no parents).
 */
static fsm_state_t *fsm_lca(const fsm_state_t *a, const fsm_state_t *b)
{
    const fsm_state_t *pa;
    const fsm_state_t *pb;

    for (pa = a; pa != NULL; pa = pa->parent)
    {
        for (pb = b; pb != NULL; pb = pb->parent)
        {
            if (pa == pb)
            {
                return (fsm_state_t *)pa;
            }
        }
    }

    return NULL;
}


/* ================================================================== */
/*  Internal: allocate an event                                         */
/* ================================================================== */

static fsm_event_t *fsm_event_alloc(fsm_event_id_t id, void *data)
{
    fsm_event_t *e = (fsm_event_t *)malloc(sizeof(fsm_event_t));

    if (e == NULL)
    {
        return NULL;
    }

    e->id   = id;
    e->data = data;
    e->next = NULL;

    return e;
}


/* ================================================================== */
/*  Public API                                                          */
/* ================================================================== */

int fsm_init(fsm_ctx_t              *fsm,
             fsm_state_t            *initial,
             const fsm_transition_t *table,
             size_t                  table_len,
             void                   *user_data)
{
    if (fsm == NULL || initial == NULL || table == NULL)
    {
        return FSM_ERR_NULL;
    }

    fsm->current    = initial;
    fsm->table      = table;
    fsm->table_len  = table_len;
    fsm->user_data  = user_data;
    fsm->queue_head = NULL;
    fsm->queue_tail = NULL;
    fsm->running    = 1;

#if (FSM_THREAD_SAFE == 1)
    fsm->lock        = NULL;
    fsm->unlock      = NULL;
    fsm->lock_isr    = NULL;
    fsm->unlock_isr  = NULL;
#endif

    /* Fire entry handler of the initial state */
    if (initial->on_entry != NULL)
    {
        initial->on_entry(initial, fsm);
    }

    return FSM_OK;
}


#if (FSM_THREAD_SAFE == 1)
void fsm_set_lock(fsm_ctx_t    *fsm,
                  fsm_lock_fn   lock,
                  fsm_unlock_fn unlock,
                  fsm_lock_fn   lock_isr,
                  fsm_unlock_fn unlock_isr)
{
    if (fsm == NULL)
    {
        return;
    }

    fsm->lock        = lock;
    fsm->unlock      = unlock;
    fsm->lock_isr    = lock_isr;
    fsm->unlock_isr  = unlock_isr;
}
#endif /* FSM_THREAD_SAFE */


void fsm_stop(fsm_ctx_t *fsm)
{
    if (fsm != NULL)
    {
        fsm->running = 0;
    }
}


int fsm_dispatch(fsm_ctx_t *fsm, fsm_event_id_t id, void *data)
{
    if (fsm == NULL)
    {
        return FSM_ERR_NULL;
    }

    fsm_event_t *e = fsm_event_alloc(id, data);

    if (e == NULL)
    {
        return FSM_ERR_ALLOC;
    }

    FSM_LOCK(fsm);
    queue_push(fsm, e);
    FSM_UNLOCK(fsm);

    return FSM_OK;
}


int fsm_dispatch_from_isr(fsm_ctx_t *fsm, fsm_event_id_t id, void *data)
{
    if (fsm == NULL)
    {
        return FSM_ERR_NULL;
    }

    fsm_event_t *e = fsm_event_alloc(id, data);

    if (e == NULL)
    {
        return FSM_ERR_ALLOC;
    }

    FSM_LOCK_ISR(fsm);
    queue_push(fsm, e);
    FSM_UNLOCK_ISR(fsm);

    return FSM_OK;
}


void fsm_run(fsm_ctx_t *fsm)
{
    if (fsm == NULL || fsm->running == 0)
    {
        return;
    }

    fsm_event_t *event;

    /* Drain the entire queue before returning */
    for (;;)
    {
        /* Pop under lock to avoid race with concurrent dispatch */
        FSM_LOCK(fsm);
        event = queue_pop(fsm);
        FSM_UNLOCK(fsm);

        if (event == NULL)
        {
            break;  /* Queue empty — done for this cycle */
        }

        /* Search transition table for (current_state, event_id) */
        for (size_t i = 0U; i < fsm->table_len; ++i)
        {
            if (fsm->table[i].from  == fsm->current &&
                fsm->table[i].event == event->id)
            {
                fsm_state_t *target = fsm->table[i].to;

                /* Find LCA for correct HSM entry/exit ordering */
                fsm_state_t *lca = fsm_lca(fsm->current, target);

                /* 1. Exit current state up to (not including) LCA */
                fsm_exit_up_to(fsm, lca);

                /* 2. Execute transition action */
                if (fsm->table[i].action != NULL)
                {
                    fsm->table[i].action(event, fsm);
                }

                /* 3. Enter target state down from LCA */
                fsm_enter_down_from(fsm, lca, target);

                /* 4. Commit new state */
                fsm->current = target;

                break;
            }
        }

        /* Events with no matching transition are silently discarded */
        free(event);
    }
}
