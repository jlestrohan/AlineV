/*******************************************************************
 * MotorsControl_service.c
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include <FreeRTOS.h>
#include <stdlib.h>
#include "MotorsControl_service.h"
#include "gpio.h"
#include "tim.h"
#include "debug.h"
#include "MG90S_service.h"

static osThreadId_t MotorsControl_taskHandle;
static const osThreadAttr_t MotorsControlTa_attributes = {
		.name = "MotorsControlServiceTask",
		.stack_size = 256,
		.priority = (osPriority_t) osPriorityLow1, };

osEventFlagsId_t xEventMotorsForward;

MotorData_t MotorData = {MotorMotion_Stop, MotorMotion_Stop, 0,0};

/**
 * Motors Control main task
 * @param argument
 */
static void MotorsControlTask_Start(void *vParameters)
{

	/* prevent compilation warning */
	UNUSED(vParameters);

	for (;;)
	{
		/* event flag motors active, if set we start a motion, if not we idle */

		if (osEventFlagsGet(xEventMotorsForward) && MOTORS_FORWARD_ACTIVE) {
			MotorSetSpeed(&MotorData, 15, 15);
			motorSetMotionForward(&MotorData);
		} else {
			motorsSetMotorsIdle(&MotorData);
		}

		osDelay(50);
	}

	osThreadTerminate(MotorsControl_taskHandle);
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
uint8_t uMotorsControlServiceInit()
{
	xEventMotorsForward = osEventFlagsNew(NULL);
	if (xEventMotorsForward == NULL) {
		dbg_printf("Motors Event Flag Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

	/* initializes PWM duties to 0 for now (idle) */
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	/* creation of the MotorsControl_task */
	MotorsControl_taskHandle = osThreadNew(MotorsControlTask_Start, NULL, &MotorsControlTa_attributes);
	if (MotorsControl_taskHandle == NULL) {
		dbg_printf("MotorsControl Task Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionPace, uint8_t targetSpeed)
{

}

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionPace, uint8_t targetSpeed)
{

}

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param speed (0-100)
 */
void MotorSetSpeed(MotorData_t *data, uint8_t speed_left, uint8_t speed_right)
{
	/* let's clamp the values to avoid overflow */
	speed_left = speed_left > 100 ? 100 : speed_left;
	speed_right = speed_right > 100 ? 100 : speed_right;

	/* if speed <= 0 we deactivate the PWM timers to save energy, motor after motor */
	if (speed_left <= 0) {
		HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
	} else {
		HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
		htim16.Instance->CCR1 = speed_left;
	}

	if (speed_right <= 0)  {
		HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);
	} else {
		HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
		htim17.Instance->CCR1 = speed_right;
	}

	/* save the current values into the data struct */
	data->currentSpeedLeft = speed_left;
	data->currentSpeedRight = speed_right;
}

/**
 * Sets LN298 GPIO controls for forward motion
 */
void motorSetMotionForward(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	data->motorMotion_Left = MotorMotion_Forward;
	data->motorMotion_Right = MotorMotion_Forward;
}

/**
 * Sets LN298 GPIO controls for backward motion
 */
void motorSetMotionBackward(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	data->motorMotion_Left = MotorMotion_Backward;
	data->motorMotion_Right = MotorMotion_Backward;
}

/**
 * Sets LN298 GPIO controls for left turn motion
 */
void motorSetMotionTurnLeft(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	data->motorMotion_Left = MotorMotion_Backward;
	data->motorMotion_Right = MotorMotion_Forward;
}

/**
 * Sets LN298 GPIO controls for right turn motion
 */
void motorSetMotionTurnRight(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	data->motorMotion_Left = MotorMotion_Forward;
	data->motorMotion_Right = MotorMotion_Backward;
}

/**
 * Sets motors power to 0 but does not change the recorded motion
 */
void motorsSetMotorsIdle(MotorData_t *data)
{
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;
	data->currentSpeedLeft = 0;
	data->currentSpeedRight = 0;
}

/**
 * Completely stops the motors
 */
void motorsSetMotionStop(MotorData_t *data)
{
	motorsSetMotorsIdle(data);
	data->motorMotion_Left = MotorMotion_Stop;
	data->motorMotion_Right = MotorMotion_Stop;

}

