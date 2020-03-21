/*
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include <string.h>
#include <stdio.h>

osTimerId_t hr04_trig_tim;

/* Definitions for HR04Sensor_task */
typedef StaticTask_t osStaticThreadDef_t;
uint32_t HR04SensorTaBuffer[ 256 ];
osStaticThreadDef_t HR04SensorTaControlBlock;
osThreadId_t HR04Sensor_taskHandle;
const osThreadAttr_t HR04Sensor_task_attributes = {
		.name = "HR04Sensor_task",
		.stack_mem = &HR04SensorTaBuffer[0],
		.stack_size = sizeof(HR04SensorTaBuffer),
		.cb_mem = &HR04SensorTaControlBlock,
		.cb_size = sizeof(HR04SensorTaControlBlock),
		.priority = (osPriority_t) osPriorityBelowNormal,
};

/**
 * HR04 Trigger callback routine
 * @param argument
 */
static void hr04_trig_tim_cb (void *argument) {
	//int32_t arg = (int32_t)argument; // cast back argument '5'
	// do something, i.e. set thread/event flags
	loggerI("HR04Sensor TRIG Timer elapsed");
	/* HR04 Pi to low after 10µs delay to send trig signal has elapsed */
	HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_RESET);
}

/**
 *	main sensor task, receives event from IRQ
 * @param argument
 */
void HR04Sensor_task_Start(void *argument)
{
	// this tasks sends a trigger every 100ms
	uint32_t    timerDelay;                       // timer value
	osStatus_t  status;                           // function return status
	loggerI("HR04Sensor Task Started");
	if (!hr04_trig_tim) hr04_trig_tim = osTimerNew(hr04_trig_tim_cb, osTimerOnce, (void *)5, NULL);

	for(;;)
	{
		/* prevent compilation warning */
		UNUSED(argument);

		// 1 - send TRIG to pin PC5 for 10µs (equiv delay 10, set a timer for that)

		if (!osTimerIsRunning(hr04_trig_tim)) {
			timerDelay = 1000U;	/* 10µs */
			HAL_GPIO_WritePin(GPIOC, HR04_1_TRIG_Pin, GPIO_PIN_SET);
			status = osTimerStart(hr04_trig_tim, timerDelay);       // start timer
			if (status != osOK) {
				loggerE("TRIG Timer could not be started");
			} else loggerI("HR04Sensor TRIG Timer Started");
		}

		osDelay(100);
	}

}

/**
 * Initialization function
 */
uint8_t sensor_HR04_initialize()
{
	/* creation of HR04Sensor_task */
	HR04Sensor_taskHandle = osThreadNew(HR04Sensor_task_Start, NULL, &HR04Sensor_task_attributes);
	if (!HR04Sensor_taskHandle) {
		loggerE("HR04 Task - Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HR04 Task - Initialization complete");
	return (EXIT_SUCCESS);
}


// 2 - watch interrupt on pin PC6 to receive ECHO, calculate the time between both, need second task dedicated to that
//}
//}
