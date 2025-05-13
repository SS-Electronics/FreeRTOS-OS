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

#include "drv_time.h"




/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/

static uint32_t timer_master_counter	= 0u;
static uint32_t timer_slave_counter 	= 0u;
static uint32_t timer_native_counter 	= 0u;

#if(INC_DRIVER_TIMER == 1)
static drv_timer_handle_type local_drv_timer_handle;
#endif

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/

static void     co_master_drv_timer_init   (uint32_t freq);
static void     co_master_drv_timer_start  (void);
static uint8_t  co_master_drv_timer_update (void);
static uint32_t co_master_drv_timer_delay  (void);
static void     co_master_drv_timer_reload (uint32_t reload);
static void     co_master_drv_timer_stop   (void);

static void     co_slave_drv_timer_init   (uint32_t freq);
static void     co_slave_drv_timer_start  (void);
static uint8_t  co_slave_drv_timer_update (void);
static uint32_t co_slave_drv_timer_delay  (void);
static void     co_slave_drv_timer_reload (uint32_t reload);
static void     co_slave_drv_timer_stop   (void);

static void     co_native_drv_timer_init   (uint32_t freq);
static void     co_native_drv_timer_start  (void);
static uint8_t  co_native_drv_timer_update (void);
static uint32_t co_native_drv_timer_delay  (void);
static void     co_native_drv_timer_reload (uint32_t reload);
static void     co_native_drv_timer_stop   (void);

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
const CO_IF_TIMER_DRV co_timer_handle_1 = {
	co_master_drv_timer_init,
	co_master_drv_timer_reload,
	co_master_drv_timer_delay,
	co_master_drv_timer_stop,
	co_master_drv_timer_start,
	co_master_drv_timer_update
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
const CO_IF_TIMER_DRV co_timer_handle_2 = {
	co_slave_drv_timer_init,
	co_slave_drv_timer_reload,
	co_slave_drv_timer_delay,
	co_slave_drv_timer_stop,
	co_slave_drv_timer_start,
	co_slave_drv_timer_update
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
const CO_IF_TIMER_DRV co_timer_handle_3 = {
	co_native_drv_timer_init,
	co_native_drv_timer_reload,
	co_native_drv_timer_delay,
	co_native_drv_timer_stop,
	co_native_drv_timer_start,
	co_native_drv_timer_update
};

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
static void co_slave_drv_timer_init(uint32_t freq)
{
    (void)freq;
    timer_slave_counter = 0u;
}

static void co_slave_drv_timer_start(void)
{
}

static uint8_t co_slave_drv_timer_update(void)
{
    uint8_t result = 0u;

    if (timer_slave_counter > 0u) {
    	timer_slave_counter--;
        if (timer_slave_counter == 0u) {
            result = 1u;
        }
    }

    return (result);
}

static uint32_t co_slave_drv_timer_delay(void)
{
    return (timer_slave_counter);
}

static void co_slave_drv_timer_reload(uint32_t reload)
{
	timer_slave_counter = reload;
}

static void co_slave_drv_timer_stop(void)
{
	timer_slave_counter = 0u;
}



static void co_master_drv_timer_init(uint32_t freq)
{
    (void)freq;
    timer_master_counter = 0u;
}

static void co_master_drv_timer_start(void)
{
}

static uint8_t co_master_drv_timer_update(void)
{
    uint8_t result = 0u;

    if (timer_master_counter > 0u) {
    	timer_master_counter--;
        if (timer_master_counter == 0u) {
            result = 1u;
        }
    }

    return (result);
}

static uint32_t co_master_drv_timer_delay(void)
{
    return (timer_master_counter);
}

static void co_master_drv_timer_reload(uint32_t reload)
{
	timer_master_counter = reload;
}

static void co_master_drv_timer_stop(void)
{
	timer_master_counter = 0u;
}




static void co_native_drv_timer_init(uint32_t freq)
{
    (void)freq;
    timer_native_counter = 0u;
}

static void co_native_drv_timer_start(void)
{
}

static uint8_t co_native_drv_timer_update(void)
{
    uint8_t result = 0u;

    if (timer_native_counter > 0u) {
    	timer_native_counter--;
        if (timer_native_counter == 0u) {
            result = 1u;
        }
    }

    return (result);
}

static uint32_t co_native_drv_timer_delay(void)
{
    return (timer_native_counter);
}

static void co_native_drv_timer_reload(uint32_t reload)
{
	timer_native_counter = reload;
}

static void co_native_drv_timer_stop(void)
{
	timer_native_counter = 0u;
}


/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
uint32_t drv_time_get_ticks(void)
{
	return HAL_GetTick();
}

void drv_time_delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
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

#if(INC_DRIVER_TIMER == 1)

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_timer_handle_type * drv_timer_get_handle (void)
{
	return (&local_drv_timer_handle);
}

/*************************************************************
 * Func:
 * Desc:
 *
 * @parm:
 * @ret:
 *
 ************************************************************/
drv_status_type	 drv_timer_pwm_output(uint8_t timer_id, float duty_cycle, uint32_t driver_channel, uint8_t n_ch_ctrl_en)
{
	drv_status_type	status = KERNEL_OK;

#if defined(STM32F429xx) || defined (STM32H7A3xxQ) || defined (STM32H743xx)

	TIM_OC_InitTypeDef sConfigOC 						= {0};
	uint32_t value;

	if( (local_drv_timer_handle.timer_handle[timer_id] != NULL) &&\
		(timer_id < INVALID_TIMER_ID) &&\
		(local_drv_timer_handle.timer_type[timer_id] == PWM_OP) &&\
		(duty_cycle >= -100.0) &&\
		(duty_cycle <= 100.0)
	  )
	{
		if(duty_cycle != 0)
		{
#if defined(STM32F429xx)
			sConfigOC.OCMode 		= TIM_OCMODE_PWM1;
#endif

#if defined (STM32H7A3xxQ)
			sConfigOC.OCMode 		= TIM_OCMODE_ASYMMETRIC_PWM1;
#endif



			sConfigOC.OCPolarity 	= TIM_OCPOLARITY_HIGH;
			sConfigOC.OCNPolarity 	= TIM_OCPOLARITY_HIGH;
			sConfigOC.OCFastMode 	= TIM_OCFAST_DISABLE;
			sConfigOC.OCIdleState 	= TIM_OCNIDLESTATE_RESET;
			sConfigOC.OCNIdleState 	= TIM_OCNIDLESTATE_RESET;

			if(duty_cycle > 0)
			{

				value = (uint32_t)(TIMER_1_PWM_PERIOD_VAL*(duty_cycle/100.0));
				sConfigOC.Pulse			= (value - 1);

				status = HAL_TIM_PWM_ConfigChannel(local_drv_timer_handle.timer_handle[timer_id],
										  	  	  	  &sConfigOC,
													  driver_channel
										  	  	  	);
				if(n_ch_ctrl_en == FLAG_ENABLE)
				{
					status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
												  driver_channel
												  );
				}

				status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
														driver_channel
														);

				status = HAL_TIM_PWM_Start(local_drv_timer_handle.timer_handle[timer_id],
								  	  	  	  driver_channel
											);
			}
			else
			{
				value = (uint32_t)(TIMER_1_PWM_PERIOD_VAL*((0 - duty_cycle)/100.0));
				sConfigOC.Pulse			= (value - 1);

				status = HAL_TIM_PWM_ConfigChannel(local_drv_timer_handle.timer_handle[timer_id],
										  	  	  	  &sConfigOC,
													  driver_channel
										  	  	  	);

				status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
														driver_channel
														);
				if(n_ch_ctrl_en == FLAG_ENABLE)
				{
					status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
											  driver_channel
											  );


					status = HAL_TIMEx_PWMN_Start(local_drv_timer_handle.timer_handle[timer_id],
											  driver_channel
											  );
				}
			}
		}
		else
		{
			status |= HAL_TIM_PWM_Stop(local_drv_timer_handle.timer_handle[timer_id],
										driver_channel
										);
			if(n_ch_ctrl_en == FLAG_ENABLE)
			{
				status |= HAL_TIMEx_PWMN_Stop(local_drv_timer_handle.timer_handle[timer_id],
										  driver_channel
										  );
			}
		}

	}
	else
	{
		/* Invalid request */
		status = KERNEL_WRONG_REQ;
	}

#else
#error "Selected MCU for driver not present in IDE!"
#endif

	return status;
}


void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	int a = 0;
}


#endif






