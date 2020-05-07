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
#include "tim.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

HR04_SensorsData_t HR04_SensorsData;

extern osMessageQueueId_t queue_HC_SR04Handle;
osEventFlagsId_t xHcrSr04ControlFlag;

/* flag to set any sensors active/inactive according to nav control decisions */

typedef enum
{
	HC_SR04_Result_Ok = 0x00, /*!< Everything OK */
	HC_SR04_Result_Error, /*!< Unknown error */
	HC_SR04_Result_TimerStart_Failed,
	HC_SR04_Result_TaskInit_Failed,

} HC_SR04_Result;

typedef StaticTask_t osStaticThreadDef_t;

static osThreadId_t xHr04SensorTaskHandle;
static osStaticThreadDef_t xHr04SensorTaControlBlock;
static uint32_t xHr04SensorTaBuffer[512];
static const osThreadAttr_t xHr04SensorTa_attributes = {
		.stack_mem = &xHr04SensorTaBuffer[0],
		.stack_size = sizeof(xHr04SensorTaBuffer),
		.name = "xHr04SensorServiceTask",
		.cb_size = sizeof(xHr04SensorTaControlBlock),
		.cb_mem = &xHr04SensorTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_HCSR04, };

static osThreadId_t xHr04ControlTaskHandle;
static osStaticThreadDef_t xHr04ControlTaControlBlock;
static uint32_t xHr04ControlTaBuffer[512];
static const osThreadAttr_t xHr04ControlTa_attributes = {
		.stack_mem = &xHr04ControlTaBuffer[0],
		.stack_size = sizeof(xHr04ControlTaBuffer),
		.name = "xHr04ControlServiceTask",
		.cb_size = sizeof(xHr04ControlTaControlBlock),
		.cb_mem = &xHr04ControlTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_HCSR04_CTL, };

HC_SR04_Result HC_SR04_StartupTimers();

/**
 * HC-SR045 Control Task - This task only controls the activation of the sensors
 * Usually driven by a cruise control service
 * @param argument
 */
static void vHr04ControlTaskStart(void *argument)
{
	dbg_printf("Starting HCSR_04 Control task...");

	for (;;) {
		if (osEventFlagsGet(xHcrSr04ControlFlag) && FLG_SONAR_FRONT_ACTIVE) {
			HAL_TIM_PWM_Start(&htim2,  TIM_CHANNEL_ALL);
		} else {
			HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_ALL);
		}

		if (osEventFlagsGet(xHcrSr04ControlFlag) && FLG_SONAR_REAR_ACTIVE) {
			HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_ALL);
		} else {
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_ALL);
		}

		if (osEventFlagsGet(xHcrSr04ControlFlag) && FLG_SONAR_BOTTOM_ACTIVE) {
			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_ALL);
		} else {
			HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_ALL);
		}

		osDelay(50);
	}
	osThreadTerminate(xHr04ControlTaskHandle);
}

/**
 *	HR04 Sensors Task
 * @param argument
 */
static void vHr04SensorTaskStart(void *argument)
{
	dbg_printf("Starting HCSR_04 Service task...");

	osStatus_t status = -1;

	for (;;) {

		status = osMessageQueueGet(queue_HC_SR04Handle, &HR04_SensorsData, NULL, osWaitForever); /* wait for message */
		if (status == osOK) {

#ifdef HCSR04_FRONT_ENABLE
			if (HR04_SensorsData.sonarNum == HR04_SONAR_FRONT) {
				dbg_printf("FRONT: %d - %0*dcm", HR04_SensorsData.sonarNum, 3,HR04_SensorsData.distance);
			}
#endif
#ifdef HCSR04_BOTTOM_ENABLE
			if (HR04_SensorsData.sonarNum == HR04_SONAR_BOTTOM) {
				dbg_printf("BOTTOM: %d - %0*dcm", HR04_SensorsData.sonarNum, 3,HR04_SensorsData.distance);
			}
#endif
#ifdef HCSR04_REAR_ENABLE
			if (HR04_SensorsData.sonarNum == HR04_SONAR_REAR) {
				dbg_printf("REAR: %d - %0*dcm", HR04_SensorsData.sonarNum, 3,HR04_SensorsData.distance);
			}
#endif

		}

		osDelay(1);
	}
	osThreadTerminate(xHr04SensorTaskHandle);
}

/**
 * Initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit()
{
	queue_HC_SR04Handle = osMessageQueueNew(10, sizeof(HR04_SensorsData_t), NULL);
	if (!queue_HC_SR04Handle) {
		dbg_printf("HR04 Sensor Queue Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* definition of evt_HCSR04ControlFlag */
	xHcrSr04ControlFlag = osEventFlagsNew(NULL);
	if (xHcrSr04ControlFlag == NULL) {
		dbg_printf("HCSR04 Event Flag Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of HR04Sensor1_task */
	xHr04SensorTaskHandle = osThreadNew(vHr04SensorTaskStart, NULL, &xHr04SensorTa_attributes);
	if (xHr04SensorTaskHandle == NULL) {
		dbg_printf("HR04 Sensor Task Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of HR04Control_task */
	xHr04ControlTaskHandle = osThreadNew(vHr04ControlTaskStart, NULL, &xHr04ControlTa_attributes);
	if (xHr04ControlTaskHandle == NULL) {
		dbg_printf("HR04 Control Task Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}
	if (HC_SR04_StartupTimers() != HC_SR04_Result_Ok) {
		dbg_printf("HC_SR04 Timers Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	dbg_printf("Initializing HC-SR04 Service... Success!");
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
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}

		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_1) != HAL_OK) {
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}

		if (HAL_TIM_IC_Start_IT(&*(timHandlers+i), TIM_CHANNEL_2) != HAL_OK) {
			Error_Handler();
			return (HC_SR04_Result_TimerStart_Failed);
		}
	}

	return (HC_SR04_Result_Ok);
}
