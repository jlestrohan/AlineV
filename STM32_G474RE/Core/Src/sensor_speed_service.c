/*
 * sensor_speed.c
 *
 *  Created on: Mar 20, 2020
 *      Author: Jack Lestrohan
 */

// see https://www.teachmemicro.com/lm393-ir-module-motor-speed-sensor/

#include <stdlib.h>
#include <FreeRTOS.h>
#include "sensor_speed_service.h"
#include "freertos_logger_service.h"
#include <string.h>
#include <stdio.h>

#define SPEED_SENSOR_DEBOUNCE_MS		80

typedef enum {
	sensorSpeedServiceNotInit,
	sensorSpeedServiceInitOK,
	sensorSpeedServiceInitError
} sensorSpeedServiceStatus;
sensorSpeedServiceStatus sensorSpeedStatus = sensorSpeedServiceInitError;

/**
 * Definitions for SpeedSensorServiceTask
 */
typedef StaticTask_t osStaticThreadDef_t;
osThreadId_t SpeedSensorServiceTaHandle;
uint32_t SpeedSensorServiceTaBuffer[ 256 ];
osStaticThreadDef_t SpeedSensorServiceTaControlBlock;
const osThreadAttr_t SpeedSensorServiceTa_attributes = {
		.name = "SpeedSensorServiceTask",
		.stack_mem = &SpeedSensorServiceTaBuffer[0],
		.stack_size = sizeof(SpeedSensorServiceTaBuffer),
		.cb_mem = &SpeedSensorServiceTaControlBlock,
		.cb_size = sizeof(SpeedSensorServiceTaControlBlock),
		.priority = (osPriority_t) osPriorityLow,
};

/**
 * wheel structs
 */
typedef struct {
	uint32_t 	lastWheelTick;
	uint8_t		wheelCurrentRPM;
	uint16_t	wheelTicksCounter;
} wheelProps_t;
wheelProps_t wheelProps[3];

void speedSensorService_task(void *argument);

/**
 * returns true if not a bounce
 */
uint8_t speedSensorDebounce (uint32_t tick)
{
	return (HAL_GetTick() - tick > SPEED_SENSOR_DEBOUNCE_MS  ? true : false);
}

/**
 * Initialize all speed sensors for the rover
 */
bool sensor_speed_initialize()
{
	/* creation of LoggerServiceTask */
	SpeedSensorServiceTaHandle = osThreadNew(speedSensorService_task, NULL, &SpeedSensorServiceTa_attributes);
	if (!SpeedSensorServiceTaHandle) {
		sensorSpeedStatus = sensorSpeedServiceInitError;
		loggerE("Speed Sensor Service - Initialization Failure");
		return (EXIT_FAILURE);
	}

	loggerI("Speed Sensor Service - Initialization complete");
	return (EXIT_SUCCESS);
}

/**
 *	main sensor task, receives event from IRQ
 * @param argument
 */
void speedSensorService_task(void *argument)
{
	evt_speed_sensor = osEventFlagsNew(NULL);

	for(;;)
	{
		/* prevent compilation warning */
		UNUSED(argument);

		//todo: have to set this for all 3 other sensors
		//todo replace the fucking HAL_GetTick by a real freeros timer function
		osEventFlagsWait(evt_speed_sensor, EVENT_SPEED_SENSOR_1, osFlagsWaitAny , osWaitForever);

		if (speedSensorDebounce(wheelProps[0].lastWheelTick) ||
				wheelProps[0].lastWheelTick == 0) {

			wheelProps[0].lastWheelTick = HAL_GetTick();
			wheelProps[0].wheelTicksCounter++;
			char msg[20];
			snprintf(msg, sizeof(msg), "tick counts: %d",
					wheelProps[0].wheelTicksCounter); //(wheelProps[0].wheelTicksCounter/20)*60); /* bad formula to be fixed */
			loggerI(msg);
		}

		osDelay(1); /* serves as debouncing as well */
	}
}



