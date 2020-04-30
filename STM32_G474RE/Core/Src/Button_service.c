/**
 ******************************************************************************
 * @file    button_handler.c
 * @author  Jack Lestrohan
 * @brief   Button handler service file
 *
 * 	Pinout:		PC5 -> USER_BTN2_Pin
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

//temp
#include "MG90S_service.h"

static uint32_t uOnboardBtnLastPressedTick = 0;
static uint32_t btnflags;
osEventFlagsId_t xEventOnBoardButton;

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
static osThreadId_t xOnboardButtonServiceTaskHandle;
static uint32_t ulOnboardButtonServiceTaBuffer[256];
static osStaticThreadDef_t xOnboardButtonServiceTaControlBlock;
static const osThreadAttr_t xOnBoardButtonServiceTask_attributes = {
		.stack_mem = &ulOnboardButtonServiceTaBuffer[0],
		.name = "buttonServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON,
		.cb_mem = &xOnboardButtonServiceTaControlBlock,
		.cb_size = sizeof(xOnboardButtonServiceTaControlBlock),
		.stack_size = 256 };

/**
 * Button service main task
 * @param argument
 */
static void vOnBoardButtonServiceTask(void *argument)
{
	loggerI("Starting Button Service task...");
	char msg[50];

	for (;;)
	{
		btnflags = osEventFlagsWait(xEventOnBoardButton, BTN_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);
		if (buttonDebounce(uOnboardBtnLastPressedTick) || uOnboardBtnLastPressedTick == 0) {
			uOnboardBtnLastPressedTick = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

			//FIXME:
			// Test to see if servo is receving the active/inactive flag
			if (HAL_GPIO_ReadPin(GPIOA, LD2_Pin)) {
				strcpy(msg, "[MSG]STM32 - Starting Servo[/MSG]\n");
				osEventFlagsSet(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
			} else {
				strcpy(msg, "[MSG]STM32 - Stopping Servo[/MSG]\n");
				osEventFlagsClear(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
			}

			/* TODO: remove this, it's just for debugging purposes */
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
	xEventOnBoardButton = osEventFlagsNew(NULL);
	if (xEventOnBoardButton == NULL) {
		loggerE("Button Service Event Flags object not created!");
		return EXIT_FAILURE;
	}

	xOnboardButtonServiceTaskHandle = osThreadNew(vOnBoardButtonServiceTask, NULL, &xOnBoardButtonServiceTask_attributes);
	if (xOnboardButtonServiceTaskHandle == NULL) {
		loggerE("Button Service Task not created");
		return EXIT_FAILURE;
	}

	loggerI("Initializing Button Service... Success!");
	return EXIT_SUCCESS;
}

