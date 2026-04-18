/*
File:        fsm_config.h
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      lib/fsm
Info:        Compile-time configuration for the generic FSM library.
             Edit this file to tune the library for your target.

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.
*/

#ifndef LIB_FSM_FSM_CONFIG_H_
#define LIB_FSM_FSM_CONFIG_H_


/* ------------------------------------------------------------------ */
/*  Hierarchy depth                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Maximum number of ancestor states (HSM nesting depth).
 *
 * Increase if you have deeply nested hierarchical states.
 * Each level costs one pointer slot on the stack during transitions.
 *
 * Default: 8
 */
#define FSM_HIERARCHY_DEPTH_MAX     8U


/* ------------------------------------------------------------------ */
/*  Thread / interrupt safety                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Enable mutex-based protection of the internal event queue.
 *
 * Set to 1 when the FSM is accessed from:
 *   - Multiple RTOS tasks
 *   - A task AND an ISR simultaneously
 *
 * Set to 0 for bare-metal single-context use (no overhead).
 *
 * When enabled, provide lock/unlock hooks via fsm_set_lock() after
 * fsm_init().
 *
 * Default: 0
 */
#define FSM_THREAD_SAFE             0


#endif /* LIB_FSM_FSM_CONFIG_H_ */
