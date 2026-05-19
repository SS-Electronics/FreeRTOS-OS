/**
 * @file        def_compiler.h
 * @brief       def_compiler.h — Compiler abstraction macros
 * @ingroup     config
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Definitions
 * @info        Project-wide attribute / compiler / standard-type macros and error code enums.
 * @dependency  Compiler intrinsics, CMSIS
 *
 * @details
 * def_compiler.h — Compiler abstraction macros
 *
 * Provides compiler-portability macros used across the OS and IPC layers.
 * Section-placement attributes are in def_attributes.h.
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_DEF_COMPILER_H_
#define INCLUDE_DEF_COMPILER_H_

#include <def_attributes.h>

/* ── Inline ────────────────────────────────────────────────────────────── */
#ifndef __INLINE
#  define __INLINE            __attribute__((always_inline)) static inline
#endif

/* ── Packed struct ─────────────────────────────────────────────────────── */
/* CMSIS cmsis_gcc.h defines __PACKED as __attribute__((packed, aligned(1))).
   Keep CMSIS's stronger definition when present to preserve unaligned-access
   semantics in structs shared with CMSIS-typed registers. */
#ifndef __PACKED
#  define __PACKED            __attribute__((packed))
#endif

/* ── Weak symbol ───────────────────────────────────────────────────────── */
#ifndef __WEAK
#  define __WEAK              __attribute__((weak))
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
