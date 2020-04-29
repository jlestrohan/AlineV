/**
 ******************************************************************************
 * @file    button_handler.c
 * @author  Jack Lestrohan
 * @brief   Button handler service file
 ******************************************************************************
 */
#include <Button_service.h>
#include "configuration.h"
#include "cmsis_os2.h"
#include "ServicesSupervisorFlags.h"
#include <FreeRTOS.h>
#include <LCD_service.h>
#include "button_handler_config.h"
#include "freertos_logger_service.h"
#include <stdlib.h>
#include "stdint.h"
#include <stdbool.h>
#include <stdio.h>
#include "gpio.h"
#include "usart.h"
#include <string.h>

static uint32_t lastPressedTick = 0;
static uint32_t btnflags;

typedef StaticTask_t osStaticThreadDef_t;

/**
 * returns true if not a bounce while releasing
 * @param tick
 * @return
 */
static uint8_t buttonDebounce(uint32_t tick)
{
	return (HAL_GetTick() - tick > BTN_DEBOUNCE_MS ? true : false);
}

/**
 * Definitions for LoggerServiceTask
 */
static osThreadId_t xButtonServiceTaskHandle;
static uint32_t buttonServiceTaBuffer[256];
static osStaticThreadDef_t buttonServiceTaControlBlock;
static const osThreadAttr_t buttonServiceTask_attributes = {
		.stack_mem = &buttonServiceTaBuffer[0],
		.name = "buttonServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON,
		.cb_mem = &buttonServiceTaControlBlock,
		.cb_size = sizeof(buttonServiceTaControlBlock),
		.stack_size = 256 };

/**
 * Button service main task
 * @param argument
 */
static void vBbuttonServiceTask(void *argument)
{
	loggerI("Starting Button Service task...");

	for (;;)
	{
		btnflags = osEventFlagsWait(evt_usrbtn_id, BTN_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);
		if (buttonDebounce(lastPressedTick) || lastPressedTick == 0) {
			lastPressedTick = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

			/* TODO: remove this, it's just for debugging purposes */
			char *msg="[MSG]button pressed on STM32[/MSG]\n";
			HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		}
		osDelay(100);
	}
}

/**
 * Main initialization function
 */
uint8_t uButtonServiceInit()
{
	evt_usrbtn_id = osEventFlagsNew(NULL);
	if (evt_usrbtn_id == NULL) {
		loggerE("Button Service Event Flags object not created!");
		return EXIT_FAILURE;
	}

	xButtonServiceTaskHandle = osThreadNew(vBbuttonServiceTask, NULL, &buttonServiceTask_attributes);
	if (xButtonServiceTaskHandle == NULL) {
		loggerE("Button Service Task not created");
		return EXIT_FAILURE;
	}

	loggerI("Initializing Button Service... Success!");
	return EXIT_SUCCESS;
}

