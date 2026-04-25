/* fsm_config.h — project-local FSM configuration
 * Copy this file into your project and edit as needed.
 * Do NOT edit the version inside lib-fsm directly.      */

#ifndef FSM_CONFIG_H_
#define FSM_CONFIG_H_

#include <os/kernel.h>

/* ------------------------------------------------------------------ */
/*  Memory allocator                                                    */
/* ------------------------------------------------------------------ */

/* Option A: standard heap (default — uncomment to be explicit)
#define FSM_MALLOC(size)   malloc(size)
#define FSM_FREE(ptr)      free(ptr)
*/

// Option B: FreeRTOS heap
#define FSM_MALLOC(size)   pvPortMalloc(size)
#define FSM_FREE(ptr)      vPortFree(ptr)


/* Option C: Zephyr kernel heap
#define FSM_MALLOC(size)   k_malloc(size)
#define FSM_FREE(ptr)      k_free(ptr)
*/

/* Option D: custom memory pool
#define FSM_MALLOC(size)   pool_alloc(&fsm_event_pool, (size))
#define FSM_FREE(ptr)      pool_free(&fsm_event_pool, (ptr))
*/


/* ------------------------------------------------------------------ */
/*  HSM nesting depth                                                   */
/* ------------------------------------------------------------------ */

/* Maximum number of ancestor states in a hierarchy.
 * Each level costs one pointer on the stack during a transition.
 * 8 is sufficient for most designs; raise only if you nest deeper.  */
#define FSM_HIERARCHY_DEPTH_MAX     8U


/* ------------------------------------------------------------------ */
/*  Thread / ISR safety                                                 */
/* ------------------------------------------------------------------ */

/* Set to 1 when the FSM is accessed from multiple RTOS tasks
 * or from both a task and an ISR simultaneously.
 * Set to 0 for bare-metal single-context builds (zero overhead).
 * When enabled, call fsm_set_lock() after fsm_init().               */
#define FSM_THREAD_SAFE             0


#endif /* FSM_CONFIG_H_ */