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
#define configMINIMAL_STACK_SIZE            ((uint16_t)CONFIG_RTOS_MINIMAL_STACK_SIZE)
#define configTOTAL_HEAP_SIZE               ((size_t)CONFIG_RTOS_TOTAL_HEAP_SIZE)
#define configMAX_TASK_NAME_LEN             (CONFIG_RTOS_MAX_TASK_NAME_LEN)

/* ── Kernel features ──────────────────────────────────────────────────────── */
#define configUSE_TRACE_FACILITY            1
#define configUSE_16_BIT_TICKS              0

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
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION  0

/* USER CODE BEGIN Defines */
/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
