/*******************************************************************
 * MG90S_service.c
 *
 *  Created on: 23 avr. 2020
 *      Author: Jack Lestrohan
 *
 *	TIM5 -> PWM 20ms -> PB2
 *******************************************************************/

#define FLAG_MG90S_STATUS_IDLE					(1 << 0)
#define FLAG_MG90S_STATUS_CENTER				(1 << 1)
#define FLAG_MG90S_STATUS_THREE_PROBES			(1 << 2)
#define FLAG_MG90S_STATUS_HALF_RADAR			(1 << 3)

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
xServoPosition_t xServoPosition;
osEventFlagsId_t xFlagSensorMotionStatus; /* used to tell the three probes task to run its code or not */


/**
 * Main task definition
 */
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

/**
 * THREE PROBES task definition
 */
static osThreadId_t xFrontServoThreeProbesTaskHnd;
static osStaticThreadDef_t xFrontServoThreeProbesTaControlBlock;
static uint32_t xFrontServoThreeProbesTaBuffer[256];
static const osThreadAttr_t xFrontServoThreeProbesTa_attributes = {
		.name = "xFrontServoThreeProbesServiceTask",
		.stack_mem = &xFrontServoThreeProbesTaBuffer[0],
		.stack_size = sizeof(xFrontServoThreeProbesTaBuffer),
		.cb_mem = &xFrontServoThreeProbesTaControlBlock,
		.cb_size = sizeof(xFrontServoThreeProbesTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_MG90S_3PROBES, };

/**
 * Three Probes pattern task
 * @param vParameter
 */
void vFrontServoThreeProbes_Start(void *vParameter)
{
	xServoPosition_t direction = SERVO_DIRECTION_CENTER;
	uint32_t flags;

	for (;;)
	{
		/* only active if internal flag is set */
		flags = osEventFlagsGet(xFlagSensorMotionStatus);
		if (flags == FLAG_MG90S_STATUS_THREE_PROBES) {
			htim5.Instance->CCR1 = xServoPosition; /* 0 to 100 */
			switch (xServoPosition) {
			case SERVO_DIRECTION_LEFT45:
				xServoPosition = SERVO_DIRECTION_CENTER; direction = SERVO_DIRECTION_RIGHT45; break;
			case SERVO_DIRECTION_CENTER:
				xServoPosition=(direction == SERVO_DIRECTION_RIGHT45 ? SERVO_DIRECTION_RIGHT45 : SERVO_DIRECTION_LEFT45); break;
			case SERVO_DIRECTION_RIGHT45: default:
				xServoPosition = SERVO_DIRECTION_CENTER; direction = SERVO_DIRECTION_LEFT45; break;
			}
			osDelay(500);
		}

		/* Radar mode */


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

	for (;;) {

		status = osMessageQueueGet(xQueueMg90sMotionOrder, &xServopattern, NULL, 0U); // no wait
		if (status == osOK) {
			osEventFlagsClear(xFlagSensorMotionStatus, 0xFFFF); /* clears all remaining flags to be sure */

			switch (xServopattern) {
			case SERVO_PATTERN_IDLE: default:
				osEventFlagsSet(xFlagSensorMotionStatus, FLAG_MG90S_STATUS_IDLE);
				break;
			case SERVO_PATTERN_RETURN_CENTER:
				osEventFlagsSet(xFlagSensorMotionStatus, FLAG_MG90S_STATUS_CENTER);
				htim5.Instance->CCR1 = SERVO_DIRECTION_CENTER;
				break;
			case SERVO_PATTERN_THREE_PROBES:
				osEventFlagsSet(xFlagSensorMotionStatus, FLAG_MG90S_STATUS_THREE_PROBES);
				break;
			case SERVO_PATTERN_HALF_RADAR:
				/* sets the starting position */
				htim5.Instance->CCR1 = 25; /* to the right first */
				osEventFlagsSet(xFlagSensorMotionStatus, FLAG_MG90S_STATUS_HALF_RADAR);
				break;
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
		Error_Handler();
		return (EXIT_FAILURE);
	}

	xFlagSensorMotionStatus = osEventFlagsNew(NULL);

	/* creation of xFrontServo_task */
	xFrontServoTaskHnd = osThreadNew(vFrontServo_Start, NULL, &xFrontServoTa_attributes);
	if (xFrontServoTaskHnd == NULL) {
		printf("Front Servo Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xFrontServoThreeProbesTaskHnd */
	xFrontServoThreeProbesTaskHnd = osThreadNew(vFrontServoThreeProbes_Start, NULL, &xFrontServoThreeProbesTa_attributes);
	if (xFrontServoThreeProbesTaskHnd == NULL) {
		printf("Front Servo Three Probes Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	if (HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1) != HAL_OK) {
		printf("Cannot start MG90 pwm Timer\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* sets to center */
	htim5.Instance->CCR1 = 75;

	/* suspending for now
	 * Will resume when motion forward */
	//osThreadSuspend(xFrontServo_taskHandle);

	printf("Initializing Front Servo... Success!\n\r");
	return (EXIT_SUCCESS);
}
