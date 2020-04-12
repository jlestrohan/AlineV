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
 *		Sonar			Timer			PWM Channel		Echo Channels		Trig Pin		Echo Pin
 *		--------------------------------------------------------------------------------------------
 *		HR04_SONAR_1	TIM1 			3				Dir1, Ind2			PC2				PC0
 *		HR04_SONAR_2
 *
 ****************************************************************************************************
 */

#include "sensor_hr04_service.h"
#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include "tim.h"
#include "freertos_logger_service.h"
#include "lcd_service.h"
#include <string.h>
#include <stdio.h>

typedef enum
{
	HC_SR04_Result_Ok = 0x00, /*!< Everything OK */
	HC_SR04_Result_Error, /*!< Unknown error */
	HC_SR04_Result_TimerStart_Failed,
	HC_SR04_Result_TaskInit_Failed,

} HC_SR04_Result;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t HR04Sensor_taskHandle;
static osStaticThreadDef_t HR04SensorTaControlBlock;
static uint32_t HR04SensorTaBuffer[256];
static const osThreadAttr_t HR04SensorTa_attributes = {
		.name = "HR04SensorServiceTask",
		.stack_mem = &HR04SensorTaBuffer[0],
		.stack_size = sizeof(HR04SensorTaBuffer),
		.cb_mem = &HR04SensorTaControlBlock,
		.cb_size = sizeof(HR04SensorTaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };


/**
 *	HR04 Sensor 1 Task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	char msg[30];

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		sprintf(msg, "cm1: %d          ", HR04_SensorsData.HR04_1_Distance);

		osSemaphoreAcquire(sem_lcdService, osWaitForever);
		lcd_send_string(msg);
		osSemaphoreRelease(sem_lcdService);
		loggerI(msg);

		osDelay(60);
	}
}

/**
 * Initialization function
 * @return
 */
uint8_t sensor_HR04_initialize()
{
	/* creation of HR04Sensor1_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04SensorTa_attributes); /* &HR04Sensor1_task_attributes); */
	if (!HR04Sensor_taskHandle) {
		//todo: improve error check routines here */
		loggerE("HR04 Sensor Task Initialization Failed");
		return (EXIT_FAILURE);
	}
	/* starst up the different channels for Sensor 1 */
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) {
		loggerE("HR04 Error Could not start PWM Timer for Sensor 1");
		return (EXIT_FAILURE);
	}

	if (HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
		loggerE("HR04 Error Could not start ICDM Timer for Sensor 1");
		return (EXIT_FAILURE);
	}

	if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2) != HAL_OK) {
		loggerE("HR04 Error Could not start ICIM Timer for Sensor 1");
		return (EXIT_FAILURE);
	}


	loggerI("Initializing HC-SR04 Service... Success!");
	return (EXIT_SUCCESS);
}

