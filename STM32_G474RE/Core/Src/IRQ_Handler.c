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
#include "esp32serial_service.h"
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
osMessageQueueId_t xQueueStm32RXserial;
static uint8_t rxData;

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
		osEventFlagsSet(xEventOnBoardButton, B_ONBOARD_PRESSED_FLAG);
		break;
	case B2_Pin:
		/* menu navigation button. We need to pass it to the button service first to debounce */
		/* no direct flag raise to the menu switching task because of that */
		osEventFlagsSet(xEventButtonExt, B_EXT_PRESSED_FLAG);
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
/*void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	loggerI("period elapsed");
}*/

/**
 * @brief  Rx Transfer completed callback.
 * @param  huart UART handle.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if (huart->Instance == USART3) {
		osMessageQueuePut(xQueueStm32RXserial, &rxData, 0U, 0U);
		/* relaunch interrupt mode */
		if (HAL_UART_Receive_DMA(&huart3, (uint8_t *)&rxData, 1) != HAL_OK)
		{
			Error_Handler();
		}


	}

}
