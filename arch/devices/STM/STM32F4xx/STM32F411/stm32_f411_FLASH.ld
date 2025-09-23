/*
File:        stm32_f411_flash.ld
Author:      Subhajit Roy  
             subhajitroy005@gmail.com 

Moudle:       linker script
Info:         Liker script need to be modified with boot section + os + Application              
Dependency:   None

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
along with FreeRTOS-KERNEL. If not, see <https://www.gnu.org/licenses/>. */

/*****************************************************************************
*  Target      : STM32F411VETx
*
*  Datasheet   : https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf 
*       
*  Memory      : Flash 512 kb [ 32 kB boot ] [ 224 kb OS ] [ 352 kb app ]
               : RAM   128 kb     
*  Flash sectors
Sector 0:  0x0800_0000 – 0x0800_3FFF  (16 KB)
Sector 1:  0x0800_4000 – 0x0800_7FFF  (16 KB)
Sector 2:  0x0800_8000 – 0x0800_BFFF  (16 KB)
Sector 3:  0x0800_C000 – 0x0800_FFFF  (16 KB)
Sector 4:  0x08010000 – 0x0801_FFFF  (64 KB)
Sector 5:  0x08020000 – 0x0803_FFFF  (128 KB)
Sector 6:  0x08040000 – 0x0807_FFFF  (256 KB)
****************************************************************************/

ENTRY(Reset_Handler)

MINIMUM_MEMORY_MODEL = 1;



/* tune these sizes to your needs */
_Min_Heap_Size  = 0x200;   /* 512 bytes - adjust */
_Min_Stack_Size = 0x400;   /* 1 KB    - adjust */

MEMORY
{
  RAM           (xrw) : ORIGIN = 0x20000000, LENGTH = 128K

  FLASH_BOOT    (rx)  : ORIGIN = 0x08000000, LENGTH = 32K    /* bootloader (sectors 0..1)  */
  FLASH_OS      (rx)  : ORIGIN = 0x08008000, LENGTH = 224K   /* OS/kernel (sectors 2..5)   */
  FLASH_APP     (rx)  : ORIGIN = 0x08040000, LENGTH = 256K   /* application (sectors 6..7) */
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* end of "RAM" Ram type memory */

/* -------------------------
   Output sections mapping
   ------------------------- */
SECTIONS
{
  /* ------------------------------------------------------------------ */
  /* BOOT area in FLASH_BOOT (vectors + boot code + boot data LMA)      */
  /* ------------------------------------------------------------------ */

  .isr_vector ORIGIN(FLASH_BOOT) :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))              /* typical startup vector for boot image */
    KEEP(*(.isr_vector.*))
    . = ALIGN(4);
  } > FLASH_BOOT

  /* Bootloader code: functions explicitly placed in .boot */
  .boot :
  {
    . = ALIGN(4);
    __boot_text_start = .;
    KEEP(*(.boot))        /* code placed with __attribute__((section(".boot"))) */
    KEEP(*(.boot*))       /* wildcard catch */
    __boot_text_end = .;
    . = ALIGN(4);
  } > FLASH_BOOT

  /* ------------------------------------------------------------------ */
  /* OS  area in FLASH_OS                                               */
  /* ------------------------------------------------------------------ */

  .os :
  {
    . = ALIGN(4);
    __os_text_start = .;
    KEEP(*(.os))          /* code placed with __attribute__((section(".os"))) */
    KEEP(*(.os*))
    __os_text_end = .;
    . = ALIGN(4);
  } > FLASH_OS

  /* ------------------------------------------------------------------ */
  /* Default code & rodata (untagged) - place into FLASH_OS by default   */
  /* ------------------------------------------------------------------ */

  .text :
  {
    . = ALIGN(4);
    __text_start = .;
    KEEP(*(.text))        /* all normal .text ends up here if not explicitly sectioned */
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

  /* Exception tables (if any) */
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

.preinit_array (READONLY) : /* The "READONLY" keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } > FLASH_OS

  .init_array (READONLY) : /* The "READONLY" keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } > FLASH_OS

  .fini_array (READONLY) : /* The "READONLY" keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
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

  .app (NOLOAD) :
  {
    . = ALIGN(4);
    __app_text_start = .;
    KEEP(*(.app))         /* code placed with __attribute__((section(".app"))) */
    KEEP(*(.app*))
    __app_text_end = .;
    . = ALIGN(4);
  } > FLASH_APP



  /* ------------------------------------------------------------------   */
  /*                    BOOT RAM + LMA                                    */
  /* Initialized data sections: runtime in RAM, load image in respective FLASH */
  /* We create separate RAM sections that load from the matching FLASH LMA   */
  /* ------------------------------------------------------------------   */

  /* Bootloader RAM section */
.data_boot (NOLOAD) :
{
  . = ALIGN(4);
  _sdata_boot = .;
  KEEP(*(SORT(.boot_data)))   /* all bootloader variables */
  . = ALIGN(4);
  _edata_boot = .;
} > RAM AT > FLASH_BOOT

/* Bootloader LMA in Flash */
.boot_data_lma :
{
  . = ALIGN(4);
  __boot_data_lma = .;
  KEEP(*(SORT(.boot_data)))
  . = ALIGN(4);
  __boot_data_lma_end = .;
} > FLASH_BOOT

  /* ------------------------------------------------------------------   */
  /*                    OS RAM + LMA                                    */
  /* Initialized data sections: runtime in RAM, load image in respective FLASH */
  /* We create separate RAM sections that load from the matching FLASH LMA   */
  /* ------------------------------------------------------------------   */

  /* OS RAM section */
.data_os (NOLOAD) :
{
  . = ALIGN(4);
  _sdata_os = .;
  KEEP(*(SORT(.os_data)))    /* all OS variables */
  . = ALIGN(4);
  _edata_os = .;
} > RAM AT > FLASH_OS

/* OS LMA in Flash */
.os_data_lma :
{
  . = ALIGN(4);
  __os_data_lma = .;
  KEEP(*(SORT(.os_data)))
  . = ALIGN(4);
  __os_data_lma_end = .;
} > FLASH_OS

  /* ------------------------------------------------------------------   */
  /*                    APP RAM + LMA                                    */
  /* Initialized data sections: runtime in RAM, load image in respective FLASH */
  /* We create separate RAM sections that load from the matching FLASH LMA   */
  /* ------------------------------------------------------------------   */

/* App RAM section */
.data_app (NOLOAD) :
{
  . = ALIGN(4);
  _sdata_app = .;
  KEEP(*(SORT(.app_data)))   /* all App variables */
  . = ALIGN(4);
  _edata_app = .;
} > RAM AT > FLASH_APP

/* App LMA in Flash */
.app_data_lma :
{
  . = ALIGN(4);
  __app_data_lma = .;
  KEEP(*(SORT(.app_data)))
  . = ALIGN(4);
  __app_data_lma_end = .;
} > FLASH_APP

/* generic untagged data */
_sidata = LOADADDR(.data_generic);
.data_generic :
{
  . = ALIGN(4);
  _sdata = .;
  KEEP(*(.data))
  KEEP(*(.data*))
  . = ALIGN(4);
  _edata = .;
} > RAM AT > FLASH_OS


  /* ------------------------------------------------------------------ */
  /* Uninitialized data (BSS) */
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
  } > RAM

  /* Heap and stack placement */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* Discard libc/libm parts which we don't want linked in place */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  /* attributes */
  .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* -------------------------
   Export useful symbols
   ------------------------- */

/* Boot code/text area in FLASH_BOOT */
PROVIDE(__boot_text_start       = __boot_text_start);
PROVIDE(__boot_text_end         = __boot_text_end);
PROVIDE(__boot_data_lma         = __boot_data_lma);
PROVIDE(__boot_data_lma_end     = __boot_data_lma_end);

/* OS area in FLASH_OS */
PROVIDE(__os_text_start         = __os_text_start);
PROVIDE(__os_text_end           = __os_text_end);
PROVIDE(__os_data_lma           = __os_data_lma);
PROVIDE(__os_data_lma_end       = __os_data_lma_end);

/* APP area in FLASH_APP */
PROVIDE(__app_text_start        = __app_text_start);
PROVIDE(__app_text_end          = __app_text_end);
PROVIDE(__app_data_lma          = __app_data_lma);
PROVIDE(__app_data_lma_end      = __app_data_lma_end);

/* Generic .text/.data symbols */
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