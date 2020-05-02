/*******************************************************************
 * IRQ_Handler.c
 *
 *  Created on: Feb 26, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/


#include <stdio.h>
#include <FreeRTOS.h>
#include <string.h>
#include <stdbool.h>

#include "button_service.h"
#include "main.h"
#include "usart.h"
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include "HCSR04_service.h"
#include "gpio.h"
#include "tim.h"
#include "sensor_speed_service.h"


char msg[50];

/**
 *
 * @param GPIO_Pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	switch (GPIO_Pin)
	{
	/**
	 * Speed sensors
	 */
	/*case SPDSens1_Pin:
		osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_1);
		break;*/
	/*case SPDSens2_Pin:
		osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_2);
		break;
	case SPDSens3_Pin:
		osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_3);
		break;
	case SPDSens4_Pin:*=
		osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_4);
		break;*/

	/**
	 * HR04 Sensors
	 */
	case B1_Pin:
		osEventFlagsSet(xEventOnBoardButton, B1_PRESSED_FLAG);
		break;
	case B2_Pin:
			osEventFlagsSet(xEventButton2, B2_PRESSED_FLAG);
			break;
	default:
		break;
	}

}

/**
 * Capture Callback
 * Reads value received from duty high on echo response
 * @param htim
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	HR04_SensorsData_t HR04_Sensors;

	if (htim->Instance == TIM1) { /* HC-SR04 Sensor ONE */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) /* we read indirect mode only, gives the echo pulse width */
		{
			HR04_Sensors.distance = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) / MICROSECONDS_TO_CM;
			HR04_Sensors.sonarNum = HR04_SONAR_1;
			osMessageQueuePut(queue_HC_SR04Handle, &HR04_Sensors, 0U, 0U);
		}
	} else if (htim->Instance == TIM2) { /* HC-SR04 Sensor ONE */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) /* we read indirect mode only, gives the echo pulse width */
		{
			HR04_Sensors.distance = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) / MICROSECONDS_TO_CM;
			HR04_Sensors.sonarNum = HR04_SONAR_2;
			osMessageQueuePut(queue_HC_SR04Handle, &HR04_Sensors, 0x0U, 0U);
		}
	} else if (htim->Instance == TIM3) { /* HC-SR04 Sensor ONE */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) /* we read indirect mode only, gives the echo pulse width */
		{
			HR04_Sensors.distance = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) / MICROSECONDS_TO_CM;
			HR04_Sensors.sonarNum = HR04_SONAR_3;
			osMessageQueuePut(queue_HC_SR04Handle, &HR04_Sensors, 0U, 0U);
		}
	}
}

/**
 * Timers Elapsed
 * @param htim
 * keep as __weak as an instance lies in main.c already (generated code)
 */
__weak void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	loggerI("period elapsed");
}

/**
 *
 * @param huart
 */
/* void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) */
/* { */
/* HAL_GPIO_TogglePin(GPIOA, LD2_Pin); */
/* char *msg="char hello"; */
/* HAL_UART_Transmit_IT(huart, (uint8_t *)msg, strlen(msg)); */
/* cmd_parse_uart_cb(huart); */
/* } */
