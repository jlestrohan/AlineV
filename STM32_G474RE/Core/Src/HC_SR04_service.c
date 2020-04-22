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
 *		---------------------------------------------------------------------------------------------
 *		HC_SR04_SONAR_1		TIM1 	3				Dir1, Ind2			PC2			PC0
 *		HC_SR04_SONAR_2		TIM2	3				Dir1, Ind2			PB10		PA0
 *		HC_SR04_SONAR_3		TIM3	3				Dir1, Ind2			PB0			PA6
 *
 ****************************************************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include <HC_SR04_service.h>
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

HC_SR04_Result HC_SR04_StartupTimers();

/**
 *	HR04 Sensor 1 Task
 * @param argument
 */
static void HR04SensorTask_Start(void *argument)
{
	char msg[30];
	HR04_SensorsData_t HR04_SensorsData;
	osStatus_t status = -1;

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		osMessageQueueGet(queue_HC_SR04Handle, &HR04_SensorsData, NULL, osWaitForever); /* wait for message */
		if (status == osOK) {
			sprintf(msg, "%0*dcm      %0*dcm", 3,HR04_SensorsData.HR04_1_Distance, 3, HR04_SensorsData.HR04_2_Distance);

			osSemaphoreAcquire(sem_lcdService, osWaitForever);
			lcd_send_string(msg);
			osSemaphoreRelease(sem_lcdService);
			loggerI(msg);
		}

		osDelay(60);
	}
}

/**
 * Initialization function
 * @return
 */
uint8_t HC_SR04_initialize()
{
	queue_HC_SR04Handle = osMessageQueueNew(10, sizeof(HR04_SensorsData_t), NULL);
	if (!queue_HC_SR04Handle) {
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor1_task */
	HR04Sensor_taskHandle = osThreadNew(HR04SensorTask_Start, NULL, &HR04SensorTa_attributes); /* &HR04Sensor1_task_attributes); */
	if (!HR04Sensor_taskHandle) {
		//todo: improve error check routines here */
		loggerE("HR04 Sensor Task Initialization Failed");
		return (EXIT_FAILURE);
	}
	if (HC_SR04_StartupTimers() != HC_SR04_Result_Ok) {
		loggerE("HC_SR04 Timers Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing HC-SR04 Service... Success!");
	return (EXIT_SUCCESS);
}

/**
 * Starts up the different timers
 * @return
 */
HC_SR04_Result HC_SR04_StartupTimers()
{
	TIM_HandleTypeDef timHandlers[] = {htim1, htim2, htim3};

	for ( int i=0; i< HC_SR04_SONARS_CNT; i++) {
		/* starst up the different channels for Sensor 1 */
		if (HAL_TIM_PWM_Start(&*(timHandlers+i), TIM_CHANNEL_3) != HAL_OK) {
			return (HC_SR04_Result_TimerStart_Failed);
		}

		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_1) != HAL_OK) {
			return (HC_SR04_Result_TimerStart_Failed);
		}

		if (HAL_TIM_IC_Start_IT(&*(timHandlers+i), TIM_CHANNEL_2) != HAL_OK) {
			return (HC_SR04_Result_TimerStart_Failed);
		}
	}

	return (HC_SR04_Result_Ok);
}