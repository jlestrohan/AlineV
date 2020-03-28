/**
 ******************************************************************************
 * @file    buzzer_service.c
 * @author  Jack Lestrohan
 * @brief   Buzzer module service file
 ******************************************************************************
 * @attention
 *				Connect Buzzer as follow (for this present project)
 *
 *					PD2	 	-> 	BUZZER SIG
 *
 ******************************************************************************
 */

#include "buzzer_service.h"
#include "stdint.h"
#include "tim.h"

void setPWM(TIM_HandleTypeDef timer, uint32_t channel, uint16_t period, uint16_t pulse)
{
	HAL_TIM_PWM_Stop(&timer, channel);
	// stop generation of pwm
	TIM_OC_InitTypeDef sConfigOC;
	timer.Init.Period = period;
	// set the period duration
	HAL_TIM_PWM_Init(&timer);
	// reinititialise with new period value
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = pulse;
	// set the pulse duration
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, channel);
	HAL_TIM_PWM_Start(&timer, channel);   // start pwm generation}
}

void buzzerService_initialize()
{
	for (int i = 0; i < 256; i++) {
		setPWM(htim2, TIM_CHANNEL_1, 50, i);
		HAL_Delay(10);
	}
}
