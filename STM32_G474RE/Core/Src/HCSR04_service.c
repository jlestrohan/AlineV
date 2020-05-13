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
#include "tim.h"
#include "printf.h"
#include <string.h>
#include <stdio.h>

HR04_SensorsData_t HR04_SensorsData;
osMessageQueueId_t queue_HC_SR04Handle;

osMessageQueueId_t xQueueMg90sMotionOrder; /* extern */
xServoPosition_t xServoPosition; /* extern */

/* flag to set any sensors active/inactive according to nav control decisions */

typedef enum
{
	HC_SR04_Result_Ok = 0x00, /*!< Everything OK */
	HC_SR04_Result_Error, /*!< Unknown error */
	HC_SR04_Result_TimerStart_Failed,
	HC_SR04_Result_TaskInit_Failed,

} HC_SR04_Result;

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
HC_SR04_Result HC_SR04_StartupTimers();


/**
 *	HR04 Sensors Task
 * @param argument
 */
static void vHr04SensorTaskStart(void *argument)
{
	printf("Starting HCSR_04 Service task...\n\r");

	IrqHCSRAcquisition_t sensorData;

	osStatus_t status = -1;

	for (;;) {

		status = osMessageQueueGet(queue_HC_SR04Handle, &sensorData, NULL, osWaitForever); /* wait for message */
		if (status == osOK) {

			/* for the front sensor we store the right values at the right vars in HR04_SensorsData */
			if (sensorData.last_capt_sensor == HR04_SONAR_FRONT) {

				/* need data smoothing here, running average for now, see bayesian later */
				sensorData.last_capt_sensor = (sensorData.last_capt_sensor + HR04_SensorsData.dist_front)/2;

				switch (htim5.Instance->CCR1) {
				case SERVO_DIRECTION_LEFT45:
					HR04_SensorsData.dist_left45 = sensorData.last_capt_value;
#ifdef DEBUG_HCSR04_LEFT45
					printf("%d\n\r", HR04_SensorsData.dist_left45); //TODO: put this var in register keyword
#endif
					break;
				case SERVO_DIRECTION_CENTER:
					HR04_SensorsData.dist_front = sensorData.last_capt_value;
#ifdef DEBUG_HCSR04_FRONT
					//printf("F: %0*dcm\n\r", 3,HR04_SensorsData.dist_front);
					printf("%d\n\r", HR04_SensorsData.dist_front);
#endif
					break;
				case SERVO_DIRECTION_RIGHT45:
					HR04_SensorsData.dist_right45 = sensorData.last_capt_value;
#ifdef DEBUG_HCSR04_RIGHT45
					printf("%d\n\r", HR04_SensorsData.dist_right45);
#endif
					break;
				default: break;
				}
			} else if (sensorData.last_capt_sensor ==  HR04_SONAR_BOTTOM) {
				HR04_SensorsData.dist_bottom = sensorData.last_capt_value;
#ifdef DEBUG_HCSR04_BOTTOM
				printf("B: %0*dcm\n\r", 3, HR04_SensorsData.dist_bottom);
#endif
			} else if (sensorData.last_capt_sensor ==  HR04_SONAR_REAR) {
				HR04_SensorsData.dist_rear = sensorData.last_capt_value;
#ifdef DEBUG_HCSR04_REAR
				printf("R: %0*dcm\n\r", 3,HR04_SensorsData.dist_rear);
#endif
			}

		}

		osDelay(20);
	}
	osThreadTerminate(xHr04SensorTaskHandle);
}

/**
 * Initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit()
{
	queue_HC_SR04Handle = osMessageQueueNew(1, sizeof(HR04_SensorsData_t), NULL);
	if (!queue_HC_SR04Handle) {
		printf("HR04 Sensor Queue Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor1_task */
	xHr04SensorTaskHandle = osThreadNew(vHr04SensorTaskStart, NULL, &xHr04SensorTa_attributes);
	if (xHr04SensorTaskHandle == NULL) {
		printf("HR04 Sensor Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	if (HC_SR04_StartupTimers() != HC_SR04_Result_Ok) {
		printf("HC_SR04 Timers Initialization Failed\n\r");
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
HC_SR04_Result HC_SR04_StartupTimers()
{
	TIM_HandleTypeDef timHandlers[] = {htim1, htim2, htim3};

	for ( int i=0; i< HC_SR04_SONARS_CNT; i++) {

		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_1) != HAL_OK) {
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}

		if (HAL_TIM_IC_Start_IT(&*(timHandlers+i), TIM_CHANNEL_2) != HAL_OK) {
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}
		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_3) != HAL_OK) {
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}

	}

	return (HC_SR04_Result_Ok);
}

