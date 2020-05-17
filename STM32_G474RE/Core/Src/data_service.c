/*******************************************************************
 * data_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *	Description:	Collects the data from the different sensors and
 *					prepares JSON to be sent by the serial service
 *					Several different sub-tasks send different data
 *					according to profile
 *
 *	using https://github.com/rafagafe/json-maker
 *******************************************************************/

#include "main.h"
#include <stdio.h>
#include <string.h>
#include "data_service.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include "printf.h"
#include "json-maker.h"
#include "esp32serial_service.h"

/* extern declared handlers to access data */

/** DATA CONTROL TASK **/
static osThreadId_t xDataControlTaskHandle;
static osStaticThreadDef_t xDataControlTaControlBlock;
static uint32_t xDataControlTaBuffer[256];
static const osThreadAttr_t xDataControlTa_attributes = {
		.name = "xDataControlTask",
		.stack_mem = &xDataControlTaBuffer[0],
		.stack_size = sizeof(xDataControlTaBuffer),
		.cb_mem = &xDataControlTaControlBlock,
		.cb_size = sizeof(xDataControlTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_DATA_CTRL
};

/** ATMOSPHERIC DATA TASK **/
static osThreadId_t xAtmosphericDataTaskHandle;
static osStaticThreadDef_t xAtmosphericDataTaControlBlock;
static uint32_t xAtmosphericDataTaBuffer[256];
static const osThreadAttr_t xAtmosphericDataTa_attributes = {
		.name = "xAtmosphericDataTask",
		.stack_mem = &xAtmosphericDataTaBuffer[0],
		.stack_size = sizeof(xAtmosphericDataTaBuffer),
		.cb_mem = &xAtmosphericDataTaControlBlock,
		.cb_size = sizeof(xAtmosphericDataTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_DATA_ATMOS
};

/**
 * Main Data Control task
 *
 * @param vParameter
 */
void vDataControlService_Start(void *vParameter)
{

	for(;;)
	{

		osDelay(10);
	}
	osThreadTerminate(NULL);
}

/**
 * Sends Atmospheric data JSON every 30 seconds
 * @param vParameter
 */
void vAtmosphericDataService_Start(void *vParameter)
{
	jsonMessage_t msg_pack;

	char msg[30] = "test sending frame";
	memcpy(msg_pack.json, (uint8_t *)msg, strlen(msg)+1);
	msg_pack.msg_size = strlen(msg);

	for (;;)
	{
		printf("Sending message to ESP32: %s\n\r", (char *)msg_pack.json);
		osMessageQueuePut(xQueueEspSerialTX, &msg_pack, 0U, osWaitForever);

		osDelay(1000);
	}
	osThreadTerminate(NULL);
}

/**
 * Main Initialization Routine
 * @return
 */
uint8_t uDataServiceinit()
{
	/* main data service task creation */
	xDataControlTaskHandle = osThreadNew(vDataControlService_Start, NULL, &xDataControlTa_attributes);
	if (xDataControlTaskHandle == NULL) {
		printf("Data Control Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/*atmospheric data service task creation */
	xAtmosphericDataTaskHandle = osThreadNew(vAtmosphericDataService_Start, NULL, &xAtmosphericDataTa_attributes);
	if (xAtmosphericDataTaskHandle == NULL) {
		printf("Atmospheric Data Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
