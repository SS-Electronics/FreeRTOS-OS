/*
File:        fsm.h
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      lib/fsm
Info:        Generic event-driven Finite State Machine — standalone,
             no OS dependency.

             Works on any MCU (bare-metal, FreeRTOS, Zephyr, …).
             Thread/ISR safety is opt-in via lock/unlock hooks.

  ┌──────────────────────────────────────────────────────────────┐
  │  Typical usage inside an RTOS task                           │
  │                                                              │
  │  void my_task(void *arg) {                                   │
  │      fsm_ctx_t *fsm = (fsm_ctx_t *)arg;                      │
  │      for (;;) {                                              │
  │          /* wait for work (semaphore, flag, delay …) */      │
  │          fsm_run(fsm);     // drain all pending events       │
  │      }                                                       │
  │  }                                                           │
  │                                                              │
  │  /* from another task */                                     │
  │  fsm_dispatch(fsm, EVT_START, NULL);                         │
  │                                                              │
  │  /* from an ISR */                                           │
  │  fsm_dispatch_from_isr(fsm, EVT_DONE, NULL);                 │
  └──────────────────────────────────────────────────────────────┘

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.
*/

#ifndef LIB_FSM_FSM_H_
#define LIB_FSM_FSM_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>     /* malloc / free */

#include "fsm_config.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ================================================================== */
/*  Forward declarations                                               */
/* ================================================================== */

struct fsm_state;
struct fsm_ctx;


/* ================================================================== */
/*  Result codes                                                        */
/* ================================================================== */

#define FSM_OK          ( 0)
#define FSM_ERR_NULL    (-1)    /**< NULL pointer passed             */
#define FSM_ERR_ALLOC   (-2)    /**< malloc returned NULL            */


/* ================================================================== */
/*  Event                                                               */
/* ================================================================== */

/** @brief Event identifier — define your own enum and cast to this. */
typedef uint32_t fsm_event_id_t;

/**
 * @brief FSM event object.
 *
 * Allocated on the heap by fsm_dispatch() / fsm_dispatch_from_isr()
 * and automatically freed by fsm_run() after the transition completes.
 *
 * 'data' is a pointer to caller-managed storage; the FSM never touches
 * or frees it.
 */
typedef struct fsm_event
{
    fsm_event_id_t    id;    /**< Event identifier                   */
    void             *data;  /**< Optional payload (caller-managed)  */
    struct fsm_event *next;  /**< Intrusive linked-list linkage      */
} fsm_event_t;


/* ================================================================== */
/*  Callback signatures                                                 */
/* ================================================================== */

/**
 * @brief Transition action callback.
 *
 * Executed between the exit handler(s) of the source state and the
 * entry handler(s) of the target state.
 *
 * @param event  Triggering event (read-only during action).
 * @param fsm    FSM context — access application data via fsm->user_data.
 */
typedef void (*fsm_action_fn)(const fsm_event_t *event,
                              struct fsm_ctx    *fsm);

/**
 * @brief State entry / exit callback.
 *
 * @param state  The state being entered or exited.
 * @param fsm    FSM context.
 */
typedef void (*fsm_state_fn)(struct fsm_state *state,
                             struct fsm_ctx   *fsm);

/**
 * @brief Lock / unlock hook for queue protection.
 *
 * Provide platform-specific implementations when FSM_THREAD_SAFE == 1.
 *
 * Examples:
 *   Bare-metal ISR-safe:  __disable_irq() / __enable_irq()
 *   FreeRTOS task:        xSemaphoreTake / xSemaphoreGive
 *   FreeRTOS task+ISR:    taskENTER_CRITICAL / taskEXIT_CRITICAL
 *   CMSIS-RTOS:           osMutexAcquire / osMutexRelease
 */
typedef void (*fsm_lock_fn)(void);
typedef void (*fsm_unlock_fn)(void);


/* ================================================================== */
/*  State descriptor                                                    */
/* ================================================================== */

/**
 * @brief A single state in the FSM / HSM.
 *
 * Set 'parent' to enable hierarchical (nested) states — the FSM will
 * automatically call exit/entry handlers in the correct top-down order.
 *
 * Static initialiser example:
 * @code
 *   static fsm_state_t st_idle = {
 *       .id       = ST_IDLE,
 *       .parent   = NULL,
 *       .on_entry = idle_entry,
 *       .on_exit  = NULL,
 *   };
 * @endcode
 */
typedef struct fsm_state
{
    uint32_t          id;       /**< Unique state identifier          */
    struct fsm_state *parent;   /**< Parent state (HSM) or NULL       */
    fsm_state_fn      on_entry; /**< Entry handler or NULL            */
    fsm_state_fn      on_exit;  /**< Exit  handler or NULL            */
} fsm_state_t;


/* ================================================================== */
/*  Transition table row                                                */
/* ================================================================== */

/**
 * @brief One row of the transition table.
 *
 * Build a const array of these and pass it to fsm_init().
 *
 * @code
 *   static const fsm_transition_t tbl[] = {
 *    // { from,      event,      to,        action      }
 *       { &st_idle,  EVT_START,  &st_run,   act_start   },
 *       { &st_run,   EVT_DONE,   &st_idle,  act_cleanup },
 *       { &st_run,   EVT_ERROR,  &st_err,   act_log     },
 *   };
 * @endcode
 */
typedef struct
{
    fsm_state_t    *from;   /**< Source state                        */
    fsm_event_id_t  event;  /**< Trigger event id                    */
    fsm_state_t    *to;     /**< Target state                        */
    fsm_action_fn   action; /**< Transition action or NULL           */
} fsm_transition_t;


/* ================================================================== */
/*  FSM context                                                         */
/* ================================================================== */

/**
 * @brief Runtime context of one FSM instance.
 *
 * Embed one per logical state machine; may be static or heap-allocated.
 * Initialise with fsm_init() before use.
 */
typedef struct fsm_ctx
{
    fsm_state_t              *current;      /**< Active state         */

    const fsm_transition_t   *table;        /**< Transition table     */
    size_t                    table_len;    /**< Number of rows       */

    void                     *user_data;    /**< Caller context       */

    /* Internal FIFO event queue (intrusive linked-list) */
    fsm_event_t              *queue_head;   /**< Front of queue       */
    fsm_event_t              *queue_tail;   /**< Back  of queue       */

    volatile int32_t          running;      /**< 1=active, 0=stopped  */

#if (FSM_THREAD_SAFE == 1)
    fsm_lock_fn               lock;         /**< Task-context lock    */
    fsm_unlock_fn             unlock;       /**< Task-context unlock  */
    fsm_lock_fn               lock_isr;     /**< ISR-context lock     */
    fsm_unlock_fn             unlock_isr;   /**< ISR-context unlock   */
#endif

} fsm_ctx_t;


/* ================================================================== */
/*  API                                                                 */
/* ================================================================== */

/**
 * @brief Initialise an FSM instance.
 *
 * Clears the internal queue, sets the initial state, and fires its
 * on_entry handler.
 *
 * @param fsm          FSM context (caller-allocated; may be static).
 * @param initial      First active state.
 * @param table        Transition table array (may be const / ROM).
 * @param table_len    Number of rows in 'table'.
 * @param user_data    Arbitrary context passed to every callback.
 *
 * @return FSM_OK or FSM_ERR_NULL.
 */
int fsm_init(fsm_ctx_t              *fsm,
             fsm_state_t            *initial,
             const fsm_transition_t *table,
             size_t                  table_len,
             void                   *user_data);


#if (FSM_THREAD_SAFE == 1)
/**
 * @brief Register lock/unlock hooks for queue protection.
 *
 * Call this after fsm_init() and before the first fsm_dispatch().
 *
 * @param fsm          FSM context.
 * @param lock         Lock function for task context.
 * @param unlock       Unlock function for task context.
 * @param lock_isr     Lock function for ISR context.
 * @param unlock_isr   Unlock function for ISR context.
 *
 * @note  If task and ISR share the same critical-section primitive
 *        (e.g. taskENTER_CRITICAL on Cortex-M), pass the same
 *        function pointers for both pairs.
 */
void fsm_set_lock(fsm_ctx_t    *fsm,
                  fsm_lock_fn   lock,
                  fsm_unlock_fn unlock,
                  fsm_lock_fn   lock_isr,
                  fsm_unlock_fn unlock_isr);
#endif /* FSM_THREAD_SAFE */


/**
 * @brief Signal the FSM to stop after the current event finishes.
 *
 * Sets fsm->running = 0.  fsm_run() will return immediately on the
 * next call.
 *
 * @param fsm  FSM context.
 */
void fsm_stop(fsm_ctx_t *fsm);


/**
 * @brief Post an event from task / thread context.
 *
 * Allocates an event on the heap and appends it to the internal queue.
 * The event is freed automatically by fsm_run() after processing.
 *
 * @param fsm   FSM context.
 * @param id    Event identifier.
 * @param data  Optional payload pointer (caller manages lifetime).
 *
 * @return FSM_OK, FSM_ERR_NULL, or FSM_ERR_ALLOC.
 */
int fsm_dispatch(fsm_ctx_t     *fsm,
                 fsm_event_id_t id,
                 void          *data);


/**
 * @brief Post an event from an ISR.
 *
 * Identical to fsm_dispatch() but uses the ISR lock/unlock hooks
 * instead of the task hooks.
 *
 * @note  malloc() is called internally.  Ensure your heap allocator
 *        is interrupt-safe (e.g. FreeRTOS heap_4 with
 *        configUSE_NEWLIB_REENTRANT or a custom ISR-safe allocator).
 *        Alternatively call from a deferred interrupt task.
 *
 * @param fsm   FSM context.
 * @param id    Event identifier.
 * @param data  Optional payload pointer.
 *
 * @return FSM_OK, FSM_ERR_NULL, or FSM_ERR_ALLOC.
 */
int fsm_dispatch_from_isr(fsm_ctx_t     *fsm,
                          fsm_event_id_t id,
                          void          *data);


/**
 * @brief Process all pending events — call this in your task loop.
 *
 * Drains the internal queue: for each event it finds the matching
 * transition, runs exit → action → entry handlers, updates the active
 * state, and frees the event.  Returns when the queue is empty.
 *
 * Events with no matching transition for the current state are
 * silently discarded.
 *
 * Typical patterns:
 * @code
 *   // Bare-metal super-loop
 *   for (;;) { fsm_run(&fsm); }
 *
 *   // RTOS task — unblock via semaphore/notification, then drain
 *   for (;;) {
 *       ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
 *       fsm_run(&fsm);
 *   }
 * @endcode
 *
 * @param fsm  FSM context.
 */
void fsm_run(fsm_ctx_t *fsm);


#ifdef __cplusplus
}
#endif

#endif /* LIB_FSM_FSM_H_ */
