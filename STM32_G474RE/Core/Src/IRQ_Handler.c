/*
 * IRQ_Handler.c
 *
 *  Created on: Feb 26, 2020
 *      Author: jack
 */

#include "main.h"
#include "usart.h"
#include "freertos_logger_service.h"
#include "button_handler.h"
#include "cmsis_os2.h"
#include <string.h>
#include "gpio.h"
#include "tim.h"
#include <stdio.h>
#include "sensor_speed_service.h"
#include "sensor_hr04_service.h"
#include <stdbool.h>

char msg[50];

/**
 *
 * @param GPIO_Pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	char msg[30];

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
	//case HR04_1_ECHO_Pin:
	/* we received an event from the echo trigger */
	//loggerI("event received");
	//sprintf(msg, "event received");
	//HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
	//osEventFlagsSet(evt_hr04_sensor, EVENT_HR04_ECHO_SENSOR_1);
	//break;
	case B1_Pin:
		osEventFlagsSet(evt_usrbtn_id, BTN_PRESSED_FLAG);
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
	if (htim->Instance == TIM1) { /* HC-SR04 Sensor ONE */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) /* we read indirect mode only, gives the echo pulse width */
		{
			HR04_SensorsData.HR04_1_Distance = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) / MICROSECONDS_TO_CM;
		}
	} else if (htim->Instance == TIM2) { /* HC-SR04 Sensor ONE */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) /* we read indirect mode only, gives the echo pulse width */
		{
			HR04_SensorsData.HR04_2_Distance = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) / MICROSECONDS_TO_CM;
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
