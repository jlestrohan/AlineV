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

/**
 * Motors Control main task
 * @param argument
 */
static void MotorsControlTask_Start(void *vParameters)
{


	/* prevent compilation warning */
	UNUSED(vParameters);

	/* TEST DRIVE */
	// 1 - set motors motion forward
	osDelay(2000);
	motorSetForward();
	MotorSetSpeed(20);
	osDelay(2000);
	motorsSetIdle();
	osDelay(2000);
	motorSetBackward();
	MotorSetSpeed(20);
	osDelay(2000);
	motorsSetIdle();
	osDelay(2000);
	motorSetTurnLeft();
	MotorSetSpeed(20);
	osDelay(2000);
	motorsSetIdle();
	osDelay(2000);
	motorSetTurnRight();
	MotorSetSpeed(20);
	osDelay(2000);
	motorSetForward();
	MotorSetSpeed(0);

	osDelay(5000);

	osThreadSuspend(NULL);
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
MOTORS_Result_t MotorsControl_Service_Initialize()
{
	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

	/* initializes PWM duties to 0 for now (idle)
	htim16.Instance->CCR1 = 0;
		htim17.Instance->CCR1 = 0;

	/* creation of HR04Sensor1_task */
	MotorsControl_taskHandle = osThreadNew(MotorsControlTask_Start, NULL, &MotorsControlTa_attributes);
	if (!MotorsControl_taskHandle) {
		//todo: improve error check routines here */
		loggerE("MotorsControl Task Initialization Failed");
		return (EXIT_FAILURE);
	}



	//loggerI("motors control called");

	return (MOTORS_Result_Ok);
}

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange)
{

}

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange)
{

}

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param speed (0-100)
 */
void MotorSetSpeed(uint8_t speed)
{
	htim16.Instance->CCR1 = speed;
	htim17.Instance->CCR1 = speed;
}

/**
 * Stop the targetted motor(s) using motionchange rate
 * @param motionchange
 */
void MotorStop(MotorsMotionChangeRate_t motionchange)
{

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
