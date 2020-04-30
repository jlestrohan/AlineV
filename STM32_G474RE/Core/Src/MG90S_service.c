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
#include "freertos_logger_service.h"
#include "LCD_service.h"

/* flag to track activity of the servo */

osEventFlagsId_t evt_Mg90sIsActive;

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

typedef enum {
	SERVO_LEFT,
	SERVO_RIGHT
} ServoDirection_t;

/**
 * Front Servo task rouotine
 * @param vParameters
 */
void vFrontServo_Start(void* vParameters)
{
	loggerI("Starting FrontServo Service task...");
	char msg[10];
	uint8_t pos = 75; /* center for a start */
	ServoDirection_t dir; /* 0 to left, 1 to right */

	for (;;) {
		/* prevent compilation warning */
		UNUSED(vParameters);

		if (osEventFlagsGet(evt_Mg90sIsActive) && FLG_MG90S_ACTIVE) {
			htim5.Instance->CCR1 = pos; /* 0 to 100 */
			switch (pos) {
			case 50:
				pos = 75; dir = SERVO_RIGHT; break;
			case 75:
				pos=(dir == SERVO_RIGHT ? 100 : 50); break;
			case 100:
				pos = 75; dir = SERVO_LEFT; break;
			default:
				pos = 75; dir = SERVO_LEFT;
			}
			osDelay(1000);

		} else {
			/* sets to center */
			htim5.Instance->CCR1 = 75;
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
	evt_Mg90sIsActive = osEventFlagsNew(NULL);
	if (evt_Mg90sIsActive == NULL) {
		loggerE("Front Servo Event Flag Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creation of xFrontServo_task */
	xFrontServo_taskHandle = osThreadNew(vFrontServo_Start, NULL, &xFrontServoTa_attributes);
	if (!xFrontServo_taskHandle) {
		//todo: improve error check routines here */
		loggerE("Front Servo Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	if (HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1) != HAL_OK) {
		return (EXIT_FAILURE);
	}

	/* sets to center */
	htim5.Instance->CCR1 = 75;

	/* suspending for now
	 * Will resume when motion forward */
	//osThreadSuspend(xFrontServo_taskHandle);

	loggerI("Initializing Front Servo... Success!");
	return (EXIT_SUCCESS);
}
