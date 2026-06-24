/**
 * @file        cc.h
 * @brief       lwIP compiler / architecture abstraction for FreeRTOS-OS
 * @ingroup     net_port
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network Port
 * @info        Maps lwIP's compiler primitives (struct packing, diagnostics,
 *              asserts, format specifiers) onto GCC / ARM Cortex-M.
 * @dependency  GCC, def_std.h, printk()
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

#ifndef NET_PORT_ARCH_CC_H_
#define NET_PORT_ARCH_CC_H_

#include <stdint.h>
#include <stddef.h>

/* ── Byte order ─────────────────────────────────────────────────────────── */
/* Cortex-M runs little-endian in every FreeRTOS-OS target. */
#ifndef BYTE_ORDER
#define BYTE_ORDER  LITTLE_ENDIAN
#endif

/* ── Structure packing (GCC attribute form) ─────────────────────────────── */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

/* ── printf-style format specifiers (lwIP uses these in debug/stat output) ─ */
#define X8_F   "02x"
#define U16_F  "u"
#define S16_F  "d"
#define X16_F  "x"
#define U32_F  "lu"
#define S32_F  "ld"
#define X32_F  "lx"
#define SZT_F  "u"

/* ── Memory placement ───────────────────────────────────────────────────── */
/* The lwIP heap (MEM_SIZE) and all memp pools (pbuf pool, PCBs, etc.) are too
 * large to share the 128 KB DTCM with the 80 KB FreeRTOS heap. Place them in
 * AXI SRAM (320 KB, D1) via the project's .axi_data section. These pools are
 * CPU-only (RX/TX DMA buffers live separately in D2), so cacheability is fine.
 * LWIP_MEM_ALIGN_BUFFER is defined later in arch.h; the reference is resolved
 * lazily at each use site in mem.c / memp.c. */
#define LWIP_DECLARE_MEMORY_ALIGNED(variable_name, size) \
	u8_t variable_name[LWIP_MEM_ALIGN_BUFFER(size)] __attribute__((section(".axi_data")))

/* ── Diagnostic / assert hooks ──────────────────────────────────────────── */
/* printk() is the OS-wide formatted console writer (see include/os/kernel_syscall.h).
 * Declared here with the matching signature so the lwIP port stays self-contained. */
extern int32_t printk(const char *fmt, ...);

#define LWIP_PLATFORM_DIAG(x)   do { printk x; } while (0)

#define LWIP_PLATFORM_ASSERT(x)                                      \
	do {                                                            \
		printk("lwIP assert: \"%s\" at %s:%d\n",                  \
		       (x), __FILE__, __LINE__);                          \
		for (;;) { /* trap for the debugger */ }                  \
	} while (0)

#endif /* NET_PORT_ARCH_CC_H_ */
