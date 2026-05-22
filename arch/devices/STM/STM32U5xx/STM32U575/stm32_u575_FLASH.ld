/*
File:        stm32_u575_FLASH.ld
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      linker script
Info:        STM32U575ZI — Cortex-M33 + TrustZone, 2 MB dual-bank Flash,
             786 KB SRAM (SRAM1 192K + SRAM2 64K + SRAM3 512K + Backup 16K).

             Single-image layout — all code/rodata in user flash, .data
             initialised into SRAM1 with LMA in flash, BSS in SRAM1.
             SRAM3 is reserved here for opt-in large buffers (DMA / image
             buffers / TFLite arenas) via __attribute__((section(".sram3_data"))).

             --gc-sections removes unreferenced functions/data.

Dependency:  None

This file is part of FreeRTOS-OS Project.
FreeRTOS-OS is free software: see <https://www.gnu.org/licenses/>.
*/

/*****************************************************************************
*  Target      : STM32U575ZITxQ (Nucleo-U575ZI-Q)
*
*  Datasheet   : RM0456 — STM32U5 series Reference Manual
*
*  Memory map (non-secure aliases — single-image build)
*  ──────────
*  SRAM1 (primary RAM) : 192 KB  @ 0x20000000
*  SRAM2               :  64 KB  @ 0x20030000
*  SRAM3 (large pool)  : 512 KB  @ 0x20040000
*  Backup SRAM         :  16 KB  @ 0x28000000
*
*  Flash               :   2 MB  @ 0x08000000 (single contiguous region —
*                                              dual-bank handled by BFB2)
****************************************************************************/

ENTRY(Reset_Handler)

MINIMUM_MEMORY_MODEL = 1;

_Min_Heap_Size  = 0x400;   /* 1 KB heap  */
_Min_Stack_Size = 0x800;   /* 2 KB stack */

MEMORY
{
  SRAM1   (xrw) : ORIGIN = 0x20000000, LENGTH = 192K  /* primary CPU RAM    */
  SRAM2   (xrw) : ORIGIN = 0x20030000, LENGTH = 64K   /* secondary          */
  SRAM3   (xrw) : ORIGIN = 0x20040000, LENGTH = 512K  /* large-buffer pool  */
  FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K /* user flash         */
}

/* Top of SRAM1 → initial MSP. */
_estack = ORIGIN(SRAM1) + LENGTH(SRAM1);

SECTIONS
{
  /* Vector table — must stay at Flash origin; never GC'd. */
  .isr_vector ORIGIN(FLASH) :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    KEEP(*(.isr_vector.*))
    . = ALIGN(4);
  } > FLASH

  /* Code and read-only data. */
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
       _init falls through into the next code — early BusFault before main.
       Same regression as on the F4 / H7 ports — keep this wrap forever. */
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

  /* Exception unwind tables. */
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

  /* C++ init/fini arrays. */
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

  /* Trustcore secure-callable functions live in a dedicated section so the
   * SAU/IDAU can mark them executable from the non-secure side once
   * TrustZone is enabled. Today they are plain code (single-image build). */
  .trustcore_text :
  {
    . = ALIGN(4);
    __trustcore_text_start = .;
    KEEP(*(.trustcore_text))
    KEEP(*(.trustcore_text.*))
    . = ALIGN(4);
    __trustcore_text_end = .;
  } > FLASH

  /* Initialised data: LMA in Flash, VMA in SRAM1. */
  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > SRAM1 AT > FLASH

  /* SRAM3 — opt-in pool for large DMA / image / inference buffers. */
  .sram3_data (NOLOAD) :
  {
    . = ALIGN(32);
    __sram3_data_start = .;
    KEEP(*(.sram3_data))
    KEEP(*(.sram3_data*))
    . = ALIGN(32);
    __sram3_data_end = .;
  } > SRAM3

  /* Uninitialised data (BSS). */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > SRAM1

  /* Heap and stack reservation. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > SRAM1

  /* Not-initialised section — outside [__bss_start__, __bss_end__] so the
   * BSS clear loop in Reset_Handler doesn't wipe fault records / trustcore
   * attestation state across soft resets. */
  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    __noinit_start = .;
    KEEP(*(.noinit))
    KEEP(*(.noinit.*))
    . = ALIGN(4);
    __noinit_end = .;
  } > SRAM1

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
