/*
 * ringbuffer.h — FreeRTOS-safe ring buffer (mirror-bit algorithm)
 *
 * This file is part of FreeRTOS-OS Project.
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
