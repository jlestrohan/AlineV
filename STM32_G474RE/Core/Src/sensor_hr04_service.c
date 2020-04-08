/*
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *      SEE https://www.carnetdumaker.net/articles/mesurer-une-distance-avec-un-capteur-ultrason-hc-sr04-et-une-carte-arduino-genuino/
 *
 *      https://www.youtube.com/watch?v=Ir-5FnmaESE
 *      https://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <assert.h>
#include "dwt_delay.h"
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include "lcd_service.h"
#include <string.h>
#include <stdio.h>

#define EVENT_HR04_ENABLE_MEASURE		0x012U 	/* event flag */
#define TRIG_TIMER_DELAY				10LU 	/* 10µs */
#define HR04_QUEUE_MAX_VALS				40		/* max number of values held in queue */
#define HALF_SOUND_SPEED_10USEC 		0.0171821*2	/* distance = measured time * 0.0171821 * 2 */

volatile uint8_t distValid[MAX_SONAR];

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
static uint16_t triggerSonar(uint8_t sonarNumber)
{
	uint16_t local_time=0;
	uint16_t trigPin, echoPin;

	switch (sonarNumber) {
	case HR04_SONAR_1:
		trigPin = HR04_1_TRIG_Pin;
		echoPin = HR04_1_ECHO_Pin;
		break;
	default: break;
	}

	HAL_GPIO_WritePin(GPIOA, trigPin, GPIO_PIN_SET);
	DWT_Delay(10);
	HAL_GPIO_WritePin(GPIOA, trigPin, GPIO_PIN_RESET);


	while (!(HAL_GPIO_ReadPin(GPIOA, echoPin))) {};  // wait for the ECHO pin to go high
	while (HAL_GPIO_ReadPin(GPIOA, echoPin))    // while the pin is high
	{
		local_time++;   // measure time for which the pin is high
		DWT_Delay (1);  /* microsecond delay */
	}

	return local_time;
}

/**
 *	HR04 task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	char msg[40];
	uint16_t sensor_time;

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		/* first we trigger 0 and 2 */
		sensor_time = triggerSonar(HR04_SONAR_1);

		//sprintf(msg, "%d", (uint16_t)(sensor_time * HALF_SOUND_SPEED_10USEC));
		//loggerI(msg);

		osDelay(70); /* need to wait at least 60ms to start the operation again */
	}
}




/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize()
{

	/* creation of HR04Sensor_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04SensorTa_attributes); /* &HR04Sensor_task_attributes); */
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HR04 - Initialization complete");
	return (EXIT_SUCCESS);
}

