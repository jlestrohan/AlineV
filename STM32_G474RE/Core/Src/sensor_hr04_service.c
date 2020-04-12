/**
 ******************************************************************************
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *
 *	Timers and Pinout:
 *
 *		Sonar			Trig Timer		Echo Timer		Trig Pin		Echo Pin
 *		-------------------------------------------------------------------------
 *		HR04_SONAR_1	TIM16 PWM		TIM2			PA6				PA0
 *		HR04_SONAR_2	TIM16 PWM		TIM5			PB6				PB2
 *
 *******************************************************************************
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include "tim.h"
#include "freertos_logger_service.h"
#include "lcd_service.h"
#include <string.h>
#include <stdio.h>

#define HR04_QUEUE_MAX_OBJECTS			40		/* max number of values held in queue */
#define HALF_SOUND_SPEED_10USEC 		0.0171821	/* distance = measured time * 0.0171821 * 2 */

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t HR04Sensor_taskHandle, HR04SensorDisplay_taskHandle;
static osStaticThreadDef_t HR04SensorTaControlBlock, HR04SensorDisplayTaControlBlock;
static uint32_t HR04SensorTaBuffer[256], HR04SensorDisplayTaBuffer[256];
static const osThreadAttr_t HR04SensorTa_attributes = {
		.name = "HR04SensorServiceTask",
		.stack_mem = &HR04SensorTaBuffer[0],
		.stack_size = sizeof(HR04SensorTaBuffer),
		.cb_mem = &HR04SensorTaControlBlock,
		.cb_size = sizeof(HR04SensorTaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };

static const osThreadAttr_t HR04SensorDisplayTa_attributes = {
		.name = "HR04SensorDisplayServiceTask",
		.stack_mem = &HR04SensorDisplayTaBuffer[0],
		.stack_size = sizeof(HR04SensorDisplayTaBuffer),
		.cb_mem = &HR04SensorDisplayTaControlBlock,
		.cb_size = sizeof(HR04SensorDisplayTaControlBlock),
		.priority = (osPriority_t) osPriorityLow, };

/**
 *	HR04 Sensor 1 Task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	osStatus_t status;
	uint16_t result;

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		/* first we trigger PWM 1 and 2 on PA6/PB6 */
		HAL_TIM_PWM_Start (&htim16, TIM_CHANNEL_1);

		/* start the echo counter for one pulse (1000000 * 1µs counter) */
		HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

		status = osMessageQueueGet(queue_icValueHandle, &HR04_SensorsData, NULL, osWaitForever);   /* waits for message for 1 sec and no more */
		if (status == osOK) {
			result = (uint16_t)(HR04_SensorsData.echo_capture_S1 * HALF_SOUND_SPEED_10USEC);
			if (result < 500) {
				HR04_SensorsData.HR04_1_Distance = result;
			}
		}
		osDelay(60); /* need to wait at least 60ms to start the operation again */
	}
}


/**
 * HR04SensorDisplayTask_Start
 * @param argument
 */
static void HR04SensorDisplayTask_Start(void* argument)
{
	char msg[40];

	for (;;) {
		sprintf(msg, "cm1: %d          ", HR04_SensorsData.HR04_1_Distance);

		osSemaphoreAcquire(sem_lcdService, osWaitForever);
		lcd_send_string(msg);
		osSemaphoreRelease(sem_lcdService);
		loggerI(msg);

		osDelay(30);
	}
}

/**
 * Initialization function
 * @return
 */
uint8_t sensor_HR04_initialize()
{
	queue_icValueHandle = osMessageQueueNew(HR04_QUEUE_MAX_OBJECTS, sizeof(HR04_SensorsData), NULL);
	if (!queue_icValueHandle) {
		loggerE("HR04 Message Queue object not created, handle failure");
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor1_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04SensorTa_attributes); /* &HR04Sensor1_task_attributes); */
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Sensor Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creation of display task */
	HR04SensorDisplay_taskHandle = osThreadNew(HR04SensorDisplayTask_Start, NULL, &HR04SensorDisplayTa_attributes);

	loggerI("Initializing HC-SR04 Service... Success!");
	return (EXIT_SUCCESS);
}

