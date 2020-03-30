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
#include "lcd_service.h"
#include <stdlib.h>
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
	for (;;) {

		btnflags = osEventFlagsWait(evt_usrbtn_id, BTN_PRESSED_FLAG,
		osFlagsWaitAny, osWaitForever);
		if (buttonDebounce(lastPressedTick) || lastPressedTick == 0) {
			lastPressedTick = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

			//osSemaphoreAcquire(sem_lcdService, 100U);
			lcd_send_string("Button Pressed");
			//osSemaphoreRelease(sem_lcdService);
		}

		osDelay(100);
	}
}

/**
 * Main initialization function
 */
uint8_t buttonService_initialize()
{
	buttonServiceTaskHandle = osThreadNew(buttonService_task, NULL, &buttonServiceTask_attributes);
	if (!buttonServiceTaskHandle) {
		return (EXIT_FAILURE);
	}

	evt_usrbtn_id = osEventFlagsNew(NULL);
	if (evt_usrbtn_id == NULL) {
		loggerE("Event Flags object not created, handle failure");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing Button Service... Success!");
	return (EXIT_SUCCESS);
}

