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

/* mutexed variables */
HR04_SensorsData_t HR04_SensorsData;
HR04_SensorsData_t HR04_OldSensorsData;
osMutexId_t mHR04_SensorsDataMutex;

osMessageQueueId_t xQueueMg90sMotionOrder; /* extern */
xServoPosition_t xServoPosition; /* extern */

/* flag to set any sensors active/inactive according to nav control decisions */

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

/* functions definitions */
static uint8_t HC_SR04_StartupTimers();


/**
 *	HR04 Sensors Task
 * @param argument
 */
static void vHr04SensorTaskStart(void *argument)
{
	printf("Starting HCSR_04 Service task...\n\r");

	hcSensorsTimersValue_t sensorCapuredData;
	osStatus_t status;

	for (;;) {

		status = osMessageQueueGet(queue_HC_SR04Handle, &sensorCapuredData, 0U, osWaitForever); /* wait for message */
		if (status == osOK) {
			osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
			HR04_OldSensorsData = HR04_SensorsData;

			HR04_SensorsData.dist_bottom = sensorCapuredData.bottom;
			HR04_SensorsData.dist_rear = sensorCapuredData.rear;

			/* need to know where to store all the front values */
			switch (xServoPosition) {
			case SERVO_DIRECTION_LEFT45:
				HR04_SensorsData.dist_left45 = sensorCapuredData.front;
				break;
			case SERVO_DIRECTION_CENTER: default:
				HR04_SensorsData.dist_front = sensorCapuredData.front;
				break;
			case SERVO_DIRECTION_RIGHT45:
				HR04_SensorsData.dist_right45 = sensorCapuredData.front;
				break;
			case SERVO_DIRECTION_LEFT90:
				HR04_SensorsData.dist_left90 = sensorCapuredData.front;
				break;
			case SERVO_DIRECTION_RIGHT90:
				HR04_SensorsData.dist_right90 = sensorCapuredData.front;
				break;
			}
#ifdef DEBUG_HCSR04_ALL
			printf("left45: %0*d - center: %0*d - right45: %0*d - bot: %0*d - rear: %0*d\n\r", 3,
					HR04_SensorsData.dist_left45, 3, HR04_SensorsData.dist_front, 3, HR04_SensorsData.dist_right45,
					3,HR04_SensorsData.dist_bottom, 3,HR04_SensorsData.dist_rear);
#endif
			osMutexRelease(mHR04_SensorsDataMutex);
		}

		osDelay(20);
	}
	osThreadTerminate(NULL);
}

/**
 * Initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit()
{
	/* create mutex for struct protection */
	mHR04_SensorsDataMutex = osMutexNew(NULL);

	queue_HC_SR04Handle = osMessageQueueNew(20, sizeof(hcSensorsTimersValue_t), NULL);
	if (!queue_HC_SR04Handle) {
		printf("HR04 Sensor Queue Initialization Failed\n\r");
		Error_Handler();
	}

	if (HC_SR04_StartupTimers() != EXIT_SUCCESS) {
		printf("HC_SR04 Timers Initialization Failed\n\r");
		Error_Handler();
	}

	/* creation of HR04Sensor1_task */
	xHr04SensorTaskHandle = osThreadNew(vHr04SensorTaskStart, NULL, &xHr04SensorTa_attributes);
	if (xHr04SensorTaskHandle == NULL) {
		printf("HR04 Sensor Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing HC-SR04 Service... Success!\n\r");
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

