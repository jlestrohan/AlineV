/*
 * uvLed_service.c
 *
 *  Created on: May 8, 2020
 *      Author: aez03
 *
 *      Pinout: PB1 -> LEDSTRIP
 */

#include "uvLed_service.h"
#include <stdlib.h>
#include "printf.h"
#include "main.h"
#include "configuration.h"
#include "gpio.h"
#include "tim.h"

osMessageQueueId_t xQueueUVLedStatus;
UV_LedStatus_t uUVLedStatus;
osMutexId_t mUvLedStatusMutex;

static osThreadId_t xUvLedServiceTaskHandle;
static osStaticThreadDef_t xUvLedServiceTaControlBlock;
static uint32_t xUvLedServiceTaBuffer[256];
static const osThreadAttr_t xUvLedServiceTa_attributes = {
		.stack_mem = &xUvLedServiceTaBuffer[0],
		.stack_size = sizeof(xUvLedServiceTaBuffer),
		.name = "xUvLedServiceServiceTask",
		.cb_size = sizeof(xUvLedServiceTaControlBlock),
		.cb_mem = &xUvLedServiceTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_UVLED
};

/**
 * Main Task
 * @param vParameters
 */
static void vUvLedServiceTaskStart(void *vParameters)
{
	printf("Starting UV LED Service task...\n\r");

	uint8_t uUvLedStatus;
	osStatus_t status;

	/* starting with leds unlit */
	HAL_GPIO_WritePin(GPIOA, UV_LED_Pin, GPIO_PIN_SET); /* set = unlit */

	for (;;) {

		status = osMessageQueueGet(xQueueUVLedStatus, &uUvLedStatus, 0U, osWaitForever);
		if (status == osOK) {
			MUTEX_UVLED_TAKE
			uUVLedStatus = uUvLedStatus; /* saved to be used by the datacenter */
			MUTEX_UVLED_GIVE

			switch (uUvLedStatus) {
			case UV_LED_STATUS_UNSET:
				HAL_GPIO_WritePin(GPIOA, UV_LED_Pin, GPIO_PIN_SET);
				break;
			case UV_LED_STATUS_SET:
				HAL_GPIO_WritePin(GPIOA, UV_LED_Pin, GPIO_PIN_RESET);
				break;
			case UV_LED_STATUS_BLINK:
				HAL_GPIO_TogglePin(GPIOA, UV_LED_Pin);
				osDelay(120);
				uUvLedStatus = UV_LED_STATUS_BLINK;
				osMessageQueuePut(xQueueUVLedStatus, &uUvLedStatus, 0U, osWaitForever);
				break;
			default: break;
			}
		}
		osDelay(10);
	}
	osThreadTerminate(xUvLedServiceTaskHandle);
}


/**
 * Main UV LED Initialization routine
 * @return
 */
uint8_t uUvLedServiceInit()
{
	mUvLedStatusMutex = osMutexNew(NULL);

	xQueueUVLedStatus = osMessageQueueNew(10,  sizeof(uint8_t), NULL);
	if (xQueueUVLedStatus == NULL) {
		printf("Error Initializing xQueueUVLedStatus UV Leds Service Queue...\n\r");
		Error_Handler();
		return EXIT_FAILURE;
	}

	/* creation of HR04Sensor1_task */
	xUvLedServiceTaskHandle = osThreadNew(vUvLedServiceTaskStart, NULL, &xUvLedServiceTa_attributes);
	if (xUvLedServiceTaskHandle == NULL) {
		printf("xUvLedService Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing UV LED Service... Success!\n\r");
	return EXIT_SUCCESS;
}

