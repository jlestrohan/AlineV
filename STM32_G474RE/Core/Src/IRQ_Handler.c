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
#include <stdio.h>
#include "sensor_speed_service.h"
#include "sensor_hr04_service.h"

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
		case SPDSens1_Pin:
			osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_1);
			break;
		case SPDSens2_Pin:
			osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_2);
			break;
		case SPDSens3_Pin:
			osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_3);
			break;
		case SPDSens4_Pin:
			osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_4);
			break;

			/**
			 * HR04 Sensors
			 */
		case HR04_1_ECHO_Pin:
			osEventFlagsSet(evt_hr04_echo_sensor, EVENT_HR04_ECHO_SENSOR_1);
			break;

		case B1_Pin:
			osEventFlagsSet(evt_usrbtn_id, BTN_PRESSED_FLAG);
			break;
		default:
			break;
	}

}

/**
 *
 * @param huart
 */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//HAL_GPIO_TogglePin(GPIOA, LD2_Pin);
//char *msg="char hello";
//HAL_UART_Transmit_IT(huart, (uint8_t *)msg, strlen(msg));
//cmd_parse_uart_cb(huart);
//}
