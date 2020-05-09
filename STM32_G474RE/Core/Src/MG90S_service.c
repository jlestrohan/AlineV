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
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdlib.h>
#include "tim.h"
#include "printf.h"
#include "main.h"

/* flag to track activity of the servo */

extern osEventFlagsId_t evt_Mg90sMotionControlFlag;

xServoPosition_t xServoPosition = ServoDirection_Center; /* center for a start */

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xFrontServo_taskHandle;
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
 * Front Servo task rouotine
 * @param vParameters
 */
void vFrontServo_Start(void* vParameters)
{
	printf("Starting FrontServo Service task...\n\r");

	xServoPosition_t direction = ServoDirection_Center;

	for (;;) {

		if (osEventFlagsGet(evt_Mg90sMotionControlFlag) && FLG_MG90S_ACTIVE) {
			htim5.Instance->CCR1 = xServoPosition; /* 0 to 100 */
			switch (xServoPosition) {
			case ServoDirection_Left:
				xServoPosition = ServoDirection_Center; direction = ServoDirection_Right; break;
			case ServoDirection_Center:
				xServoPosition=(direction == ServoDirection_Right ? ServoDirection_Right : ServoDirection_Left); break;
			case ServoDirection_Right: default:
				xServoPosition = ServoDirection_Center; direction = ServoDirection_Left; break;
			}
			osDelay(1000);

		} else {
			/* sets to center */
			direction = ServoDirection_Center;
			htim5.Instance->CCR1 = 75;
		}


		osDelay(10);
	}
	osThreadTerminate(xFrontServo_taskHandle);
}

/**
 * Main setup routine
 * @return
 */
uint8_t uMg90sServiceInit()
{
	evt_Mg90sMotionControlFlag = osEventFlagsNew(NULL);
	if (evt_Mg90sMotionControlFlag == NULL) {
		printf("Front Servo Event Flag Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xFrontServo_task */
	xFrontServo_taskHandle = osThreadNew(vFrontServo_Start, NULL, &xFrontServoTa_attributes);
	if (xFrontServo_taskHandle == NULL) {
		printf("Front Servo Task Initialization Failed\n\r");
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
