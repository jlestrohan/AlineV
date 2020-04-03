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
#include "tim.h"
#include <string.h>
#include <stdio.h>

#define EVENT_HR04_ENABLE_MEASURE		0x012U 	/* event flag */
#define TRIG_TIMER_DELAY				10LU 	/* 10µs */

TIM_HandleTypeDef htim16;

//static uint32_t currentSystick; /* contains trig hal_systick for each HR-04 */
//static osStatus_t status; /* function return status */

static uint32_t sensor_time;
static uint16_t distance;

typedef StaticTask_t osStaticThreadDef_t;

/* Definitions for HR04Sensor_Trig_task TRIGGER EMITTER*/
static uint32_t HR04SensorTaBuffer[256];
static osStaticThreadDef_t HR04SensorTaControlBlock;
static osThreadId_t HR04Sensor_taskHandle;
static const osThreadAttr_t HR04Sensor_task_attributes = {
        .name = "HR04Sensor_task",
        .stack_mem = &HR04SensorTaBuffer[0],
        .stack_size = sizeof(HR04SensorTaBuffer),
        .cb_mem = &HR04SensorTaControlBlock,
        .cb_size = sizeof(HR04SensorTaControlBlock),
        .priority = (osPriority_t) osPriorityNormal, };

uint32_t hcsr04_read(void)
{
	uint32_t local_time;
	/* first we pull the HR04_1_TRIG_Pin high */
	//HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(GPIOA, OSCIL_MEAS_Pin, GPIO_PIN_SET);
	/* 10µs delay */
	//DWT_Delay(10);
	/* pull down Trig pin, signal sent */
	//HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_htim16RESET);
	//HAL_GPIO_WritePin(GPIOA, OSCIL_MEAS_Pin, GPIO_PIN_RESET);
	while (!(HAL_GPIO_ReadPin(GPIOC, HR04_1_ECHO_Pin)));

	while (HAL_GPIO_ReadPin(GPIOC, HR04_1_ECHO_Pin)) {
		local_time++; // increment local time
		DWT_Delay(1); // every 1µs
	}
	return local_time * 2;
}

/**
 *	HR04 task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	loggerI("Starting hr04 service task...");

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
		sensor_time = hcsr04_read();
		distance = sensor_time * .034 / 2;

		/*lcd_send_cmd(0x80);
		 lcd_send_string("Dist =");
		 lcd_send_data((distance / 100) + 48);
		 lcd_send_data(((distance % 100) / 10) + 48);
		 lcd_send_data(((distance % 10)) + 48);
		 lcd_send_string(" cm");*/

		/* next thing will be waiting for an interrupt signal on the echo pin and then calculate the delay spent inbetween */
		//osEventFlagsWait(evt_hr04_sensor, EVENT_HR04_ECHO_SENSOR_1, osFlagsWaitAny, osWaitForever);
		//loggerI("event passed event flag wait routine");
		/* now we have to get the final time and compare */
		//final_time = HAL_GetTick();
		sprintf(test, "%d cm", distance);
		loggerI(test);
		/*lcd_send_cmd(0x80);
		 lcd_send_string(test);*/
		osDelay(50); /* we need to calculate distance every 100ms or so */

	}
}

/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize()
{
	/* creates event flag */
	evt_hr04_sensor = osEventFlagsNew(NULL);
	if (evt_hr04_sensor == NULL) {
		loggerE("HR04 Event Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04Sensor_task_attributes);
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HR04 - Initialization complete");
	return (EXIT_SUCCESS);
}

