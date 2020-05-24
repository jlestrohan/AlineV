/**
 *****************************************************************************************************
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *
 *	Timers and Pinout:
 *
 *		Sonar				Timer	PWM Channel		Echo Channels		Trig Pin	Echo Pin
 *		--------------------------------------------------------------------------------------------------
 *		HC_SR04_SONAR_REAR		TIM1 	3				Dir1, Ind2			PC2			PC0		REAR
 *		HC_SR04_SONAR_FRONT		TIM2	3				Dir1, Ind2			PB10		PA0		FRONT
 *		HC_SR04_SONAR_BOTTOM	TIM3	3				Dir1, Ind2			PB0			PA6		BOTTOM
 *
 *********************************************************************************************************
 */

#include <stdlib.h>
#include "configuration.h"
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include <HCSR04_service.h>
#include "MG90S_service.h"
#include "IRQ_Handler.h"
#include "tim.h"
#include "printf.h"
#include <string.h>
#include <stdio.h>


/* orders flags */
osEventFlagsId_t xEventFlagHCSR04Orders;

/* mutexed variables */
HR04_SensorsData_t HR04_SensorsData;
HR04_SensorsData_t HR04_OldSensorsData;
osMutexId_t mHR04_SensorsDataMutex;

osMessageQueueId_t xQueueMg90sMotionOrder; /* extern */
xServoPosition_t xServoPosition; /* extern */

/* flag to set any sensors active/inactive according to nav control decisions */
//#ifdef DEBUG_HCSR04_ALL
static osThreadId_t xHr04SensorTaskHandle;
static osStaticThreadDef_t xHr04SensorTaControlBlock;
static uint32_t xHr04SensorTaBuffer[256];
static const osThreadAttr_t xHr04SensorTa_attributes = {
		.stack_mem = &xHr04SensorTaBuffer[0],
		.stack_size = sizeof(xHr04SensorTaBuffer),
		.name = "xHr04SensorServiceTask",
		.cb_size = sizeof(xHr04SensorTaControlBlock),
		.cb_mem = &xHr04SensorTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_HCSR04, };
//#endif

/* functions definitions */
static uint8_t HC_SR04_StartupTimers();


//#ifdef DEBUG_HCSR04_ALL
/**
 *	HR04 Sensors Task
 * @param argument
 */
static void vHr04SensorTaskStart(void *argument)
{
	printf("Starting HCSR_04 Service task...\n\r");

	//hcSensorsTimersValue_t sensorCapuredData;
	//osStatus_t status;
	uint32_t flags;

	for (;;) {

		flags = osEventFlagsWait(xEventFlagHCSR04Orders, EVT_HCSR_FLAG_RESET, osFlagsWaitAny, osWaitForever);

		if (flags && EVT_HCSR_FLAG_RESET) {
			osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
			HR04_SensorsData = (HR04_SensorsData_t){0};
			HR04_SensorsData = (HR04_SensorsData_t){0};

			osMutexRelease(mHR04_SensorsDataMutex);
		}

		/*printf("left45: %0*d - center: %0*d - right45: %0*d - bot: %0*d - rear: %0*d\n\r", 3,
				HR04_SensorsData.dist_left45, 3, HR04_SensorsData.dist_front, 3, HR04_SensorsData.dist_right45,
				3,HR04_SensorsData.dist_bottom, 3,HR04_SensorsData.dist_rear);*/

		//osMutexRelease(mHR04_SensorsDataMutex);

		osDelay(1);
	}
	osThreadTerminate(NULL);
}
//#endif

/**
 * Initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit()
{
	/* create mutex for struct protection */
	mHR04_SensorsDataMutex = osMutexNew(NULL);

	xEventFlagHCSR04Orders = osEventFlagsNew(NULL);
	if (xEventFlagHCSR04Orders == NULL) {
		printf("Error Initializing xEventFlagHCSR04Orders HCSR...\n\r");
		Error_Handler();
		return EXIT_FAILURE;
	}

	/* creation of HR04Sensor1_task */
	xHr04SensorTaskHandle = osThreadNew(vHr04SensorTaskStart, NULL, &xHr04SensorTa_attributes);
	if (xHr04SensorTaskHandle == NULL) {
		printf("HR04 Sensor Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing HC-SR04 Service... Success!\n\r");

	if (HC_SR04_StartupTimers() != EXIT_SUCCESS) {
		printf("HC_SR04 Timers Initialization Failed\n\r");
		Error_Handler();
	}

	return (EXIT_SUCCESS);
}

/**
 * Starts up the different timers
 * @return
 */
static uint8_t HC_SR04_StartupTimers()
{
	TIM_HandleTypeDef timHandlers[] = {htim1, htim2, htim3};

	for ( int i=0; i< HC_SR04_SONARS_CNT; i++) {
		/* starst up the different channels for Sensor 1 */
		if (HAL_TIM_PWM_Start(&*(timHandlers+i), TIM_CHANNEL_3) != HAL_OK) {
			return (EXIT_FAILURE);
		}

		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_1) != HAL_OK) {
			return (EXIT_FAILURE);
		}

		if (HAL_TIM_IC_Start_IT(&*(timHandlers+i), TIM_CHANNEL_2) != HAL_OK) {
			return (EXIT_FAILURE);
		}
	}

	return (EXIT_SUCCESS);
}

