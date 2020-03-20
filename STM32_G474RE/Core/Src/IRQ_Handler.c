/*
 * IRQ_Handler.c
 *
 *  Created on: Feb 26, 2020
 *      Author: jack
 */

#include "main.h"
#include "button_handler.h"
#include "cmsis_os2.h"
#include <string.h>
#include "sensor_speed_service.h"

/**
 *
 * @param GPIO_Pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin) {
	case SPDSens1_Pin:
		osEventFlagsSet(evt_speed_sensor, EVENT_SPEED_SENSOR_1);
		//sensor_speed_IRQ_cb();
		break;

	default:
		buttonIRQ_cb();
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
