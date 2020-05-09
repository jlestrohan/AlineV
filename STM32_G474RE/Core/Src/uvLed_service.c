/*
 * uvLed_service.c
 *
 *  Created on: May 8, 2020
 *      Author: aez03
 */

#include "uvLed_service.h"
#include <stdlib.h>
#include "printf.h"
#include "main.h"
#include "configuration.h"
#include "gpio.h"

osEventFlagsId_t xEventUvLed;

typedef StaticTask_t osStaticThreadDef_t;

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
void vUvLedServiceTaskStart(void *vParameters)
{
	printf("Starting UV LED Service task...\n\r");

	for (;;) {
		/* LED is lit when pin is down */
		HAL_GPIO_WritePin(GPIOA, UV_LED_Pin, !(osEventFlagsGet(xEventUvLed) && FLG_UV_LED_ACTIVE));
		osDelay(500);
	}
	osThreadTerminate(xUvLedServiceTaskHandle);
}


/**
 * Main UV LED Initialization routine
 * @return
 */
uint8_t uUvLedServiceInit()
{
	xEventUvLed = osEventFlagsNew(NULL);
	if (xEventUvLed == NULL) {
		printf("Error Initializing xEventUvLed Event Flag...\n\r");
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

	return EXIT_SUCCESS;
}

