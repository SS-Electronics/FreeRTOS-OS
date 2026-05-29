/*
File:        stm32_h723_FLASH.ld
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      linker script
Info:        STM32H723ZG — Cortex-M7, 1 MB single-bank Flash, 128 KB DTCM
             Flat single-region layout: all code + rodata in FLASH,
             .data in DTCM (LMA in FLASH), DMA buffers in AXI SRAM.
             --gc-sections removes unreferenced functions/data.

Dependency:  None

This file is part of FreeRTOS-OS Project.

FreeRTOS-OS is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version
3 of the License, or (at your option) any later version.

FreeRTOS-OS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>.
*/

/*****************************************************************************
*  Target      : STM32H723ZGTx
*
*  Datasheet   : RM0468 — STM32H723/733/725/735/730 Reference Manual
*
*  Memory map
*  ──────────
*  DTCM  (primary RAM) : 128 KB  @ 0x20000000  (CPU zero-wait, not DMA-able)
*  ITCM                :  64 KB  @ 0x00000000  (not used here)
*  AXI SRAM (D1)       : 320 KB  @ 0x24000000  (DMA-accessible, optional)
*  SRAM1/2 (D2)        :  32 KB  @ 0x30000000
*  SRAM4   (D3)        :  16 KB  @ 0x38000000
*
*  Flash              :   1 MB  @ 0x08000000  (single flat region)
****************************************************************************/

ENTRY(Reset_Handler)

MINIMUM_MEMORY_MODEL = 1;

_Min_Heap_Size  = 0x200;   /* 512 bytes */
_Min_Stack_Size = 0x400;   /* 1 KB      */

MEMORY
{
  DTCM          (xrw) : ORIGIN = 0x20000000, LENGTH = 128K   /* primary CPU RAM         */
  AXI_SRAM      (xrw) : ORIGIN = 0x24000000, LENGTH = 320K   /* DMA-accessible SRAM     */
  FLASH         (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K  /* single flat flash region */
}

/* Top of DTCM → initial stack pointer */
_estack = ORIGIN(DTCM) + LENGTH(DTCM);

SECTIONS
{
  /* Vector table — must stay at Flash origin; never GC'd */
  .isr_vector ORIGIN(FLASH) :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    KEEP(*(.isr_vector.*))
    . = ALIGN(4);
  } > FLASH

  /* Code and read-only data */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    /* crtn.o supplies the _init / _fini epilogs as anonymous .init / .fini
       fragments (no own symbol). Without KEEP, --gc-sections drops them and
       _init falls through into the next code — early BusFault before main. */
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* Exception unwind tables */
  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    KEEP(*(.ARM.extab* .gnu.linkonce.armextab.*))
    . = ALIGN(4);
  } > FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    KEEP(*(.ARM.exidx*))
    __exidx_end = .;
    . = ALIGN(4);
  } > FLASH

  /* C++ init/fini arrays */
  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } > FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } > FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } > FLASH

  /* Initialized data: LMA in Flash, VMA in DTCM */
  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > DTCM AT > FLASH

  /* AXI SRAM — DMA-accessible buffers (opt-in via __attribute__((section(".axi_data")))) */
  .axi_data (NOLOAD) :
  {
    . = ALIGN(32);
    __axi_data_start = .;
    KEEP(*(.axi_data))
    KEEP(*(.axi_data*))
    . = ALIGN(32);
    __axi_data_end = .;
  } > AXI_SRAM

  /* TFLite Micro tensor arena — too large for DTCM, lives in AXI SRAM.
   * Opt-in via __attribute__((section(".inference_arena"))) in the ECG app.
   * NOLOAD: uninitialised scratch, not zeroed at boot (the interpreter owns it). */
  .inference_arena (NOLOAD) :
  {
    . = ALIGN(16);
    __inference_arena_start = .;
    KEEP(*(.inference_arena))
    KEEP(*(.inference_arena*))
    . = ALIGN(16);
    __inference_arena_end = .;
  } > AXI_SRAM

  /* Uninitialized data (BSS) */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > DTCM

  /* Heap and stack reservation */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > DTCM

  /* Not-initialised section — placed AFTER heap/stack so it lies outside
   * [__bss_start__, __bss_end__] and is never zeroed by the BSS clear loop.
   * Contents survive NVIC soft resets; used for fault records and safe-state
   * reason persistence. */
  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    __noinit_start = .;
    KEEP(*(.noinit))
    KEEP(*(.noinit.*))
    . = ALIGN(4);
    __noinit_end = .;
  } > DTCM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

PROVIDE(_sidata  = _sidata);
PROVIDE(_sdata   = _sdata);
PROVIDE(_edata   = _edata);
PROVIDE(_sbss    = __bss_start__);
PROVIDE(_ebss    = __bss_end__);
