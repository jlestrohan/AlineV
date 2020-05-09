/*******************************************************************
 * MotorsControl_service.c
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include <FreeRTOS.h>
#include "configuration.h"
#include <stdlib.h>
#include "MotorsControl_service.h"
#include "gpio.h"
#include "tim.h"
#include "printf.h"
#include "main.h"
#include "MG90S_service.h"
#include <assert.h>

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xMotorsControlTaskHnd;
static uint32_t xMotorsControlTaBuffer[256];
static osStaticThreadDef_t xMotorsControlTaControlBlock;
static const osThreadAttr_t xMotorsControlTa_attributes = {
		.name = "MotorsControlServiceTask",
		.priority = (osPriority_t) osPriorityLow1,
		.stack_mem = &xMotorsControlTaBuffer[0],
		.stack_size = sizeof(xMotorsControlTaBuffer),
		.cb_mem = &xMotorsControlTaControlBlock,
		.cb_size = sizeof(xMotorsControlTaControlBlock),
};

osMessageQueueId_t xQueueMotorMotionOrder;

/**
 * Main DataStruct accessible from everywhere
 */
extern MotorData_t MotorData;
MotorData_t MotorData, lastMotorData = {MOTOR_MOTION_IDLE, MOTOR_MOTION_IDLE, 0,0};

/* functions definitions */
static void motorsSetMotionIdle(MotorData_t *data);
static void motorSetMotionForward(MotorData_t *data);
static void motorSetMotionBackward(MotorData_t *data);
static void MotorSetSpeed(MotorData_t *data, uint8_t speed_left, uint8_t speed_right);
static void motorSetMotionTurnRight(MotorData_t *data);
static void motorSetMotionTurnLeft(MotorData_t *data);


/**
 * Motors Control main task
 * @param argument
 */
static void vMotorsControlTaskStart(void *vParameters)
{
	MotorMotion_t msgOrder;

	for (;;)
	{
		/* event flag motors active - these flags are just disposable events flags */
		osMessageQueueGet(xQueueMotorMotionOrder, &msgOrder, NULL, osWaitForever);

		switch (msgOrder) {
		case MOTOR_MOTION_FORWARD:
			motorSetMotionForward(&MotorData);
			break;

		case MOTOR_MOTION_BACKWARD:
			motorSetMotionBackward(&MotorData);
			break;

		case MOTOR_MOTION_IDLE: default:
			motorsSetMotionIdle(&MotorData);
			break;
		}

		osDelay(10);
	}
	osThreadTerminate(xMotorsControlTaskHnd);
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
uint8_t uMotorsControlServiceInit()
{
	/* start Idle for now */
	xQueueMotorMotionOrder = MOTOR_MOTION_IDLE;

	xQueueMotorMotionOrder = osMessageQueueNew(10, sizeof(uint8_t), NULL);
	if (xQueueMotorMotionOrder == NULL) {
		printf("Motors Message Queue Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

	/* initializes PWM duties to 0 for now (idle) */
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	/* creation of the MotorsControl_task */
	xMotorsControlTaskHnd = osThreadNew(vMotorsControlTaskStart, NULL, &xMotorsControlTa_attributes);
	if (xMotorsControlTaskHnd == NULL) {
		printf("MotorsControl Task Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Motors Control Service... Success!\n\r");
	return (EXIT_SUCCESS);
}

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param speed (0-100)
 */
static void MotorSetSpeed(MotorData_t *data, uint8_t speed_left, uint8_t speed_right)
{
	assert(speed_right > -1);
	assert(speed_left > -1);

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
	data->currentSpeedLeft = speed_left;							/* records the current speed */
	data->currentSpeedRight = speed_right;

}

/**
 * Sets LN298 GPIO controls for forward motion
 */
static void motorSetMotionForward(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	lastMotorData = *data; /* backup the data into the last known values struct */
	data->motorMotion_Left = MOTOR_MOTION_FORWARD;
	data->motorMotion_Right = MOTOR_MOTION_FORWARD;

	/* if the last motion is not backward, we can restore the old speed. */
	if ((lastMotorData.motorMotion_Left != MOTOR_MOTION_BACKWARD) && (lastMotorData.motorMotion_Right != MOTOR_MOTION_BACKWARD)) {
		MotorSetSpeed(data, lastMotorData.currentSpeedLeft > 0 ? lastMotorData.currentSpeedLeft : MOTORS_DEFAULT_FW_SPEED,
				lastMotorData.currentSpeedRight > 0 ? lastMotorData.currentSpeedLeft : MOTORS_DEFAULT_FW_SPEED);
	} else {
		/* or we just set the speed by default */
		MotorSetSpeed(data,MOTORS_DEFAULT_FW_SPEED, MOTORS_DEFAULT_FW_SPEED);
	}
}

/**
 * Sets LN298 GPIO controls for backward motion
 */
static void motorSetMotionBackward(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	lastMotorData = *data; /* backup the data into the last known values struct */
	data->motorMotion_Left = MOTOR_MOTION_BACKWARD;
	data->motorMotion_Right = MOTOR_MOTION_BACKWARD;
	MotorSetSpeed(data, MOTORS_DEFAULT_BW_SPEED, MOTORS_DEFAULT_BW_SPEED);
}

/**
 * Sets LN298 GPIO controls for left turn motion
 */
static void motorSetMotionTurnLeft(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	lastMotorData = *data; /* backup the data into the last known values struct */
	data->motorMotion_Left = MOTOR_MOTION_BACKWARD;
	data->motorMotion_Right = MOTOR_MOTION_FORWARD;
}

/**
 * Sets LN298 GPIO controls for right turn motion
 */
static void motorSetMotionTurnRight(MotorData_t *data)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	lastMotorData = *data; /* backup the data into the last known values struct */
	data->motorMotion_Left = MOTOR_MOTION_FORWARD;
	data->motorMotion_Right = MOTOR_MOTION_BACKWARD;
}

/**
 * Sets motors power to 0 but does not change the recorded motion
 */
static void motorsSetMotionIdle(MotorData_t *data)
{
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	lastMotorData = *data; /* backup the data into the last known values struct */
	MotorSetSpeed(data, 0, 0);
	data->motorMotion_Left = MOTOR_MOTION_IDLE;
	data->motorMotion_Right = MOTOR_MOTION_IDLE;
}


