/* USER CODE BEGIN Header */
/*
 * FreeRTOS Kernel configuration for FreeRTOS-OS
 *
 * Values are driven by Kconfig via autoconf.h.
 * Edit settings with:  make menuconfig
 * Regenerate headers:  make config-outputs
 *
 * Original template: Amazon.com / STMicroelectronics
 * Adapted for Kconfig integration: Subhajit Roy
 */
/* USER CODE END Header */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Pull in every CONFIG_* symbol produced by "make config-outputs" */
#include "autoconf.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure definitions are only used by the compiler, not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif

/* CMSIS device header selection — driven by TARGET_MCU */
#ifndef CMSIS_device_header
  #if defined(STM32F411xE) || defined(STM32F405xx) || defined(STM32F407xx) || \
      defined(STM32F429xx) || defined(STM32F401xE)
    #define CMSIS_device_header "stm32f4xx.h"
  #endif
#endif

/* ── STM32F411xE memory map (mirrors stm32_f411_FLASH.ld) ────────────────── */
/*
 * Flash  512 K total
 *   FLASH_BOOT  0x0800_0000  32 K   sectors 0–1  (bootloader)
 *   FLASH_OS    0x0800_8000 224 K   sectors 2–5  (OS + kernel + drivers)
 *   FLASH_APP   0x0804_0000 256 K   sector  6–7  (application)
 *
 * RAM    128 K total  (0x2000_0000 – 0x2001_FFFF)
 *   FreeRTOS heap   80 000 B   (~78 K)   heap_4 static array in .bss
 *   .data / .bss    ~  49 KB   globals, handles, ring buffers
 *   MSP stack        1 024 B   linker _Min_Stack_Size (grows downward from _estack)
 *   Newlib heap        512 B   linker _Min_Heap_Size  (sbrk region, not RTOS heap)
 */
#define configFLASH_BOOT_ORIGIN     0x08000000UL
#define configFLASH_BOOT_SIZE_BYTES (32UL  * 1024UL)   /*  32 K */
#define configFLASH_OS_ORIGIN       0x08008000UL
#define configFLASH_OS_SIZE_BYTES   (224UL * 1024UL)   /* 224 K */
#define configFLASH_APP_ORIGIN      0x08040000UL
#define configFLASH_APP_SIZE_BYTES  (256UL * 1024UL)   /* 256 K */
#define configRAM_ORIGIN            0x20000000UL
#define configRAM_SIZE_BYTES        (128UL * 1024UL)   /* 128 K */

/* Minimum MSP stack reserved by the linker (_Min_Stack_Size in .ld) */
#define configMSP_STACK_MIN_BYTES   (1024UL)
/* Minimum newlib heap reserved by the linker (_Min_Heap_Size in .ld) */
#define configNEWLIB_HEAP_MIN_BYTES (512UL)

/* FPU / MPU — Cortex-M4F has FPU, no MPU on most F4 variants */
#define configENABLE_FPU    1
#define configENABLE_MPU    0

/* ── Scheduler behaviour ──────────────────────────────────────────────────── */
#ifdef CONFIG_RTOS_USE_PREEMPTION
  #define configUSE_PREEMPTION              1
#else
  #define configUSE_PREEMPTION              1
#endif

#ifdef CONFIG_RTOS_USE_TIME_SLICING
  #define configUSE_TIME_SLICING            1
#else
  #define configUSE_TIME_SLICING            1
#endif

#define configSUPPORT_STATIC_ALLOCATION     1
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configUSE_IDLE_HOOK                 1
#define configUSE_TICK_HOOK                 0
#define configCHECK_FOR_STACK_OVERFLOW      2
#define configUSE_MALLOC_FAILED_HOOK        1

/* ── Clock & tick ─────────────────────────────────────────────────────────── */
#define configCPU_CLOCK_HZ                  (SystemCoreClock)
#define configTICK_RATE_HZ                  ((TickType_t)CONFIG_RTOS_TICK_RATE_HZ)

/* ── Task parameters ──────────────────────────────────────────────────────── */
#define configMAX_PRIORITIES                (CONFIG_RTOS_MAX_PRIORITIES)

/* Idle-task / minimum stack in WORDS (×4 = bytes on Cortex-M4).
 * CONFIG_RTOS_MINIMAL_STACK_SIZE = 512 words = 2048 bytes. */
#define configMINIMAL_STACK_SIZE            ((uint16_t)CONFIG_RTOS_MINIMAL_STACK_SIZE)

/* FreeRTOS heap (heap_4 static array in .bss).
 * Budget: 128K RAM − 80000 B heap − 1024 B MSP − 512 B newlib ≈ 49 KB for .data/.bss.
 * CONFIG_RTOS_TOTAL_HEAP_SIZE = 80000 bytes (~78 K). */
#define configTOTAL_HEAP_SIZE               ((size_t)CONFIG_RTOS_TOTAL_HEAP_SIZE)

/* Heap is the static array inside heap_4.c — application does not provide it. */
#define configAPPLICATION_ALLOCATED_HEAP    0

#define configMAX_TASK_NAME_LEN             (CONFIG_RTOS_MAX_TASK_NAME_LEN)

/* ── Kernel features ──────────────────────────────────────────────────────── */
#define configUSE_TRACE_FACILITY            1
#define configUSE_16_BIT_TICKS              0
/* Record the top-of-stack address in the TCB so uxTaskGetStackHighWaterMark()
 * can accurately report remaining stack on Cortex-M4. */
#define configRECORD_STACK_HIGH_ADDRESS     1

#ifdef CONFIG_RTOS_USE_MUTEXES
  #define configUSE_MUTEXES                 1
#else
  #define configUSE_MUTEXES                 0
#endif

#define configQUEUE_REGISTRY_SIZE           CONFIG_RTOS_QUEUE_REGISTRY_SIZE

#ifdef CONFIG_RTOS_USE_RECURSIVE_MUTEXES
  #define configUSE_RECURSIVE_MUTEXES       1
#else
  #define configUSE_RECURSIVE_MUTEXES       0
#endif

#ifdef CONFIG_RTOS_USE_COUNTING_SEMAPHORES
  #define configUSE_COUNTING_SEMAPHORES     1
#else
  #define configUSE_COUNTING_SEMAPHORES     0
#endif

#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t

/* ── Co-routines (unused) ─────────────────────────────────────────────────── */
#define configUSE_CO_ROUTINES               0
#define configMAX_CO_ROUTINE_PRIORITIES     (2)

/* ── Software timers ──────────────────────────────────────────────────────── */
#ifdef CONFIG_RTOS_USE_TIMERS
  #define configUSE_TIMERS                  1
#else
  #define configUSE_TIMERS                  0
#endif
#define configTIMER_TASK_PRIORITY           (CONFIG_RTOS_TIMER_TASK_PRIORITY)
#define configTIMER_QUEUE_LENGTH            CONFIG_RTOS_TIMER_QUEUE_LENGTH
#define configTIMER_TASK_STACK_DEPTH        CONFIG_RTOS_TIMER_TASK_STACK_DEPTH

/* newlib reentrancy (required when using newlib-nano) */
#define configUSE_NEWLIB_REENTRANT          1

/* ── Optional API functions ───────────────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTimerPendFunctionCall      1
#define INCLUDE_xQueueGetMutexHolder        1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_eTaskGetState               1

/* Heap implementation selection */
#define USE_FreeRTOS_HEAP_4

/* ── FreeRTOS+CLI ─────────────────────────────────────────────────────────── */
/* Size of the internal static output buffer inside FreeRTOS_CLI.c.
 * Must be >= the pcWriteBuffer length passed to FreeRTOS_CLIProcessCommand(). */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE       512

/* ── Cortex-M4 interrupt priority configuration ───────────────────────────── */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS    __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS    4   /* STM32F4xx: 4-bit priority field (0..15) */
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY       \
        CONFIG_RTOS_LIBRARY_LOWEST_IRQ_PRIORITY

#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  \
        CONFIG_RTOS_LIBRARY_MAX_SYSCALL_IRQ_PRIORITY

/* Kernel interrupt priorities (shifted into the NVIC register format) */
#define configKERNEL_INTERRUPT_PRIORITY    \
        (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
        (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ── Assert ───────────────────────────────────────────────────────────────── */
/* USER CODE BEGIN 1 */
#define configASSERT(x)  if ((x) == 0) { taskDISABLE_INTERRUPTS(); for (;;); }
/* USER CODE END 1 */

/* ── Port handler name aliases ────────────────────────────────────────────── */
#define vPortSVCHandler      SVC_Handler
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION  0

/* USER CODE BEGIN Defines */
/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
