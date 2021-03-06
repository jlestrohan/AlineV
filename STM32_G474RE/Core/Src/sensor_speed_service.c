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

typedef enum
{
	SP_SENS_1, SP_SENS_2, SP_SENS_3, SP_SENS_4,
} speedSensorNum_t;
static speedSensorNum_t speedSensorNum;

typedef enum
{
	sensorSpeedServiceNotInit,
	sensorSpeedServiceInitOK,
	sensorSpeedServiceInitError
} sensorSpeedServiceStatus_t;
sensorSpeedServiceStatus_t sensorSpeedStatus = sensorSpeedServiceInitError;

/**
 * Definitions for SpeedSensorServiceTask, dynamic allocation
 */
osThreadId_t SpeedSensorServiceTaHandle;
const osThreadAttr_t SpeedSensorServiceTa_attributes = { .name =
        "SpeedSensorServiceTask", .priority = (osPriority_t) osPriorityLow };

/**
 * wheel structs
 */
typedef struct
{
	uint32_t currentWheelTick;
	uint32_t previousWheelTick;
	uint8_t wheelCurrentRPM;
	uint16_t wheelTicksCounter;
} wheelProps_t;
static wheelProps_t wheelProps[4];

void speedSensorService_task(void *argument);

/**
 * Initialize all speed sensors for the rover
 */
bool sensor_speed_initialize()
{
	/* creation of LoggerServiceTask */
	SpeedSensorServiceTaHandle = osThreadNew(speedSensorService_task, NULL,
	        &SpeedSensorServiceTa_attributes);
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
	uint32_t flagStatus;
	char msg[40];

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		//todo: have to set this for all 3 other sensors
		//todo use freertos timer to deduce rpm
		flagStatus = osEventFlagsWait(evt_speed_sensor,
		        EVENT_SPEED_SENSOR_1 | EVENT_SPEED_SENSOR_2
		                | EVENT_SPEED_SENSOR_3 | EVENT_SPEED_SENSOR_4,
		        osFlagsWaitAny | osFlagsNoClear, osWaitForever);

		switch (flagStatus)
		{
			case EVENT_SPEED_SENSOR_1:
				osEventFlagsClear(evt_speed_sensor, EVENT_SPEED_SENSOR_1);
				speedSensorNum = SP_SENS_1;
				break;
			case EVENT_SPEED_SENSOR_2:
				osEventFlagsClear(evt_speed_sensor, EVENT_SPEED_SENSOR_2);
				speedSensorNum = SP_SENS_2;
				break;
			case EVENT_SPEED_SENSOR_3:
				osEventFlagsClear(evt_speed_sensor, EVENT_SPEED_SENSOR_3);
				speedSensorNum = SP_SENS_3;
				break;
			case EVENT_SPEED_SENSOR_4:
				osEventFlagsClear(evt_speed_sensor, EVENT_SPEED_SENSOR_4);
				speedSensorNum = SP_SENS_4;
				break;
			default:
				break;
		}

		/* saves the current SysTick to fdurther measure */
		wheelProps[speedSensorNum].currentWheelTick = HAL_GetTick();
		wheelProps[speedSensorNum].wheelTicksCounter++;
		sprintf(msg, "Sensor number: %d - tick counts: %d", speedSensorNum,
		        wheelProps[speedSensorNum].wheelTicksCounter); //(wheelProps[0].wheelTicksCounter/20)*60); /* bad formula to be fixed */
		loggerI(msg);

		osDelay(1);
	}
}

