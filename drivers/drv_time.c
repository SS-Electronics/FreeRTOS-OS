/*
# Copyright (C) 2024 Subhajit Roy
# This file is part of RTOS OS project
#
# RTOS OS project is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTOS OS project is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
*/

#include <drivers/drv_time.h>

/* *****************************************************
 *
 *
 *
 * *****************************************************/

static type_drv_hw_handle 		*timer_handle_ref;



/* *****************************************************
 *
 *
 *
 * *****************************************************/
uint32_t drv_time_get_ticks(void)
{
	return HAL_GetTick();
}

/* *****************************************************
 *
 *
 *
 * *****************************************************/
void drv_time_delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}


#if (NO_OF_TIMER > 0)
/* *****************************************************
 *
 *
 *
 * *****************************************************/
void	drv_timer_update_handle(__TYPE_HW_TIMER_HANDLE_TYPE * handle, uint8_t dev_id)
{
	if( (handle != NULL) && (dev_id < NO_OF_TIMER) )
	{
		timer_handle_ref->handle_hw_timer[dev_id] = handle;
	}
}



/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
uint32_t	drv_get_tim_3_encoder_ticks(void)
{
#if defined(STM32H7A3xxQ) || defined(STM32H743xx) || defined(STM32F429xx)
	return TIM3->CNT;
#else
			return 0;
#endif

}

void drv_set_tim_3_encoder_ticks(uint32_t tim_val)
{
#if defined(STM32H7A3xxQ) || defined(STM32H743xx) || defined(STM32F429xx)
	TIM3->CNT = tim_val;
#else

#endif
}


uint32_t	drv_get_tim_2_encoder_ticks(void)
{
#if defined(STM32L431xx) || defined(STM32H743xx) || defined(STM32F429xx)
	return TIM2->CNT;
#else
			return 0;
#endif

}

//#if(INC_DRIVER_TIMER == 1)
//
///*************************************************************
// * Func:
// * Desc:
// *
// * @parm:
// * @ret:
// *
// ************************************************************/
//drv_timer_handle_type * drv_timer_get_handle (void)
//{
//	return (&local_drv_timer_handle);
//}
//
///*************************************************************
// * Func:
// * Desc:
// *
// * @parm:
// * @ret:
// *
// ************************************************************/
//drv_status_type	 drv_timer_pwm_output(uint8_t timer_id, float duty_cycle, uint32_t driver_channel, uint8_t n_ch_ctrl_en)
//{
//	drv_status_type	status = KERNEL_OK;
//
//#if defined(STM32F429xx) || defined (STM32H7A3xxQ) || defined (STM32H743xx)
//
//	TIM_OC_InitTypeDef sConfigOC 						= {0};
//	uint32_t value;
//
//	if( (local_drv_timer_handle.timer_handle[timer_id] != NULL) &&\
//		(timer_id < INVALID_TIMER_ID) &&\
//		(local_drv_timer_handle.timer_type[timer_id] == PWM_OP) &&\
//		(duty_cycle >= -100.0) &&\
//		(duty_cycle <= 100.0)
//	  )
//	{
//		if(duty_cycle != 0)
//		{
//#if defined(STM32F429xx)
//			sConfigOC.OCMode 		= TIM_OCMODE_PWM1;
//#endif
//
//#if defined (STM32H7A3xxQ)
//			sConfigOC.OCMode 		= TIM_OCMODE_ASYMMETRIC_PWM1;
//#endif
//
//
//
//			sConfigOC.OCPolarity 	= TIM_OCPOLARITY_HIGH;
//			sConfigOC.OCNPolarity 	= TIM_OCPOLARITY_HIGH;
//			sConfigOC.OCFastMode 	= TIM_OCFAST_DISABLE;
//			sConfigOC.OCIdleState 	= TIM_OCNIDLESTATE_RESET;
//			sConfigOC.OCNIdleState 	= TIM_OCNIDLESTATE_RESET;
//
//			if(duty_cycle > 0)
//			{
//
//				value = (uint32_t)(TIMER_1_PWM_PERIOD_VAL*(duty_cycle/100.0));
//				sConfigOC.Pulse			= (value - 1);
//
//				status = HAL_TIM_PWM_ConfigChannel(local_drv_timer_handle.timer_handle[timer_id],
//										  	  	  	  &sConfigOC,
//													  driver_channel
//										  	  	  	);
//				if(n_ch_ctrl_en == FLAG_ENABLE)
//				{
//					status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
//												  driver_channel
//												  );
//				}
//
//				status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
//														driver_channel
//														);
//
//				status = HAL_TIM_PWM_Start(local_drv_timer_handle.timer_handle[timer_id],
//								  	  	  	  driver_channel
//											);
//			}
//			else
//			{
//				value = (uint32_t)(TIMER_1_PWM_PERIOD_VAL*((0 - duty_cycle)/100.0));
//				sConfigOC.Pulse			= (value - 1);
//
//				status = HAL_TIM_PWM_ConfigChannel(local_drv_timer_handle.timer_handle[timer_id],
//										  	  	  	  &sConfigOC,
//													  driver_channel
//										  	  	  	);
//
//				status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
//														driver_channel
//														);
//				if(n_ch_ctrl_en == FLAG_ENABLE)
//				{
//					status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
//											  driver_channel
//											  );
//
//
//					status = HAL_TIMEx_PWMN_Start(local_drv_timer_handle.timer_handle[timer_id],
//											  driver_channel
//											  );
//				}
//			}
//		}
//		else
//		{
//			status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
//										driver_channel
//										);
//			if(n_ch_ctrl_en == FLAG_ENABLE)
//			{
//				status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
//										  driver_channel
//										  );
//			}
//		}
//
//	}
//	else
//	{
//		/* Invalid request */
//		status = KERNEL_WRONG_REQ;
//	}
//
//#else
//#error "Selected MCU for driver not present in IDE!"
//#endif
//
//	return status;
//}
//
//
//void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
//{
//	int a = 0;
//}
//
//
#endif






