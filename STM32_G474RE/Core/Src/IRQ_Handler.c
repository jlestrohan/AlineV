/*******************************************************************
 * IRQ_Handler.c
 *
 *  Created on: Feb 26, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/


#include <stdio.h>
#include "IRQ_Handler.h"
#include <FreeRTOS.h>
#include <string.h>
#include <stdbool.h>
#include "esp32serial_service.h"
#include "UartRingbuffer.h"
#include "button_service.h"
#include "main.h"
#include "usart.h"
#include "printf.h"
#include "cmsis_os2.h"
#include "gpio.h"
#include "tim.h"
#include "SystemInfos.h"
#include "MG90S_service.h"

char msg[50];

uint32_t ADC_BUF[3]; /* extern */
//uint8_t UART_DMA_BUF[4]; /* extern */

osMessageQueueId_t xQueueDmaAdcInternalSensors;
DMAInternalSensorsAdcValues_t DMAInternalSensorsAdcValues; /* extern */
uint8_t UartRXDmaBuffer[10]; /* extern */
osMessageQueueId_t xQueueEspSerialRX; /* extern */

xServoPosition_t xServoPosition; /* extern */
osMutexId_t mServoPositionMutex;

osMessageQueueId_t xQueueButtonEvent; /* extern */

HR04_SensorsData_t HR04_SensorsData;
osMutexId_t mHR04_SensorsDataMutex;


/**
 *
 * @param GPIO_Pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	ButtonEvent_t btn_event;

	switch (GPIO_Pin)
	{
	/**
	 * HR04 Sensors
	 */
	case B1_Pin:
		btn_event = BTN_BUTTON_ONBOARD_PRESSED;
		osMessageQueuePut(xQueueButtonEvent, &btn_event, 0U, 0U);
		break;

	case B2_Pin:
		/* menu navigation button. We need to pass it to the button service first to debounce */
		/* no direct flag raise to the menu switching task because of that */
		btn_event = BTN_BUTTON_EXT_PRESSED;
		osMessageQueuePut(xQueueButtonEvent, &btn_event, 0U, 0U);
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

	if (htim->Instance == TIM1) { /* HC-SR04 Sensor REAR */
		osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
		HR04_SensorsData.dist_rear = htim->Instance->CCR2 / MICROSECONDS_TO_CM;
		osMutexRelease(mHR04_SensorsDataMutex);

	}
	else if (htim->Instance == TIM2)  /* HC-SR04 Sensor FRONT */
	{
		osMutexAcquire(mServoPositionMutex, osWaitForever);
		osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
		switch (xServoPosition) {
		case SERVO_DIRECTION_LEFT45:
			HR04_SensorsData.dist_left45 = htim->Instance->CCR2 / MICROSECONDS_TO_CM;;
			break;
		case SERVO_DIRECTION_CENTER: default:
			HR04_SensorsData.dist_front = htim->Instance->CCR2 / MICROSECONDS_TO_CM;;
			break;
		case SERVO_DIRECTION_RIGHT45:
			HR04_SensorsData.dist_right45 = htim->Instance->CCR2 / MICROSECONDS_TO_CM;;
			break;
		case SERVO_DIRECTION_LEFT90:
			HR04_SensorsData.dist_left90 = htim->Instance->CCR2 / MICROSECONDS_TO_CM;;
			break;
		case SERVO_DIRECTION_RIGHT90:
			HR04_SensorsData.dist_right90 =htim->Instance->CCR2 / MICROSECONDS_TO_CM;;
			break;
		}
		osMutexRelease(mHR04_SensorsDataMutex);
		osMutexRelease(mServoPositionMutex);

	}
	else if (htim->Instance == TIM3) /* HC-SR04 Sensor BOTTOM */
	{
		osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
		HR04_SensorsData.dist_bottom = htim->Instance->CCR2 / MICROSECONDS_TO_CM;
		osMutexRelease(mHR04_SensorsDataMutex);
	}
}

extern void Uart_isr (UART_HandleTypeDef *huart);

/**
 * ADC Conversion IRQ fb
 * @param hadc
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC5)
	{
		DMAInternalSensorsAdcValues.adc0 = ADC_BUF[0];
		DMAInternalSensorsAdcValues.adc1 = ADC_BUF[1];
		DMAInternalSensorsAdcValues.adc2 = ADC_BUF[2];
		osMessageQueuePut(xQueueDmaAdcInternalSensors, &DMAInternalSensorsAdcValues, 0U, 0U);
	}
}

