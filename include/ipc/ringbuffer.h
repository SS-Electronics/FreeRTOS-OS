/**
 * @file        ringbuffer.h
 * @brief       ringbuffer.h — FreeRTOS-safe ring buffer (mirror-bit algorithm)
 * @ingroup     ipc
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      IPC
 * @info        Inter-process communication: ring buffers (HW byte streams) and xQueue-based message queues.
 * @dependency  FreeRTOS queue, ringbuffer.h
 *
 * @details
 * ringbuffer.h — FreeRTOS-safe ring buffer (mirror-bit algorithm)
 *
 * Dynamic model:
 *   ringbuffer_create(size) — allocates descriptor + data buffer via kmaloc()
 *   ringbuffer_destroy(rb)  — frees both via kfree()
 *
 * Static model (pre-allocated pool):
 *   ringbuffer_init(rb, pool, size) — initialises into caller-owned memory
 *
 * Thread safety:
 *   putchar / putchar_force — ISR-safe (taskENTER_CRITICAL_FROM_ISR)
 *   all other ops            — task-safe (taskENTER_CRITICAL)
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

#ifndef INCLUDE_IPC_RINGBUFFER_H_
#define INCLUDE_IPC_RINGBUFFER_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Descriptor ─────────────────────────────────────────────────────────── */

/*
 * Mirror-bit ring buffer.
 *
 * read_index / write_index are 15-bit values (max buffer size 32 KiB).
 * The single mirror bit distinguishes full from empty when both indices
 * are equal: same mirror → empty, different mirror → full.
 */
struct ringbuffer {
    uint8_t  *buffer_ptr;
    uint16_t  read_mirror  : 1;
    uint16_t  read_index   : 15;
    uint16_t  write_mirror : 1;
    uint16_t  write_index  : 15;
    int16_t   buffer_size;
};

enum ringbuffer_state {
    RT_RINGBUFFER_EMPTY,
    RT_RINGBUFFER_FULL,
    RT_RINGBUFFER_HALFFULL,
};

/* ── Dynamic lifecycle ──────────────────────────────────────────────────── */

struct ringbuffer *ringbuffer_create(uint16_t size);
void               ringbuffer_destroy(struct ringbuffer *rb);

/* ── Static init / flush ────────────────────────────────────────────────── */

void ringbuffer_init(struct ringbuffer *rb, uint8_t *pool, int16_t size);
void ringbuffer_flush(struct ringbuffer *rb);

/* ── Multi-byte put — task context ──────────────────────────────────────── */

uint32_t ringbuffer_put(struct ringbuffer *rb, const uint8_t *ptr, uint16_t length);
uint32_t ringbuffer_put_force(struct ringbuffer *rb, const uint8_t *ptr, uint16_t length);

/* ── Single-char put — ISR-safe ─────────────────────────────────────────── */

uint32_t ringbuffer_putchar(struct ringbuffer *rb, uint8_t ch);
uint32_t ringbuffer_putchar_force(struct ringbuffer *rb, uint8_t ch);

/* ── Multi-byte get — task context ──────────────────────────────────────── */

uint32_t ringbuffer_get(struct ringbuffer *rb, uint8_t *ptr, uint16_t length);

/* ── Single-char get — task context ─────────────────────────────────────── */

uint32_t ringbuffer_getchar(struct ringbuffer *rb, uint8_t *ch);

/* ── Single-char get — ISR-safe ─────────────────────────────────────────── */

uint32_t ringbuffer_getchar_from_isr(struct ringbuffer *rb, uint8_t *ch);

/* ── Queries ─────────────────────────────────────────────────────────────── */

enum ringbuffer_state ringbuffer_status(struct ringbuffer *rb);
uint16_t              ringbuffer_data_len(struct ringbuffer *rb);
uint16_t              ringbuffer_get_size(struct ringbuffer *rb);

#define ringbuffer_empty_space(rb) \
    ((uint16_t)((rb)->buffer_size) - ringbuffer_data_len(rb))

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_IPC_RINGBUFFER_H_ */
