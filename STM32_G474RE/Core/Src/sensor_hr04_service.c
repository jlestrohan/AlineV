/*
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *      SEE https://www.carnetdumaker.net/articles/mesurer-une-distance-avec-un-capteur-ultrason-hc-sr04-et-une-carte-arduino-genuino/
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include "lcd_service.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

#define EVENT_HR04_ENABLE_MEASURE		0x012U 	/* event flag */
#define TRIG_TIMER_DELAY				10LU 	/* 10µs */

//static uint32_t currentSystick; /* contains trig hal_systick for each HR-04 */
//static osStatus_t status; /* function return status */
static uint32_t local_time = 0;
static uint32_t final_time = 0;
//static uint32_t sensor_time;
//static uint16_t distance;

static TIM_HandleTypeDef *_htim;
typedef StaticTask_t osStaticThreadDef_t;

/* Definitions for HR04Sensor_Trig_task TRIGGER EMITTER*/
static uint32_t HR04SensorTaBuffer[256];
static osStaticThreadDef_t HR04SensorTaControlBlock;
static osThreadId_t HR04Sensor_taskHandle;
static const osThreadAttr_t HR04Sensor_task_attributes = {
        .name = "HR04Sensor_task", .stack_mem = &HR04SensorTaBuffer[0],
        .stack_size = sizeof(HR04SensorTaBuffer),
        .cb_mem = &HR04SensorTaControlBlock,
        .cb_size = sizeof(HR04SensorTaControlBlock),
        .priority = (osPriority_t) osPriorityBelowNormal, };

/**
 *	HR04 task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		/* first we pull the HR04_1_TRIG_Pin high */
		HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_SET);

		/* and then we start the TIM 1 µs timer */
		HAL_TIM_Base_Start(_htim);

		/*distance = sensor_time * .034 / 2;*/

		/*lcd_send_cmd(0x80);
		 lcd_send_string("Dist =");
		 lcd_send_data((distance / 100) + 48);
		 lcd_send_data(((distance % 100) / 10) + 48);
		 lcd_send_data(((distance % 10)) + 48);
		 lcd_send_string(" cm");*/

		osEventFlagsWait(evt_hr04_sensor, EVENT_HR04_TRIG_SENSOR_1, osFlagsWaitAny, osWaitForever);

		/* flag received after 10µs we stop that timer */
		HAL_TIM_Base_Stop(_htim);

		/* and then lets' pull the HR04_1_TRIG_Pin low */
		HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_RESET);

		/* now we start counting in millis */
		local_time = HAL_GetTick();

		/* next thing will be waiting for an interrupt signal on the echo pin and then calculate the delay spent inbetween */
		osEventFlagsWait(evt_hr04_sensor, EVENT_HR04_ECHO_SENSOR_1, osFlagsWaitAny, osWaitForever);

		/* now we have to get the final time and compare */
		final_time = HAL_GetTick();

		char test[20];
		sprintf(test, "distance = %lu", final_time - local_time);
		loggerI(test);

		osDelay(100); /* we need to calculate distance every 100ms or so */

	}
}

/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize(TIM_HandleTypeDef *htim)
{
	assert(htim);
	_htim = htim;

	/* creation of HR04Sensor_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04Sensor_task_attributes);
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creates event flag */
	evt_hr04_sensor = osEventFlagsNew(NULL);
	if (evt_hr04_sensor == NULL) {
		loggerE("HR04 Event Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HR04 - Initialization complete");
	return (EXIT_SUCCESS);
}

