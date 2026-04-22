/*
 * ringbuffer.c — FreeRTOS-safe ring buffer (mirror-bit algorithm)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Dynamic memory model:
 *   ringbuffer_create()  — kmaloc() for descriptor + data buffer
 *   ringbuffer_destroy() — kfree() for both
 *
 * Thread safety:
 *   putchar / putchar_force — ISR-safe via taskENTER_CRITICAL_FROM_ISR /
 *                             taskEXIT_CRITICAL_FROM_ISR.  Safe to call
 *                             from both ISR and task context.
 *   all other ops           — task-safe via taskENTER_CRITICAL /
 *                             taskEXIT_CRITICAL.  Must not be called from
 *                             ISR context.
 *   FreeRTOS nests critical sections by reference count so mixing the two
 *   variants on a single-core Cortex-M4 is deadlock-free.
 */

#include <ipc/ringbuffer.h>
#include <os/kernel_mem.h>

#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

/* ── Internal helpers (no lock — callers hold critical section) ─────────── */

static inline enum ringbuffer_state _status(const struct ringbuffer *rb)
{
    if (rb->read_index != rb->write_index)
        return RT_RINGBUFFER_HALFFULL;
    return (rb->read_mirror == rb->write_mirror)
           ? RT_RINGBUFFER_EMPTY : RT_RINGBUFFER_FULL;
}

static inline uint16_t _data_len(const struct ringbuffer *rb)
{
    switch (_status(rb)) {
    case RT_RINGBUFFER_EMPTY: return 0;
    case RT_RINGBUFFER_FULL:  return (uint16_t)rb->buffer_size;
    default:
        return (rb->write_index > rb->read_index)
               ? rb->write_index - rb->read_index
               : (uint16_t)rb->buffer_size - (rb->read_index - rb->write_index);
    }
}

/* ── Dynamic lifecycle ──────────────────────────────────────────────────── */

struct ringbuffer *ringbuffer_create(uint16_t size)
{
    if (size == 0)
        return NULL;

    struct ringbuffer *rb = (struct ringbuffer *)kmaloc(sizeof(*rb));
    if (rb == NULL)
        return NULL;

    uint8_t *buf = (uint8_t *)kmaloc(size);
    if (buf == NULL) {
        kfree(rb);
        return NULL;
    }

    ringbuffer_init(rb, buf, (int16_t)size);
    return rb;
}

void ringbuffer_destroy(struct ringbuffer *rb)
{
    if (rb == NULL)
        return;
    if (rb->buffer_ptr != NULL)
        kfree(rb->buffer_ptr);
    kfree(rb);
}

/* ── Init / flush ───────────────────────────────────────────────────────── */

void ringbuffer_init(struct ringbuffer *rb, uint8_t *pool, int16_t size)
{
    rb->read_mirror  = 0;
    rb->read_index   = 0;
    rb->write_mirror = 0;
    rb->write_index  = 0;
    rb->buffer_ptr   = pool;
    rb->buffer_size  = size;
}

void ringbuffer_flush(struct ringbuffer *rb)
{
    taskENTER_CRITICAL();
    rb->read_mirror  = 0;
    rb->read_index   = 0;
    rb->write_mirror = 0;
    rb->write_index  = 0;
    taskEXIT_CRITICAL();
}

/* ── Queries (task context) ─────────────────────────────────────────────── */

enum ringbuffer_state ringbuffer_status(struct ringbuffer *rb)
{
    taskENTER_CRITICAL();
    enum ringbuffer_state s = _status(rb);
    taskEXIT_CRITICAL();
    return s;
}

uint16_t ringbuffer_data_len(struct ringbuffer *rb)
{
    taskENTER_CRITICAL();
    uint16_t len = _data_len(rb);
    taskEXIT_CRITICAL();
    return len;
}

uint16_t ringbuffer_get_size(struct ringbuffer *rb)
{
    return (uint16_t)rb->buffer_size;
}

/* ── Multi-byte put (task context) ──────────────────────────────────────── */

uint32_t ringbuffer_put(struct ringbuffer *rb, const uint8_t *ptr, uint16_t length)
{
    taskENTER_CRITICAL();

    uint16_t space = (uint16_t)rb->buffer_size - _data_len(rb);
    if (space == 0) {
        taskEXIT_CRITICAL();
        return 0;
    }
    if (space < length)
        length = space;

    uint16_t tail = (uint16_t)rb->buffer_size - rb->write_index;
    if (tail > length) {
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
        rb->write_index += length;
    } else {
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, tail);
        memcpy(&rb->buffer_ptr[0], ptr + tail, length - tail);
        rb->write_mirror = (uint16_t)(~rb->write_mirror) & 1U;
        rb->write_index  = length - tail;
    }

    taskEXIT_CRITICAL();
    return length;
}

uint32_t ringbuffer_put_force(struct ringbuffer *rb, const uint8_t *ptr, uint16_t length)
{
    taskENTER_CRITICAL();

    enum ringbuffer_state old = _status(rb);
    if (length > (uint16_t)rb->buffer_size)
        length = (uint16_t)rb->buffer_size;

    uint16_t tail = (uint16_t)rb->buffer_size - rb->write_index;
    if (tail > length) {
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
        rb->write_index += length;
        if (old == RT_RINGBUFFER_FULL)
            rb->read_index = rb->write_index;
    } else {
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, tail);
        memcpy(&rb->buffer_ptr[0], ptr + tail, length - tail);
        rb->write_mirror = (uint16_t)(~rb->write_mirror) & 1U;
        rb->write_index  = length - tail;
        if (old == RT_RINGBUFFER_FULL) {
            rb->read_mirror = (uint16_t)(~rb->read_mirror) & 1U;
            rb->read_index  = rb->write_index;
        }
    }

    taskEXIT_CRITICAL();
    return length;
}

/* ── Single-char put — ISR-safe ─────────────────────────────────────────── */

uint32_t ringbuffer_putchar(struct ringbuffer *rb, uint8_t ch)
{
    UBaseType_t ux = taskENTER_CRITICAL_FROM_ISR();

    uint32_t ret = 0;
    if (_data_len(rb) < (uint16_t)rb->buffer_size) {
        rb->buffer_ptr[rb->write_index] = ch;
        if (rb->write_index == (uint16_t)(rb->buffer_size - 1)) {
            rb->write_mirror = (uint16_t)(~rb->write_mirror) & 1U;
            rb->write_index  = 0;
        } else {
            rb->write_index++;
        }
        ret = 1;
    }

    taskEXIT_CRITICAL_FROM_ISR(ux);
    return ret;
}

uint32_t ringbuffer_putchar_force(struct ringbuffer *rb, uint8_t ch)
{
    UBaseType_t ux = taskENTER_CRITICAL_FROM_ISR();

    enum ringbuffer_state old = _status(rb);
    rb->buffer_ptr[rb->write_index] = ch;

    if (rb->write_index == (uint16_t)(rb->buffer_size - 1)) {
        rb->write_mirror = (uint16_t)(~rb->write_mirror) & 1U;
        rb->write_index  = 0;
        if (old == RT_RINGBUFFER_FULL) {
            rb->read_mirror = (uint16_t)(~rb->read_mirror) & 1U;
            rb->read_index  = 0;
        }
    } else {
        rb->write_index++;
        if (old == RT_RINGBUFFER_FULL)
            rb->read_index = rb->write_index;
    }

    taskEXIT_CRITICAL_FROM_ISR(ux);
    return 1;
}

/* ── Multi-byte get (task context) ──────────────────────────────────────── */

uint32_t ringbuffer_get(struct ringbuffer *rb, uint8_t *ptr, uint16_t length)
{
    taskENTER_CRITICAL();

    uint16_t avail = _data_len(rb);
    if (avail == 0) {
        taskEXIT_CRITICAL();
        return 0;
    }
    if (avail < length)
        length = avail;

    uint16_t tail = (uint16_t)rb->buffer_size - rb->read_index;
    if (tail > length) {
        memcpy(ptr, &rb->buffer_ptr[rb->read_index], length);
        rb->read_index += length;
    } else {
        memcpy(ptr, &rb->buffer_ptr[rb->read_index], tail);
        memcpy(ptr + tail, &rb->buffer_ptr[0], length - tail);
        rb->read_mirror = (uint16_t)(~rb->read_mirror) & 1U;
        rb->read_index  = length - tail;
    }

    taskEXIT_CRITICAL();
    return length;
}

/* ── Single-char get (task context) ─────────────────────────────────────── */

uint32_t ringbuffer_getchar(struct ringbuffer *rb, uint8_t *ch)
{
    taskENTER_CRITICAL();

    uint32_t ret = 0;
    if (_data_len(rb) > 0) {
        *ch = rb->buffer_ptr[rb->read_index];
        if (rb->read_index == (uint16_t)(rb->buffer_size - 1)) {
            rb->read_mirror = (uint16_t)(~rb->read_mirror) & 1U;
            rb->read_index  = 0;
        } else {
            rb->read_index++;
        }
        ret = 1;
    }

    taskEXIT_CRITICAL();
    return ret;
}
