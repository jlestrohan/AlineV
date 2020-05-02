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
#include "freertos_logger_service.h"
#include "MG90S_service.h"

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t MotorsControl_taskHandle;
static osStaticThreadDef_t MotorsControlTaControlBlock;
static uint32_t MotorsControlTaBuffer[256];
static const osThreadAttr_t MotorsControlTa_attributes = {
		.name = "MotorsControlServiceTask",
		.stack_mem = &MotorsControlTaBuffer[0],
		.stack_size = sizeof(MotorsControlTaBuffer),
		.cb_mem = &MotorsControlTaControlBlock,
		.cb_size = sizeof(MotorsControlTaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };

osEventFlagsId_t xEventMotorsForward;

MotorData_t MotorData = {Motor_Stop, Motor_Stop, 0,0};

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
		if (osEventFlagsGet(xEventMotorsForward) && MOTORS_FORWARD_ACTIVE) {
			MotorSetSpeed(&MotorData, 15, 15);
			motorSetForward();
		} else {
			motorsSetIdle();
		}
		osDelay(50);
	}

	osThreadSuspend(NULL);
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
MOTORS_Result_t MotorsControl_Service_Initialize()
{

	xEventMotorsForward = osEventFlagsNew(NULL);
	if (xEventMotorsForward == NULL) {
		loggerE("Motors Event Flag Initialization Failed");
		return (EXIT_FAILURE);
	}

	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

	/* initializes PWM duties to 0 for now (idle) */
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	/* creation of the MotorsControl_task */
	MotorsControl_taskHandle = osThreadNew(MotorsControlTask_Start, NULL, &MotorsControlTa_attributes);
	if (!MotorsControl_taskHandle) {
		loggerE("MotorsControl Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionchange)
{

}

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionchange)
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
void motorSetForward()
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* activates front servo */
	osEventFlagsSet(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
}

/**
 * Sets LN298 GPIO controls for backward motion
 */
void motorSetBackward()
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* deactivate front servo */
	osEventFlagsClear(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
}

/**
 * Sets LN298 GPIO controls for left turn motion
 */
void motorSetTurnLeft()
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);
}

/**
 * Sets LN298 GPIO controls for right turn motion
 */
void motorSetTurnRight()
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);
}

/**
 * Sets motors power to 0
 */
void motorsSetIdle()
{
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	/* deactivate front servo */
	osEventFlagsClear(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
}
