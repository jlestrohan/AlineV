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
#include "jwrite.h"
#include "esp32serial_service.h"
#include "command_service.h"
#include "BMP280_service.h"

static struct jWriteControl jwc;

/* function definitions */
uint8_t uEncodeJson(command_type_t cmd_type, jsonMessage_t *msg_pack);

/* extern declared vars */
BMP280_Data_t BMP280_Data;	/* extern */
osMutexId_t mBMP280_DataMutex;	/* extern */

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
static uint32_t xAtmosphericDataTaBuffer[512];
static const osThreadAttr_t xAtmosphericDataTa_attributes = {
		.name = "xAtmosphericDataTask",
		.stack_mem = &xAtmosphericDataTaBuffer[0],
		.stack_size = sizeof(xAtmosphericDataTaBuffer),
		.cb_mem = &xAtmosphericDataTaControlBlock,
		.cb_size = sizeof(xAtmosphericDataTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_DATA_ATMOS
};

/** NAVIGATION DATA TASK **/
static osThreadId_t xNavigationDataTaskHandle;
static osStaticThreadDef_t xNavigationDataTaControlBlock;
static uint32_t xNavigationDataTaBuffer[512];
static const osThreadAttr_t xNavigationDataTa_attributes = {
		.name = "xNavigationDataTask",
		.stack_mem = &xNavigationDataTaBuffer[0],
		.stack_size = sizeof(xNavigationDataTaBuffer),
		.cb_mem = &xNavigationDataTaControlBlock,
		.cb_size = sizeof(xNavigationDataTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_DATA_NAV
};


/**
 * Main Data Control task
 *
 * @param vParameter
 */
void vDataControlService_Start(void *vParameter)
{
	printf("Starting Data Control Service task.. Success!\n\r");

	for(;;)
	{
		/* queue to accept nav/system on request events */

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

	for (;;)
	{
		osDelay(10000); /* we start with a delay to give sensors a chance to be populated */

		uEncodeJson(CMD_TYPE_JSON_ATM, &msg_pack);
		osMessageQueuePut(xQueueEspSerialTX, &msg_pack, 0U, osWaitForever);

		osDelay(1);
	}
	osThreadTerminate(NULL);
}

/**
 * Sends Navigation data JSON automatically
 * @param vParameter
 */
void vNavigationDataTask_Start(void *vParameter)
{

	for (;;)
	{

		osDelay(10);
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

	/* navigation data service task creation */
	xNavigationDataTaskHandle = osThreadNew(vNavigationDataTask_Start, NULL, &xNavigationDataTa_attributes);
	if (xNavigationDataTaskHandle == NULL)
	{
		printf("Navigation Data Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

/**
 * Collects the different sensors/systems/nav datas on demand and build a json,
 * then copies it into the json buffer passed in reference
 * @param cmd_type
 * @return
 */
uint8_t uEncodeJson(command_type_t cmd_type, jsonMessage_t *msg_pack)
{
	//https://github.com/jonaskgandersson/jWrite/blob/master/main.c
	char buffer[1024];
	unsigned int buflen= 1024;
	int err;

	jwOpen( &jwc, buffer, buflen, JW_OBJECT, JW_COMPACT );  /* open root node as object */
	jwObj_string( &jwc, "uuid", "1256454565" );            /* STUB */
	jwObj_string( &jwc, "command_type", "ATM" );

	switch (cmd_type)
	{
	/* atmospheric stuff */
	case CMD_TYPE_JSON_ATM:
		jwObj_object(&jwc, "data");
		jwObj_string(&jwc, "sensor_type", "BMP280");
		osMutexAcquire(mBMP280_DataMutex, osWaitForever);
		jwObj_double(&jwc, "pressure", (double)BMP280_Data.pressure/100);
		jwObj_double(&jwc, "temperature", (double)BMP280_Data.temperature);
		osMutexRelease(mBMP280_DataMutex);
		jwEnd(&jwc);
		break;

	default: break;
	}

	err= jwClose(&jwc);                                  // close root object - done

	//printf("%s\n\r", buffer);
	uint8_t buffer_size = strlen(buffer);
	memcpy(msg_pack->json, buffer, buffer_size);
	msg_pack->msg_size = buffer_size;

	return EXIT_SUCCESS;
}
