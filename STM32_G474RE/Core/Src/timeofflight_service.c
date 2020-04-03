/*
 * timeofflight_service.c
 *
 *  Created on: Apr 3, 2020
 *      Author: aez03
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "timeofflight_service.h"
#include "freertos_logger_service.h"

//static osStatus_t osStatus;

typedef enum
{
	 timeofflightServiceNotInit,
	 timeofflightServiceInitOK,
	 timeofflightServiceInitError
}  timeofflightServiceStatus_t;
static  timeofflightServiceStatus_t  timeofflightServiceStatus =  timeofflightServiceNotInit;

/**
 * Definitions for lcdServiceTask
 */
typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t timeofflightServiceTaHandle;
static uint32_t timeofflightServiceTaBuffer[256];
static osStaticThreadDef_t timeofflightServiceTaControlBlock;
static const osThreadAttr_t timeofflightServiceTa_attributes = {
		.name = "timeofflightServiceTask",
		.stack_mem = &timeofflightServiceTaBuffer[0],
		.stack_size = sizeof(timeofflightServiceTaBuffer),
		.cb_mem = &timeofflightServiceTaControlBlock,
		.cb_size = sizeof(timeofflightServiceTaControlBlock),
		.priority = (osPriority_t) osPriorityLow, };

/**
 * timeofflight main task
 * @param argument
 */
void timeofflightService_task(void *argument)
{
	loggerI("Starting timeofflight task...");
	for (;;) {


		osDelay(10);
	}
}

/**
 *
 */
uint8_t timeofflight_initialize()
{
	/* creation of timeofflightServiceTask */
	timeofflightServiceTaHandle = osThreadNew(timeofflightService_task, NULL, &timeofflightServiceTa_attributes);
	if (!timeofflightServiceTaHandle) {
		timeofflightServiceStatus = timeofflightServiceInitError;
		loggerE("Initializing timeofflight Service - Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing timeofflight Service - Success!");
	return (EXIT_SUCCESS);
}
