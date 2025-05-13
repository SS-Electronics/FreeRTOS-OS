/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS Basic Software
#
# RTOS Basic Software is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS Basic Software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#include "drv_cpu.h"

static drv_wdg_handle_type	wdg_handle;
static uint8_t 				temp_byte_buff;

// Debug Exception Monitor and Control register
#define DEMCR   (*((unsigned long *) 0xe000edfc))
#define TRCENA  0x01000000  // enable trace

// Stimulus Port registers
#define ITM_STIM0 (*((unsigned long *) 0xe0000000))
#define ITM_STIM1 (*((unsigned long *) 0xe0000004))
#define ITM_STIM2 (*((unsigned long *) 0xe0000008))
#define ITM_STIM3 (*((unsigned long *) 0xe000000c))

// Trace enable registers
#define ITM_TER   (*((unsigned long *) 0xe0000e00))

// Privilege register: registers that can be accessed by unprivileged code
#define ITM_TPR   (*((unsigned long *) 0xe0000e40))

// Trace Control register
#define ITM_TCR   (*((unsigned long *) 0xe0000e80))
// Lock Access register
#define ITM_LAR   (*((unsigned long *) 0xe0000fb0))

// unlock value
#define ITM_LAR_ACCESS  0xc5acce55

#define ITM_Port8(n)    (*((volatile unsigned char *)(0xe0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xe0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xe0000000+4*n)))

#define ITM_TER   (*((unsigned long *) 0xe0000e00))
#define ITM_TCR   (*((unsigned long *) 0xe0000e80))

#define TRCENA          0x01000000


int32_t ITM_SendChar_1 (int32_t ch) {
    if ((ITM_TCR & (1UL << 0)) &&         /* ITM enabled */
        (DEMCR & TRCENA) &&
        (ITM_TER & (1UL << 0)))
    {
        while (ITM_Port32(0) == 0);
        ITM_Port8(0) = (uint8_t)ch;
    }
    return (ch);
}


drv_wdg_handle_type* 	drv_wdg_get_handle(void)
{
	return &wdg_handle;
}

drv_status_type drv_wdg_init(void)
{
	drv_status_type drv_stat = DRV_ERR;
	if(HAL_IWDG_Init(wdg_handle.handle) == HAL_OK)
	{
		drv_stat = DRV_OK;
	}

	return drv_stat;
}

void drv_wdg_refresh(void)
{
	HAL_IWDG_Refresh(wdg_handle.handle);
}


void drv_cpu_SVC_prio_set(void)
{
#if (__ARM_ARCH_7A__ == 0U)
	/* Service Call interrupt might be configured before kernel start     */
	/* and when its priority is lower or equal to BASEPRI, svc intruction */
	/* causes a Hard Fault.                                               */
	NVIC_SetPriority (SVCall_IRQn, 0U);
#endif
}

drv_status_type drv_itm_init(void)
{
	drv_status_type drv_status = DRV_OK;
    ITM_LAR = ITM_LAR_ACCESS; // unlock
    ITM_TCR = 0x1;            // global enable for ITM
    ITM_TPR = 0x1;            // first 8 stim registers have unpriv access
    ITM_TER = 0xf;            // enable 4 stim ports
    DEMCR = TRCENA;           // global enable DWT and ITM
	return drv_status;
}

int drv_itm_print(uint8_t *ptr)
{
	ATOMIC_ENTER_CRITICAL();
	int DataIdx;
	int len = strlen((char*)ptr);

		for (DataIdx = 0; DataIdx < len; DataIdx++)
		{
			temp_byte_buff = ptr[DataIdx];
			ITM_SendChar_1(temp_byte_buff);
		}

	memset(ptr,0,ITM_PRINT_BUFF_LENGTH);
	ATOMIC_EXIT_CRITICAL();
	return len;
}


void reset_mcu(void)
{
    /* Disable interrupts */
       NVIC_SystemReset();
}


/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
#ifdef SERIAL_DEBUG_COMM
	//drv_serial_transmit(SERIAL_DEBUG_COMM, (uint8_t*)"[ FLT ] Hardware fault!\n\r",25);
#else
#warning "Error handler print disabled! As no serial_com_bus selected!!"
#endif
	__disable_irq();
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
#ifdef SERIAL_DEBUG_COMM
	//drv_serial_transmit(SERIAL_DEBUG_COMM, (uint8_t*)"[ FLT ] Memory mgmt!\n\r",21);
#else
#warning "Error handler print disabled! As no serial_com_bus selected!!"
#endif
	__disable_irq();
  while (1)
  {
	  /* Error Handling*/
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
#ifdef SERIAL_DEBUG_COMM
	//drv_serial_transmit(SERIAL_DEBUG_COMM, (uint8_t*)"[ FLT ] Bus fault!\n\r",19);
#else
#warning "Error handler print disabled! As no serial_com_bus selected!!"
#endif
	__disable_irq();
  while (1)
  {
	  /* Error Handling*/
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
#ifdef SERIAL_DEBUG_COMM
	//drv_serial_transmit(SERIAL_DEBUG_COMM, (uint8_t*)"[ FLT ] illegal instruction!\n\r",27);
#else
#warning "Error handler print disabled! As no serial_com_bus selected!!"
#endif
	__disable_irq();
  while (1)
  {
	  /* Error Handling*/
  }
}
