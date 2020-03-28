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
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include <string.h>
#include <stdio.h>

#define EVENT_HR04_ENABLE_MEASURE		0x012U 	/* event flag */
#define TRIG_TIMER_DELAY				10LU 	/* 10µs */

uint32_t	currentSystick; 	/* contains trig hal_systick for each HR-04 */
osStatus_t  status;  			/* function return status */

TIM_HandleTypeDef htim1;
typedef StaticTask_t osStaticThreadDef_t;
osEventFlagsId_t evt_hr04_sensor; /* this event flag is raised everytime the echo task is authorized to take a measure */

/* Definitions for HR04Sensor_Trig_task TRIGGER EMITTER*/
uint32_t HR04SensorTrigTaBuffer[ 256 ];
osStaticThreadDef_t HR04SensorTrigTaControlBlock;
osThreadId_t HR04SensorTrig_taskHandle;
const osThreadAttr_t HR04SensorTrig_task_attributes = {
		.name = "HR04SensorTrig_task",
		.stack_mem = &HR04SensorTrigTaBuffer[0],
		.stack_size = sizeof(HR04SensorTrigTaBuffer),
		.cb_mem = &HR04SensorTrigTaControlBlock,
		.cb_size = sizeof(HR04SensorTrigTaControlBlock),
		.priority = (osPriority_t) osPriorityBelowNormal,
};

/* Definitions for HR04Sensor_task ECHO EMITTER*/
uint32_t HR04SensorEchoTaBuffer[ 256 ];
osStaticThreadDef_t HR04SensorEchoTaControlBlock;
osThreadId_t HR04SensorEcho_taskHandle;
const osThreadAttr_t HR04SensorEcho_task_attributes = {
		.name = "HR04SensorEcho_task",
		.stack_mem = &HR04SensorEchoTaBuffer[0],
		.stack_size = sizeof(HR04SensorEchoTaBuffer),
		.cb_mem = &HR04SensorEchoTaControlBlock,
		.cb_size = sizeof(HR04SensorEchoTaControlBlock),
		.priority = (osPriority_t) osPriorityBelowNormal,
};


/**
 * Waits for an echo event to be triggered
 * @param argument
 */
void HR04SensorEcho_task_Start(void *argument)
{
	evt_hr04_echo_sensor = osEventFlagsNew(NULL);
	uint32_t flagStatus;
	char msg[40];

	for(;;)
	{
		/* flag to let that processing task know when she's allowed to take a measure */
		flagStatus = osEventFlagsWait(evt_hr04_sensor, EVENT_HR04_ENABLE_MEASURE, osFlagsWaitAny, osWaitForever);

		flagStatus = osEventFlagsWait(evt_hr04_echo_sensor,
				EVENT_HR04_ECHO_SENSOR_1 | EVENT_HR04_ECHO_SENSOR_2,
				osFlagsWaitAny | osFlagsNoClear, osWaitForever);

		switch (flagStatus) {
		case EVENT_HR04_ECHO_SENSOR_1:
			/* loggerI("HR04 SENS1 ECHO received"); */
			osEventFlagsClear(evt_hr04_echo_sensor, EVENT_HR04_ECHO_SENSOR_1);
			/* sprintf(msg, "echo delay is %lu", HAL_GetTick() - currentSystick); */
			/* loggerI(msg); */
			break;
		case EVENT_HR04_ECHO_SENSOR_2:
			/* loggerI("HR04 SENS2 ECHO received"); */
			osEventFlagsClear(evt_hr04_echo_sensor, EVENT_HR04_ECHO_SENSOR_2);
			break;

		default:

			break;
		}
		osDelay(1);
	}

}
/**
 *	HR04 Trigger task, Sends TRIGGER
 *	every 100ms (osDelay) over to HR-04
 * @param argument
 */
void HR04SensorTrig_task_Start(void *argument)
{
	//HAL_TIM_Base_Start_IT(&htim1);

	for(;;)
	{
		/* prevent compilation warning */
		UNUSED(argument);

		/* send TRIG to every HR04 in the house for 10µs (equiv delay 10, set a timer for that) */

		osEventFlagsSet(evt_hr04_sensor, EVENT_HR04_ENABLE_MEASURE); /*enable measure in echo task */
		HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_SET);
		currentSystick = HAL_GetTick(); /* retrieves the current SysTick for further mleasurement of echo return delay */
		HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_RESET);


		osDelay(1); /* lets send that every 100ms only */
	}

}

/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize()
{
	/* creation of HR04Sensor TRIGGER_task */
	HR04SensorTrig_taskHandle = osThreadNew(HR04SensorTrig_task_Start, NULL, &HR04SensorTrig_task_attributes);
	if (!HR04SensorTrig_taskHandle) {
		loggerE("HR04 TRIG Task - Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor ECHO task */
	HR04SensorEcho_taskHandle = osThreadNew(HR04SensorEcho_task_Start, NULL, &HR04SensorEcho_task_attributes);
	if (!HR04SensorEcho_taskHandle) {
		loggerE("HR04 ECHO Task - Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HR04 Tasks - TRIG + ECHO Initialization complete");
	return (EXIT_SUCCESS);
}


