/*******************************************************************
 * MotorsControl_service.c
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include <FreeRTOS.h>
#include "cmsis_os2.h"
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

/* extern variables */
osMessageQueueId_t xQueueMotorMotionOrder;
osMutexId_t mMotorDataMutex, mLastMotorDataMutex;
MotorData_t MotorData, lastMotorData = {MOTOR_MOTION_IDLE, MOTOR_MOTION_IDLE, 0,0};



/* functions definitions */
static void motorsSetMotionIdle(MotorData_t *data);
static void motorSetMotionForward(MotorData_t *data, uint8_t speed);
static void motorSetMotionBackward(MotorData_t *data, uint8_t speed);
static void MotorSetSpeed(MotorData_t *data, uint8_t speed_left, uint8_t speed_right);
static void motorSetMotionTurnRight(MotorData_t *data, uint8_t speed_left, uint8_t speed_right);
static void motorSetMotionTurnLeft(MotorData_t *data, uint8_t speed_left, uint8_t speed_right);


/**
 * Motors Control main task
 * @param argument
 */
static void vMotorsControlTaskStart(void *vParameters)
{
	printf("Starting Motors Control Task...\n\r");

	mMotorDataMutex = osMutexNew(NULL);
	mLastMotorDataMutex = osMutexNew(NULL);

	xQueueMotorMotionOrder = osMessageQueueNew(10, sizeof(uint8_t), NULL);
	if (xQueueMotorMotionOrder == NULL) {
		printf("Motors Message Queue Initialization Failed\n\r");
		Error_Handler();
	}

	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

	/* initializes PWM duties to 0 for now (idle) */
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;


	MotorMotion_t msgOrder;

	for (;;)
	{
		/* event flag motors active - these flags are just disposable events flags */
		osMessageQueueGet(xQueueMotorMotionOrder, &msgOrder, NULL, osWaitForever);

		switch (msgOrder) {
		case MOTOR_MOTION_FORWARD:
			motorSetMotionForward(&MotorData, MOTORS_DEFAULT_FW_SPEED);
			break;

		case MOTOR_MOTION_BACKWARD:
			motorSetMotionBackward(&MotorData, MOTORS_DEFAULT_BW_SPEED);
			break;

		case MOTOR_MOTION_IDLE:
			motorsSetMotionIdle(&MotorData);
			break;
		case MOTOR_SPEED_NORMAL:
			MotorSetSpeed(&MotorData, MOTORS_DEFAULT_FW_SPEED, MOTORS_DEFAULT_FW_SPEED);
			break;
		case MOTOR_SPEED_REDUCE_DANGER:
			MotorSetSpeed(&MotorData, MOTORS_DEFAULT_DANGER_SPEED, MOTORS_DEFAULT_DANGER_SPEED);
			break;
		case MOTOR_SPEED_REDUCE_WARNING:
			MotorSetSpeed(&MotorData, MOTORS_DEFAULT_WARNING_SPEED, MOTORS_DEFAULT_WARNING_SPEED);
			break;
		case MOTOR_MOTION_TURN_RIGHT:
			motorSetMotionTurnRight(&MotorData, MOTORS_DEFAULT_TURN_SPEED, MOTORS_DEFAULT_TURN_SPEED);
			break;
		case MOTOR_MOTION_TURN_LEFT:
			motorSetMotionTurnLeft(&MotorData, MOTORS_DEFAULT_TURN_SPEED, MOTORS_DEFAULT_TURN_SPEED);
			break;
		case MOTOR_MOTION_FORWARD_RIGHT: //TODO: add PID here, the more/less we reduce distance against wall the more we derive each caterp speed */
			MotorSetSpeed(&MotorData, MotorData.currentSpeedLeft + 2, MotorData.currentSpeedRight - 2);
			break;
		case MOTOR_MOTION_FORWARD_LEFT: //TODO: add PID here, the more/less we reduce distance against wall the more we derive each caterp speed */
			MotorSetSpeed(&MotorData, MotorData.currentSpeedLeft - 2, MotorData.currentSpeedRight + 2);
			break;
		case MOTOR_MOTION_CORRECT_RIGHT:
			MotorSetSpeed(&MotorData, MotorData.currentSpeedLeft + MOTORS_DEFAULT_CORRECT_BIAS, MotorData.currentSpeedRight - MOTORS_DEFAULT_CORRECT_BIAS);
			break;
		case MOTOR_MOTION_CORRECT_LEFT:
			MotorSetSpeed(&MotorData, MotorData.currentSpeedLeft - MOTORS_DEFAULT_CORRECT_BIAS, MotorData.currentSpeedRight + MOTORS_DEFAULT_CORRECT_BIAS);
			break;
		default:
			motorsSetMotionIdle(&MotorData);
			break;
		}

		osDelay(1);
	}
	osThreadTerminate(xMotorsControlTaskHnd);
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
uint8_t uMotorsControlServiceInit()
{
	/* creation of the MotorsControl_task */
	xMotorsControlTaskHnd = osThreadNew(vMotorsControlTaskStart, NULL, &xMotorsControlTa_attributes);
	if (xMotorsControlTaskHnd == NULL) {
		printf("MotorsControl Task Initialization Failed\n\r");
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
	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->currentSpeedLeft = speed_left; /* records the current speed */
	data->currentSpeedRight = speed_right;
	MUTEX_MOTORDATA_GIVE
}

/**
 * Sets LN298 GPIO controls for forward motion
 * One speed for two motors, we are supposed to go forward
 */
static void motorSetMotionForward(MotorData_t *data, uint8_t speed)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->motorMotion_Left = MOTOR_MOTION_FORWARD;
	data->motorMotion_Right = MOTOR_MOTION_FORWARD;
	MUTEX_MOTORDATA_GIVE

	/* set the required speed */
	MotorSetSpeed(data, speed, speed);
}

/**
 * Sets LN298 GPIO controls for backward motion
 * We are supposed to not be turning, straight backward
 */
static void motorSetMotionBackward(MotorData_t *data, uint8_t speed)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->motorMotion_Left = MOTOR_MOTION_BACKWARD;
	data->motorMotion_Right = MOTOR_MOTION_BACKWARD;
	MUTEX_MOTORDATA_GIVE

	MotorSetSpeed(data, speed, speed);
}

/**
 * Sets LN298 GPIO controls for left turn motion
 * speed = turning speed, two different values will equal to have a turning forward motion
 */
static void motorSetMotionTurnLeft(MotorData_t *data, uint8_t speed_left, uint8_t speed_right)
{

	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_SET);

	/* now we set the right values into the main motors control struct */
	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->motorMotion_Left = MOTOR_MOTION_BACKWARD;
	data->motorMotion_Right = MOTOR_MOTION_FORWARD;
	MUTEX_MOTORDATA_GIVE

	MotorSetSpeed(&MotorData, speed_left, speed_right);
}

/**
 * Sets LN298 GPIO controls for right turn motion
 */
static void motorSetMotionTurnRight(MotorData_t *data, uint8_t speed_left, uint8_t speed_right)
{
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, MOTOR1_IN2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, MOTOR2_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, MOTOR2_IN4_Pin, GPIO_PIN_RESET);

	/* now we set the right values into the main motors control struct */
	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->motorMotion_Left = MOTOR_MOTION_FORWARD;
	data->motorMotion_Right = MOTOR_MOTION_BACKWARD;
	MUTEX_MOTORDATA_GIVE

	MotorSetSpeed(&MotorData, speed_left, speed_right);
}


/**
 * Sets motors power to 0 but does not change the recorded motion
 */
static void motorsSetMotionIdle(MotorData_t *data)
{
	htim16.Instance->CCR1 = 0;
	htim17.Instance->CCR1 = 0;

	MUTEX_MOTORDATA_TAKE
	lastMotorData = *data; /* backup the data into the last known values struct */

	data->motorMotion_Left = MOTOR_MOTION_IDLE;
	data->motorMotion_Right = MOTOR_MOTION_IDLE;
	MUTEX_MOTORDATA_GIVE

	MotorSetSpeed(data, 0, 0);
}


