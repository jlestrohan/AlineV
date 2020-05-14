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
osMessageQueueId_t xMessageQueueSensorMotionStatus; /* used to tell the three probes task to run its code or not */


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
	uint8_t motion_status;
	osStatus_t status;

	for (;;)
	{
		status = osMessageQueueGet(xMessageQueueSensorMotionStatus, &motion_status, 0U, 0U);
		switch(motion_status){
		case FLAG_MG90S_STATUS_THREE_PROBES:
			htim5.Instance->CCR1 = xServoPosition; /* 0 to 100 */
			switch (xServoPosition) {
			case SERVO_DIRECTION_LEFT45:
				xServoPosition = SERVO_DIRECTION_CENTER;  /* go to the right place */
				if (htim5.Instance->CCR1 == SERVO_DIRECTION_CENTER) { /* time for the servo to be on place */
					HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3); /* starts measuring */
				}
				direction = SERVO_DIRECTION_RIGHT45;
				osDelay(200);
				HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_3); /* stops measuring */
				break;
			case SERVO_DIRECTION_CENTER:
				if (direction == SERVO_DIRECTION_RIGHT45) {
					xServoPosition = SERVO_DIRECTION_RIGHT45;
					if (htim5.Instance->CCR1 == SERVO_DIRECTION_RIGHT45) { /* time for the servo to be on place */
						HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3); /* starts measuring */
					}
				} else {
					xServoPosition = SERVO_DIRECTION_LEFT45;
					if (htim5.Instance->CCR1 == SERVO_DIRECTION_LEFT45) { /* time for the servo to be on place */
						HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3); /* starts measuring */
					}
				}

				osDelay(100); /* time for the servo to be on place */
				HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3); /* starts measuring */
				osDelay(300);
				HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_3); /* stops measuring */
				break;
			case SERVO_DIRECTION_RIGHT45: default:
				xServoPosition = SERVO_DIRECTION_CENTER;
				direction = SERVO_DIRECTION_LEFT45;
				if (htim5.Instance->CCR1 == SERVO_DIRECTION_CENTER) { /* time for the servo to be on place */
					HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3); /* starts measuring */
				}
				osDelay(200);
				HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_3); /* stops measuring */
				break;
			}
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
	uint8_t motion_status;

	for (;;) {

		status = osMessageQueueGet(xQueueMg90sMotionOrder, &xServopattern, NULL, osWaitForever); // no wait
		if (status == osOK) {

			switch (xServopattern) {
			case SERVO_PATTERN_IDLE: default:
				motion_status = FLAG_MG90S_STATUS_IDLE;
				osMessageQueuePut(xMessageQueueSensorMotionStatus, &motion_status, 0U, osWaitForever);
				break;
			case SERVO_PATTERN_RETURN_CENTER:
				motion_status = FLAG_MG90S_STATUS_CENTER;
				osMessageQueuePut(xMessageQueueSensorMotionStatus, &motion_status, 0U, osWaitForever);
				htim5.Instance->CCR1 = SERVO_DIRECTION_CENTER;
				break;
			case SERVO_PATTERN_THREE_PROBES:
				motion_status = FLAG_MG90S_STATUS_THREE_PROBES;
				osMessageQueuePut(xMessageQueueSensorMotionStatus, &motion_status, 0U, osWaitForever);
				break;
			case SERVO_PATTERN_HALF_RADAR:
				/* sets the starting position */
				htim5.Instance->CCR1 = 25; /* to the right first */
				motion_status = FLAG_MG90S_STATUS_HALF_RADAR;
				osMessageQueuePut(xMessageQueueSensorMotionStatus, &motion_status, 0U, osWaitForever);
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

	xMessageQueueSensorMotionStatus =  osMessageQueueNew(10, sizeof(uint8_t), NULL);
	if (xMessageQueueSensorMotionStatus == NULL) {
		printf("Front Servo MotionStatus Queue Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

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
