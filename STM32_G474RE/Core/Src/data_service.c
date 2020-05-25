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
#include <stdint.h>
#include "printf.h"
#include "jwrite.h"
#include "esp32serial_service.h"
#include "configuration.h"
#include "command_service.h"
#include "BMP280_service.h"
#include "usart.h"
#include "CMPS12_service.h"
#include "uvLed_service.h"
#include "HCSR04_service.h"
#include "MotorsControl_service.h"
#include "navControl_service.h"


static struct jWriteControl jwc;


unsigned long *id = (unsigned long *)0x1FFF7590;

/* extern declared vars */
BMP280_Data_t BMP280_Data;	/* extern */
osMutexId_t mBMP280_DataMutex;	/* extern */

CMPS12_SensorData_t CMPS12_SensorData;
osMutexId_t mCMPS12_SensorDataMutex;

extern HR04_SensorsData_t HR04_SensorsData;		/* always hold the current values on every field */
extern osMutexId_t mHR04_SensorsDataMutex;

MotorData_t MotorData;
osMutexId_t mMotorDataMutex;

extern UV_LedStatus_t uUVLedStatus;
extern osMutexId_t mUvLedStatusMutex;

extern NavigationStatus_t xCurrentNavStatus;
extern osMutexId_t mCurrentNavStatusMutex;

/**
 * Used in NavData data service
 */
typedef struct {
	HR04_SensorsData_t hcsrData;
	MotorData_t Motordata;
	CMPS12_SensorData_t cmps12Data;
	UV_LedStatus_t uvLedStatus;
} NAV_Data_t;

/* function definitions */
static uint8_t uEncodeJson(command_type_t cmd_type, jsonMessage_t *msg_pack, NAV_Data_t *nav_data);
static void vAtmosphericDataService_Start(void *vParameter);
static uint8_t eq(NAV_Data_t *one, NAV_Data_t *two);

osMessageQueueId_t xQueueEspSerialTX; /* extern */

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
static void vDataControlService_Start(void *vParameter)
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
static void vAtmosphericDataService_Start(void *vParameter)
{
	jsonMessage_t msg_pack;

	for (;;)
	{
		osDelay(5000);  /* we start with a delay to give sensors a chance to be populated */

		if (uEncodeJson(CMD_TYPE_JSON_ATM, &msg_pack, NULL) == EXIT_SUCCESS) {
#ifdef DEBUG_DATA_CONTROL
			printf("%.*s", msg_pack.msg_size, (char *)msg_pack.json);
#endif
			if (xQueueEspSerialTX != NULL)
				osMessageQueuePut(xQueueEspSerialTX, &msg_pack, 0U, osWaitForever);
		} else {
			printf("error uEncodeJSON");
		}

		osDelay(25000); /* this will make 30 seconds */
	}
	osThreadTerminate(NULL);
}

/**
 * Sends Navigation data JSON automatically 4 times a second ONLy when new data has changed vs previous data
 * @param vParameter
 */
static void vNavigationDataTask_Start(void *vParameter)
{

	NAV_Data_t xCurrentNavData, xLastNavData;
	jsonMessage_t msg_pack; /* packet to be sent out */

	for (;;)
	{
		/* we constantly update this structure NAV_Data_t containing all the infos we need for the JSON, we send it 2 times a second
		 * unless nothing has changed vs the previous record. It is easier and faster to compare them structs then */
		MUTEX_MOTORDATA_TAKE
		xCurrentNavData.Motordata = MotorData;
		MUTEX_MOTORDATA_GIVE

		MUTEX_CMPS12_TAKE
		xCurrentNavData.cmps12Data = CMPS12_SensorData;
		MUTEX_CMPS12_GIVE

		MUTEX_HCSR04_TAKE
		xCurrentNavData.hcsrData = HR04_SensorsData;
		MUTEX_HCSR04_GIVE

		MUTEX_UVLED_TAKE
		xCurrentNavData.uvLedStatus = uUVLedStatus;
		MUTEX_UVLED_GIVE

		/* only if we are not in certain idle modes */
		MUTEX_NAVSTATUS_TAKE
		if (xCurrentNavStatus != NAV_STATUS_IDLE) {
			MUTEX_NAVSTATUS_GIVE /* releasing asap */
			/* we compare bothe structures to see if anything has changed */
			if (!eq(&xCurrentNavData, &xLastNavData)) {
				/* we send JSON here!! */
				if (uEncodeJson(CMD_TYPE_JSON_NAV, &msg_pack, &xCurrentNavData) == EXIT_SUCCESS) {
#ifdef DEBUG_DATA_CONTROL
					printf("%.*s", msg_pack.msg_size, (char *)msg_pack.json);
#endif
					if (xQueueEspSerialTX != NULL)
						osMessageQueuePut(xQueueEspSerialTX, &msg_pack, 0U, osWaitForever);
				} else {
					printf("error uEncodeJSON");
				}

				/* we copy the current struct to the backup */
				xLastNavData = xCurrentNavData;
			}
		}
		MUTEX_NAVSTATUS_GIVE /* releasing anyway */

		osDelay(500); /* twice a second if data HAS changed */
	}
	osThreadTerminate(NULL);
}

/**
 * Main Initialization Routine
 * @return
 */
uint8_t uDataServiceinit()
{
	/*atmospheric data service task creation */
	xAtmosphericDataTaskHandle = osThreadNew(vAtmosphericDataService_Start, NULL, &xAtmosphericDataTa_attributes);
	if (xAtmosphericDataTaskHandle == NULL) {
		printf("Atmospheric Data Initialization Failed\n\r");
		Error_Handler();
		osThreadTerminate(NULL);
	}

	/* main data service task creation */
	xDataControlTaskHandle = osThreadNew(vDataControlService_Start, NULL, &xDataControlTa_attributes);
	if (xDataControlTaskHandle == NULL) {
		printf("Data Control Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* navigation data service task creation */
	xNavigationDataTaskHandle = osThreadNew(vNavigationDataTask_Start, NULL, &xNavigationDataTa_attributes);
	if (xNavigationDataTaskHandle == NULL)
	{
		printf("Navigation Data Task Initialization Failed\n\r");
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
static uint8_t uEncodeJson(command_type_t cmd_type, jsonMessage_t *msg_pack, NAV_Data_t *nav_data)
{
	//https://github.com/jonaskgandersson/jWrite/blob/master/main.c
	char buffer[MAX_JSON_MSG_SIZE+1];
	uint8_t err;

	/* formats the unique id */
	char uuid[40];
	sprintf(uuid, "%lu-%lu-%lu", id[0], id[1], id[2]);

	jwOpen( &jwc, buffer, MAX_JSON_MSG_SIZE, JW_OBJECT, JW_COMPACT );  /* open root node as object */
	jwObj_string( &jwc, "id", uuid );

	switch (cmd_type)
	{
	/*********************************************/
	/* ATMOSPHERIC STUFF */
	case CMD_TYPE_JSON_ATM:
		jwObj_string( &jwc, "cmd", "ATM" );
		jwObj_object(&jwc, "data");
		jwObj_string(&jwc, "sns", "BME280");

		MUTEX_BME280_TAKE
		if (BMP280_Data.pressure > 0 || BMP280_Data.temperature > 0) {
			jwObj_double(&jwc, "Ps", (double)BMP280_Data.pressure/100);
			jwObj_double(&jwc, "Tp", (double)BMP280_Data.temperature);
			jwObj_double(&jwc, "Hm", (double)BMP280_Data.humidity);
		} else return EXIT_FAILURE;
		MUTEX_BME280_GIVE

		jwEnd(&jwc);
		break;

		/*********************************************/
		/* NAVIGATION STUFF */
	case CMD_TYPE_JSON_NAV:
		jwObj_string( &jwc, "cmd", "NAV" );
		jwObj_object(&jwc, "data");

		MUTEX_MOTORDATA_TAKE
		jwObj_int(&jwc, "mtSpL", nav_data->Motordata.currentSpeedLeft);
		jwObj_int(&jwc, "mtSpR", nav_data->Motordata.currentSpeedRight);
		jwObj_int(&jwc, "mtMotL", nav_data->Motordata.motorMotion_Left);
		jwObj_int(&jwc, "mtMotR", nav_data->Motordata.motorMotion_Right);
		MUTEX_MOTORDATA_GIVE

		MUTEX_CMPS12_TAKE
		jwObj_int(&jwc, "cmPi", nav_data->cmps12Data.PitchAngle);
		jwObj_int(&jwc, "cmRo", nav_data->cmps12Data.RollAngle);
		jwObj_int(&jwc, "cmHdg", nav_data->cmps12Data.CompassBearing);
		MUTEX_CMPS12_GIVE

		MUTEX_HCSR04_TAKE
		jwObj_int(&jwc, "hcFr", nav_data->hcsrData.dist_front);
		jwObj_int(&jwc, "hcRr", nav_data->hcsrData.dist_rear);
		jwObj_int(&jwc, "hcBt", nav_data->hcsrData.dist_bottom);
		MUTEX_HCSR04_GIVE

		MUTEX_UVLED_TAKE
		jwObj_bool(&jwc, "Uv", nav_data->uvLedStatus);
		MUTEX_UVLED_GIVE

		jwEnd(&jwc);
		break;

	default: break;
	}

	err = jwClose(&jwc);                                  // close root object - done
	if (err != JWRITE_OK) {
		printf("%s", err);
	}

	size_t buffer_size = strlen(buffer);
	memcpy(msg_pack->json, buffer, buffer_size);

	//msg_pack->json[buffer_size+1] = 0x7E;
	msg_pack->msg_size = buffer_size;
	return EXIT_SUCCESS;
}

/**
 * Compare NAV_Data_t A and NAV_Data_t B, return false if non equal
 * VERY DULL but the only way :/
 * @param one
 * @param two
 * @return
 */
uint8_t eq(NAV_Data_t *one, NAV_Data_t *two)
{
	/* NOW the DULL code is coming .... We check each member for equality ... */
	return
			((one->cmps12Data.CompassBearing == two->cmps12Data.CompassBearing) &&
					(one->cmps12Data.PitchAngle == two->cmps12Data.PitchAngle) &&
					(one->cmps12Data.RollAngle == two->cmps12Data.RollAngle) &&

					(one->Motordata.currentSpeedLeft == two->Motordata.currentSpeedLeft) &&
					(one->Motordata.currentSpeedRight == two->Motordata.currentSpeedRight) &&
					(one->Motordata.motorMotion_Left == two->Motordata.motorMotion_Left) &&
					(one->Motordata.motorMotion_Right == two->Motordata.motorMotion_Right) &&

					(one->hcsrData.dist_bottom == two->hcsrData.dist_bottom) &&
					(one->hcsrData.dist_front == two->hcsrData.dist_front) &&

					(one->uvLedStatus == two->uvLedStatus));
}
