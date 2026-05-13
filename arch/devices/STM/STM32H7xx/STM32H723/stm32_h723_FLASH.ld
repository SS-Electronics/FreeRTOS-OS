/*
File:        stm32_h723_FLASH.ld
Author:      Subhajit Roy
             subhajitroy005@gmail.com

Module:      linker script
Info:        STM32H723ZG — Cortex-M7, 1 MB single-bank Flash, 128 KB DTCM
             Boot / OS / Application three-region layout.
             DTCM is the primary execution RAM (zero-wait-state from CPU).
             AXI SRAM (320 KB @ 0x24000000) is available for DMA-accessible
             buffers — place explicitly via __attribute__((section(".axi_data"))).

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
*  Flash sectors (single bank, 128 KB each)
*  ─────────────────────────────────────────
*  Sector 0 : 0x08000000 – 0x0801FFFF  (128 KB)   FLASH_BOOT
*  Sector 1 : 0x08020000 – 0x0803FFFF  (128 KB) ┐
*  Sector 2 : 0x08040000 – 0x0805FFFF  (128 KB) │ FLASH_OS  (384 KB)
*  Sector 3 : 0x08060000 – 0x0807FFFF  (128 KB) ┘
*  Sector 4 : 0x08080000 – 0x0809FFFF  (128 KB) ┐
*  Sector 5 : 0x080A0000 – 0x080BFFFF  (128 KB) │
*  Sector 6 : 0x080C0000 – 0x080DFFFF  (128 KB) │ FLASH_APP (512 KB)
*  Sector 7 : 0x080E0000 – 0x080FFFFF  (128 KB) ┘
****************************************************************************/

ENTRY(Reset_Handler)

MINIMUM_MEMORY_MODEL = 1;

_Min_Heap_Size  = 0x200;   /* 512 bytes */
_Min_Stack_Size = 0x400;   /* 1 KB      */

MEMORY
{
  DTCM          (xrw) : ORIGIN = 0x20000000, LENGTH = 128K   /* primary CPU RAM         */
  AXI_SRAM      (xrw) : ORIGIN = 0x24000000, LENGTH = 320K   /* DMA-accessible SRAM     */

  FLASH_BOOT    (rx)  : ORIGIN = 0x08000000, LENGTH = 128K   /* bootloader  (sector 0)  */
  FLASH_OS      (rx)  : ORIGIN = 0x08020000, LENGTH = 384K   /* OS/kernel   (sectors 1-3) */
  FLASH_APP     (rx)  : ORIGIN = 0x08080000, LENGTH = 512K   /* application (sectors 4-7) */
}

/* Top of DTCM → initial stack pointer */
_estack = ORIGIN(DTCM) + LENGTH(DTCM);

/* -------------------------
   Output sections mapping
   ------------------------- */
SECTIONS
{
  /* ------------------------------------------------------------------ */
  /* BOOT area in FLASH_BOOT (vectors + boot code)                       */
  /* ------------------------------------------------------------------ */

  .isr_vector ORIGIN(FLASH_BOOT) :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    KEEP(*(.isr_vector.*))
    . = ALIGN(4);
  } > FLASH_BOOT

  .boot :
  {
    . = ALIGN(4);
    __boot_text_start = .;
    KEEP(*(.boot))
    __boot_text_end = .;
    . = ALIGN(4);
  } > FLASH_BOOT

  /* ------------------------------------------------------------------ */
  /* OS area in FLASH_OS                                                 */
  /* ------------------------------------------------------------------ */

  .os :
  {
    . = ALIGN(4);
    __os_text_start = .;
    KEEP(*(.os))
    __os_text_end = .;
    . = ALIGN(4);
  } > FLASH_OS

  /* Default code & rodata (untagged) → FLASH_OS */
  .text :
  {
    . = ALIGN(4);
    __text_start = .;
    KEEP(*(.text))
    KEEP(*(.text*))
    KEEP(*(.glue_7))
    KEEP(*(.glue_7t))
    KEEP(*(.eh_frame))
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    __text_end = .;
  } > FLASH_OS

  .rodata :
  {
    . = ALIGN(4);
    KEEP(*(.rodata))
    KEEP(*(.rodata*))
    . = ALIGN(4);
  } > FLASH_OS

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    KEEP(*(.ARM.extab* .gnu.linkonce.armextab.*))
    . = ALIGN(4);
  } > FLASH_OS

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    KEEP(*(.ARM.exidx*))
    __exidx_end = .;
    . = ALIGN(4);
  } > FLASH_OS

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } > FLASH_OS

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } > FLASH_OS

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } > FLASH_OS

  /* ------------------------------------------------------------------ */
  /* APP area in FLASH_APP                                               */
  /* ------------------------------------------------------------------ */

  .app :
  {
    . = ALIGN(4);
    __app_text_start = .;
    KEEP(*(.app))
    __app_text_end = .;
    . = ALIGN(4);
  } > FLASH_APP

  /* ------------------------------------------------------------------ */
  /* Initialized data sections (LMA in Flash, VMA in DTCM)              */
  /* ------------------------------------------------------------------ */

  /* Boot initialized data */
__boot_data_lma = LOADADDR(.data_boot);
.data_boot :
{
  . = ALIGN(4);
  _sdata_boot = .;
  KEEP(*(SORT(.boot_data)))
  . = ALIGN(4);
  _edata_boot = .;
} > DTCM AT > FLASH_BOOT

  /* OS initialized data */
__os_data_lma = LOADADDR(.data_os);
.data_os :
{
  . = ALIGN(4);
  _sdata_os = .;
  KEEP(*(SORT(.os_data)))
  . = ALIGN(4);
  _edata_os = .;
} > DTCM AT > FLASH_OS

  /* App initialized data */
__app_data_lma = LOADADDR(.data_app);
.data_app :
{
  . = ALIGN(4);
  _sdata_app = .;
  KEEP(*(SORT(.app_data)))
  . = ALIGN(4);
  _edata_app = .;
} > DTCM AT > FLASH_APP

  /* Generic untagged initialized data */
_sidata = LOADADDR(.data_generic);
.data_generic :
{
  . = ALIGN(4);
  _sdata = .;
  KEEP(*(.data))
  KEEP(*(.data*))
  . = ALIGN(4);
  _edata = .;
} > DTCM AT > FLASH_OS

  /* ------------------------------------------------------------------ */
  /* AXI SRAM — DMA-accessible buffers (opt-in via section attribute)   */
  /* ------------------------------------------------------------------ */
  .axi_data (NOLOAD) :
  {
    . = ALIGN(32);   /* align to cache line */
    __axi_data_start = .;
    KEEP(*(.axi_data))
    KEEP(*(.axi_data*))
    . = ALIGN(32);
    __axi_data_end = .;
  } > AXI_SRAM

  /* ------------------------------------------------------------------ */
  /* Uninitialized data (BSS) in DTCM                                   */
  /* ------------------------------------------------------------------ */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    KEEP(*(.bss))
    KEEP(*(.bss*))
    KEEP(*(COMMON))
    . = ALIGN(4);
    __bss_end__ = .;
  } > DTCM

  /* Heap and stack placement */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > DTCM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* -------------------------
   Exported symbols
   ------------------------- */

PROVIDE(__boot_text_start       = __boot_text_start);
PROVIDE(__boot_text_end         = __boot_text_end);
PROVIDE(__boot_data_lma         = __boot_data_lma);

PROVIDE(__os_text_start         = __os_text_start);
PROVIDE(__os_text_end           = __os_text_end);
PROVIDE(__os_data_lma           = __os_data_lma);

PROVIDE(__app_text_start        = __app_text_start);
PROVIDE(__app_text_end          = __app_text_end);
PROVIDE(__app_data_lma          = __app_data_lma);

PROVIDE(__text_start            = __text_start);
PROVIDE(__text_end              = __text_end);

PROVIDE(_sdata_boot             = _sdata_boot);
PROVIDE(_edata_boot             = _edata_boot);
PROVIDE(_sdata_os               = _sdata_os);
PROVIDE(_edata_os               = _edata_os);
PROVIDE(_sdata_app              = _sdata_app);
PROVIDE(_edata_app              = _edata_app);

PROVIDE(_sidata                 = _sidata);
PROVIDE(_sdata                  = _sdata);
PROVIDE(_edata                  = _edata);
PROVIDE(_sbss                   = __bss_start__);
PROVIDE(_ebss                   = __bss_end__);
