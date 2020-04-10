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
#include <stdio.h>
#include "sensor_speed_service.h"
#include "sensor_hr04_service.h"
#include <stdbool.h>

char msg[50];

bool Channel1Edge = false;

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
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2) { /* TIM2 = Sensor 1 echo */
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) { /* rising edge */

			if(!Channel1Edge) {
				/* next capture will be on falling edge */
				__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);

				/* on first rising edge we just reset the counter */
				TIM2->CNT = 0;

				/* and we flip the flag so we can know next stage will be falling */
				Channel1Edge = true;

			} else  {

				/* on falling edge we just count how many microseconds elapsed since rising */
				HR04_SensorsData.echo_capture = HAL_TIM_ReadCapturedValue( htim, TIM_CHANNEL_1 );

				/* flip up the polarity for a next capture */
				__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);

				/* this is first sonar, let's record that */
				HR04_SensorsData.sonar_number = HR04_SONAR_1;

				osMessageQueuePut(queue_icValueHandle, &HR04_SensorsData, 0U, 0U);
				Channel1Edge = false;
			}
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
