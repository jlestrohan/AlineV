/*******************************************************************
 * MG90S_service.c
 *
 *  Created on: 23 avr. 2020
 *      Author: Jack Lestrohan
 *
 *	TIM5 -> PWM 20ms -> PB2
 *******************************************************************/

#include "MG90S_service.h"
#include "configuration.h"
#include "HCSR04_service.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdlib.h>
#include "tim.h"
#include "printf.h"
#include "main.h"
#include <stdbool.h>
#include "navControl_service.h"

typedef StaticTask_t osStaticThreadDef_t;

osMessageQueueId_t xQueueMg90sMotionOrder;

/* mutexed externs */
xServoPosition_t xServoPosition;
osMutexId_t mServoPositionMutex;

osMessageQueueId_t xMessageQueueSensorMotionStatus; /* used to tell the three probes task to run its code or not */

/*********************************************************************/
/* MAIN DECISION TASK DEFINITION */
static osThreadId_t xFrontServoTaskHnd;
static osStaticThreadDef_t xFrontServoTaControlBlock;
static uint32_t xFrontServoTaBuffer[256];
static const osThreadAttr_t xFrontServoTa_attributes = {
		.name = "xFrontServoServiceTask",
		.stack_mem = &xFrontServoTaBuffer[0],
		.stack_size = sizeof(xFrontServoTaBuffer),
		.cb_mem = &xFrontServoTaControlBlock,
		.cb_size = sizeof(xFrontServoTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_MG90S, };

/*********************************************************************/
/* THREE PROBES MOTION TASK DEFINITION */
static osThreadId_t xThreeProbesMotionServoTaskHnd;
static osStaticThreadDef_t xThreeProbesMotionServoTaControlBlock;
static uint32_t xThreeProbesMotionServoTaBuffer[256];
static const osThreadAttr_t xThreeProbesMotionServoTa_attributes = {
		.name = "xThreeProbesMotionServoTask",
		.stack_mem = &xThreeProbesMotionServoTaBuffer[0],
		.stack_size = sizeof(xThreeProbesMotionServoTaBuffer),
		.cb_mem = &xThreeProbesMotionServoTaControlBlock,
		.cb_size = sizeof(xThreeProbesMotionServoTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_MG90S_3PROBES, };


/**
 * Three Probes pattern task
 * @param vParameter
 */
void vFrontServoThreeProbes_Start(void *vParameter)
{
	printf("Starting FrontServo ThreeProbes pattern task...\n\r");

	xServoPosition_t direction = SERVO_DIRECTION_CENTER; /* direction that the servo is about to take */


	/* starts servo general PWM timer */
	if (HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1) != HAL_OK) {
		printf("Cannot start MG90 pwm Timer\n\r");
		Error_Handler();
	}

	/* suspends self for now */
	osThreadSuspend(xThreeProbesMotionServoTaskHnd);

	for (;;)
	{
		MUTEX_SERVO_TAKE
		xServoPosition = htim5.Instance->CCR1; /* constantly publishes the real servo position */
		MUTEX_SERVO_GIVE

		switch (htim5.Instance->CCR1) {

		case SERVO_DIRECTION_LEFT45:
			htim5.Instance->CCR1 = SERVO_DIRECTION_CENTER;  /* go to the right place */
			if (htim5.Instance->CCR1 == SERVO_DIRECTION_CENTER) { /* we check if the servo has reached its position */
#ifdef DEBUG_MG90S
				MUTEX_SERVO_TAKE
				printf("Servo position: CENTER\n\r");
				MUTEX_SERVO_GIVE
#endif

			}
			direction = SERVO_DIRECTION_RIGHT45;
			osDelay(200);
			break;
		case SERVO_DIRECTION_CENTER:
			if (direction == SERVO_DIRECTION_RIGHT45) {

				htim5.Instance->CCR1 = SERVO_DIRECTION_RIGHT45;
				if (htim5.Instance->CCR1 == SERVO_DIRECTION_RIGHT45) { /* we check if the servo has reached its position */
#ifdef DEBUG_MG90S
					MUTEX_SERVO_TAKE
					printf("Servo position: RIGHT 45\n\r");
					MUTEX_SERVO_GIVE
#endif
				}
			} else {
				htim5.Instance->CCR1 = SERVO_DIRECTION_LEFT45;
				if (htim5.Instance->CCR1 == SERVO_DIRECTION_LEFT45) { /* we check if the servo has reached its position */
#ifdef DEBUG_MG90S
					MUTEX_SERVO_TAKE
					printf("Servo position: LEFT 45\n\r");
					MUTEX_SERVO_GIVE
#endif

				}
			}
			osDelay(300);
			break;

		case SERVO_DIRECTION_RIGHT45: default:
			htim5.Instance->CCR1 = SERVO_DIRECTION_CENTER;
			direction = SERVO_DIRECTION_LEFT45;
			if (htim5.Instance->CCR1 == SERVO_DIRECTION_CENTER) { /* we check if the servo has reached its position */
#ifdef DEBUG_MG90S
				MUTEX_SERVO_TAKE
				printf("Servo position: CENTER\n\r");
				MUTEX_SERVO_GIVE
#endif

			}
			osDelay(200);
			break;
		}
		osDelay(1);
	}
	osThreadTerminate(NULL);
}

/**
 * Front Servo task rouotine
 * @param vParameters
 */
void vFrontServo_Start(void* vParameters)
{
	printf("Starting FrontServo Service task...\n\r");

	xServoPattern_t xServopattern;
	osStatus_t status;
	//uint8_t motion_status;

	for (;;) {

		status = osMessageQueueGet(xQueueMg90sMotionOrder, &xServopattern, NULL, osWaitForever); // no wait
		if (status == osOK) {

			switch (xServopattern) {
			case SERVO_PATTERN_IDLE:
				htim5.Instance->CCR1 = SERVO_DIRECTION_CENTER;
				osThreadSuspend(xThreeProbesMotionServoTaskHnd);
				break;
			case SERVO_PATTERN_THREE_PROBES:
				osThreadResume(xThreeProbesMotionServoTaskHnd);
				break;
			case SERVO_PATTERN_HALF_RADAR:
				/* sets the starting position */
				/* not done yet, have to create a task for this */
				break;
			case SERVO_PATTERN_LEFT45:
				htim5.Instance->CCR1 = SERVO_DIRECTION_LEFT45;
				break;
			case SERVO_PATTERN_LEFT90:
				htim5.Instance->CCR1 = SERVO_DIRECTION_LEFT90;
				break;
			case SERVO_PATTERN_RIGHT45:
				htim5.Instance->CCR1 = SERVO_DIRECTION_RIGHT45;
				break;
			case SERVO_PATTERN_RIGHT90:
				htim5.Instance->CCR1 = SERVO_DIRECTION_LEFT90;
				break;
			default: break;
			}

		}

		osDelay(10);
	}
	osThreadTerminate(NULL);
}

/**
 * Main setup routine
 * @return
 */
uint8_t uMg90sServiceInit()
{

	xQueueMg90sMotionOrder = osMessageQueueNew(10, sizeof(uint8_t), NULL);
	if (xQueueMg90sMotionOrder == NULL) {
		printf("Front Servo Message Queue Initialization Failed\n\r");
		osThreadTerminate(NULL);
		Error_Handler();
	}

	/* creation of xFrontServo_task */
	xFrontServoTaskHnd = osThreadNew(vFrontServo_Start, NULL, &xFrontServoTa_attributes);
	if (xFrontServoTaskHnd == NULL) {
		printf("Front Servo Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xFrontServoThreeProbesTaskHnd */
	xThreeProbesMotionServoTaskHnd = osThreadNew(vFrontServoThreeProbes_Start, NULL, &xThreeProbesMotionServoTa_attributes);
	if (xThreeProbesMotionServoTaskHnd == NULL) {
		printf("Front Servo Three Probes Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Front Servo... Success!\n\r");
	return (EXIT_SUCCESS);
}
