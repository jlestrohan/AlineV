/**
 ******************************************************************************
 * @file    button_handler.c
 * @author  Jack Lestrohan
 * @brief   Button handler service file
 ******************************************************************************
 */
#include "button_handler.h"
#include "button_handler_config.h"
#include "freertos_logger_service.h"
#include "stdint.h"
#include <stdbool.h>
#include "gpio.h"

static uint32_t lastPressedTick = 0;
static uint32_t btnflags;

/**
 * returns true if not a bounce while releasing
 */
static uint8_t buttonDebounce(uint32_t tick)
{
	return (HAL_GetTick() - tick > BTN_DEBOUNCE_MS ? true : false);
}

/**
 * Definitions for LoggerServiceTask
 */
static osThreadId_t buttonServiceTaskHandle;
static const osThreadAttr_t buttonServiceTask_attributes = {
        .name = "buttonServiceTask",
        .priority = (osPriority_t) osPriorityNormal, .stack_size = 256 };

/**
 * Button service main task
 * @param argument
 */
static void buttonService_task(void *argument)
{
	while (1) {
		btnflags = osEventFlagsWait(evt_usrbtn_id, BTN_PRESSED_FLAG,
		osFlagsWaitAny, osWaitForever);
		if (buttonDebounce(lastPressedTick) || lastPressedTick == 0) {
			lastPressedTick = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOA, LD2_Pin);
		}

		osDelay(100);
	}
	/* if we exit the loop for some reason, let's temrinate that task */
	/* todo: handle error here */
	osThreadTerminate(buttonService_task);
}

/**
 * Main initialization function
 */
uint8_t buttonService_initialize()
{
	buttonServiceTaskHandle = osThreadNew(buttonService_task, NULL,
	        &buttonServiceTask_attributes);

	evt_usrbtn_id = osEventFlagsNew(NULL);
	if (evt_usrbtn_id == NULL) {
		loggerE("Event Flags object not created, handle failure");
		return (-1);
	}
	return (0);
}

