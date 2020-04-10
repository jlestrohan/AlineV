/*
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <assert.h>
#include "dwt_delay.h"
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
static osThreadId_t HR04Sensor_taskHandle;
static osStaticThreadDef_t HR04SensorTaControlBlock;
static uint32_t HR04SensorTaBuffer[256];
static const osThreadAttr_t HR04SensorTa_attributes = {
		.name = "HR04SensorServiceTask",
		.stack_mem = &HR04SensorTaBuffer[0],
		.stack_size = sizeof(HR04SensorTaBuffer),
		.cb_mem = &HR04SensorTaControlBlock,
		.cb_size = sizeof(HR04SensorTaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };

/**
 * Sends a single echo to HR04_1_TRIG_Pin
 */
static void triggerSonar(uint8_t sonarNumber)
{
	uint16_t trigPin;
	TIM_HandleTypeDef *htim;
	GPIO_TypeDef *GPIOx;
	uint32_t Channel;

	switch (sonarNumber) {
	case HR04_SONAR_1:
		trigPin = HR04_1_TRIG_Pin;
		htim = &htim2;
		GPIOx = GPIOA;
		Channel = TIM_CHANNEL_1;
		break;
	default: break;
	}

	HAL_GPIO_WritePin(GPIOx, trigPin, GPIO_PIN_RESET);
	DWT_Delay(2);

	HAL_GPIO_WritePin(GPIOx, trigPin, GPIO_PIN_SET);
	DWT_Delay(10);
	HAL_GPIO_WritePin(GPIOx, trigPin, GPIO_PIN_RESET);

	/* start the echo counter for one pulse (1000000 * 1µs counter) */
	HAL_TIM_IC_Start_IT(htim, Channel);
}

/**
 *	HR04 task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	char msg[40];
	osStatus_t status;
	uint16_t result;

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		/* first we trigger 0 and 2 */
		triggerSonar(HR04_SONAR_1);

		status = osMessageQueueGet(queue_icValueHandle, &HR04_SensorsData, NULL, 1000U);   /* waits for message for 1 sec and no more */
		if (status == osOK) {
			result = (uint16_t)(HR04_SensorsData.echo_capture * HALF_SOUND_SPEED_10USEC);
			if (result < 2000) {
			sprintf(msg, "cm: %d          ", result);
			osSemaphoreAcquire(sem_lcdService, osWaitForever);
			lcd_send_string(msg);
			osSemaphoreRelease(sem_lcdService);
			//loggerI(msg);
			}
		}

		osDelay(60); /* need to wait at least 60ms to start the operation again */
	}
}

/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize()
{
	queue_icValueHandle = osMessageQueueNew(HR04_QUEUE_MAX_OBJECTS, sizeof(HR04_SensorsData), NULL);
	if (!queue_icValueHandle) {
		loggerE("HR04 Message Queue object not created, handle failure");
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04SensorTa_attributes); /* &HR04Sensor_task_attributes); */
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Task Initialization Failed");
		return (EXIT_FAILURE);
	}



	loggerI("HR04 - Initialization complete");
	return (EXIT_SUCCESS);
}

