/**
 * @file        sysmem.c
 * @brief       sysmem.c — Newlib heap allocation via _sbrk (bare-metal ARM)
 * @ingroup     kernel_glue
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Kernel Glue
 * @info        C-runtime / newlib shim: malloc/free wrappers, syscalls, FreeRTOS kernel adapters.
 * @dependency  newlib-nano, FreeRTOS-Kernel
 *
 * @details
 * sysmem.c — Newlib heap allocation via _sbrk (bare-metal ARM)
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
