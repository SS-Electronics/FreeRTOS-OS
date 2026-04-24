/*
 * def_compiler.h — Compiler abstraction macros
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Provides compiler-portability macros used across the OS and IPC layers.
 * Section-placement attributes are in def_attributes.h.
 */

#ifndef INCLUDE_DEF_COMPILER_H_
#define INCLUDE_DEF_COMPILER_H_

#include <def_attributes.h>

/* ── Inline ────────────────────────────────────────────────────────────── */
#ifndef INLINE
#  define INLINE            __attribute__((always_inline)) static inline
#endif

/* ── Packed struct ─────────────────────────────────────────────────────── */
#ifndef PACKED
#  define PACKED            __attribute__((packed))
#endif

/* ── Weak symbol ───────────────────────────────────────────────────────── */
#ifndef WEAK
#  define WEAK              __attribute__((weak))
#endif

/* ── Alignment ─────────────────────────────────────────────────────────── */
#ifndef ALIGNED
#  define ALIGNED(n)        __attribute__((aligned(n)))
#endif

/* ── Unused suppression ────────────────────────────────────────────────── */
#ifndef UNUSED
#  define UNUSED(x)         ((void)(x))
#endif

#endif /* INCLUDE_DEF_COMPILER_H_ */
