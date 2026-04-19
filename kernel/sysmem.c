/*
 * sysmem.c — Newlib heap allocation via _sbrk (bare-metal ARM)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * _sbrk() is the low-level heap extension hook used by malloc and the C
 * library.  Grows the heap from _end (top of .bss) upward and guards against
 * encroaching on the reserved MSP stack region defined by the linker script.
 *
 * Linker symbols required:
 *   _end            — first address past .bss (heap start)
 *   _estack         — top of RAM (stack origin)
 *   _Min_Stack_Size — minimum stack reservation
 *
 * These stubs are architecture-agnostic — they compile for any arm-none-eabi
 * target whose linker script exports the symbols above.
 */

#include <errno.h>
#include <stdint.h>

static uint8_t *__sbrk_heap_end = NULL;

void *_sbrk(ptrdiff_t incr)
{
    extern uint8_t  _end;
    extern uint8_t  _estack;
    extern uint32_t _Min_Stack_Size;

    const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
    const uint8_t *max_heap    = (uint8_t *)stack_limit;
    uint8_t       *prev_heap_end;

    if (__sbrk_heap_end == NULL)
        __sbrk_heap_end = &_end;

    if (__sbrk_heap_end + incr > max_heap)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    prev_heap_end    = __sbrk_heap_end;
    __sbrk_heap_end += incr;
    return (void *)prev_heap_end;
}
