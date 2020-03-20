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
 * Debouncing each wheel
 */
uint32_t 	lastWheelTick[4] = {0};

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
	sensorSpeedStatus = (SpeedSensorServiceTaHandle) ? sensorSpeedServiceInitOK : sensorSpeedServiceInitError;
	loggerI(sensorSpeedStatus == sensorSpeedServiceInitOK ? "Speed Sensor Initialization complete" : "Speed Sensor Initialization Failed");

	return (true);
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
		osEventFlagsWait(evt_speed_sensor, EVENT_SPEED_SENSOR_1, osFlagsWaitAny , osWaitForever);
		if (speedSensorDebounce(lastWheelTick[0]) || lastWheelTick[0] == 0) {
			lastWheelTick[0] = HAL_GetTick();
			loggerI("received speed sensor event");
		}

		osDelay(1); /* serves as debouncing as well */
	}
}



