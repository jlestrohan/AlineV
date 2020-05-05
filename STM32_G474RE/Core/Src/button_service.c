/**
 ******************************************************************************
 * @file    button_handler.c
 * @author  Jack Lestrohan
 * @brief   Button handler service file
 *
 * 	Pinout:		PC5 -> 	USER_BTN2_Pin
 * 				PA5 ->	ONBOARD BTN
 *
 * 				ONBOARD FUNCTION = 	Navigate Thru Menu (LCD)
 * 				B2 FUNCTION		 =	Select current option
 ******************************************************************************
 */
#include "Button_service.h"
#include "configuration.h"
#include "cmsis_os2.h"
#include "ServicesSupervisorFlags.h"
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include <stdlib.h>
#include "stdint.h"
#include <stdbool.h>
#include <stdio.h>
#include "gpio.h"
#include "usart.h"
#include <string.h>
#include "lcdMenu_service.h"

//temp
#include "MotorsControl_service.h"

osEventFlagsId_t xEventOnBoardButton,xEventButtonExt;

typedef StaticTask_t osStaticThreadDef_t;

/**
 * returns true if not a bounce while releasing
 * @param tick
 * @return
 */
static uint8_t uButtonDebounce(uint32_t tick)
{
	return (osKernelGetSysTimerCount() - tick > BTN_DEBOUNCE_MS ? true : false);
}

/**
 * Definitions for the onBoard button B1
 */
static osThreadId_t xOnboardButtonServiceTaskHandle;
static uint32_t ulOnboardButtonServiceTaBuffer[256];
static osStaticThreadDef_t xOnboardButtonServiceTaControlBlock;
static const osThreadAttr_t xOnBoardButtonServiceTask_attributes = {
		.stack_mem = &ulOnboardButtonServiceTaBuffer[0],
		.name = "buttonServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ONBOARD,
		.cb_mem = &xOnboardButtonServiceTaControlBlock,
		.cb_size = sizeof(xOnboardButtonServiceTaControlBlock),
		.stack_size = 512 };

/**
 * Definitions for the Additional button B2
 */
static osThreadId_t xButton2ServiceTaskHandle;
static uint32_t ulButton2ServiceTaBuffer[256];
static osStaticThreadDef_t xButton2ServiceTaControlBlock;
static const osThreadAttr_t xButton2ServiceTask_attributes = {
		.stack_mem = &ulButton2ServiceTaBuffer[0],
		.name = "button2ServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ADD,
		.cb_mem = &xButton2ServiceTaControlBlock,
		.cb_size = sizeof(xButton2ServiceTaControlBlock),
		.stack_size = 512 };

/**
 * OnBoard Button service main task
 * @param argument
 */
static void vOnBoardButtonServiceTask(void *argument)
{
	uint32_t uOnboardBtnLastPressedTick = 0;
	loggerI("Starting Button Service task...");
	char msg[50];
	uint32_t btnflags;

	for (;;)
	{
		btnflags = osEventFlagsWait(xEventOnBoardButton,B1_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);

		if (uButtonDebounce(uOnboardBtnLastPressedTick) || uOnboardBtnLastPressedTick == 0) {

			uOnboardBtnLastPressedTick = osKernelGetSysTimerCount();
			HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

			//FIXME:
			// Test to see if servo is receving the active/inactive flag
			if (HAL_GPIO_ReadPin(GPIOA, LD2_Pin)) {
				strcpy(msg, "[MSG]STM32 - Starting Motors[/MSG]\n");
				//osEventFlagsSet(xEventMotorsForward, MOTORS_FORWARD_ACTIVE);
			} else {
				strcpy(msg, "[MSG]STM32 - Stopping Motors[/MSG]\n");
				//osEventFlagsClear(xEventMotorsForward, MOTORS_FORWARD_ACTIVE);
			}

			/* TODO: remove this, it's just for debugging purposes */
			HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		}
	}
		osDelay(100);
	}
}


/**
 * Additional Button service main task B2
 * @param argument
 */
static void vButton2ServiceTask(void *argument)
{
	uint32_t uBtn2LastPressedTick = 0;
	loggerI("Starting Button EXT Service task...");

	for (;;)
	{
		osEventFlagsWait(xEventButtonExt, B_EXT_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);
		if (uButtonDebounce(uBtn2LastPressedTick) || uBtn2LastPressedTick == 0) {
			uBtn2LastPressedTick = HAL_GetTick();
			osEventFlagsSet(xEventMenuNavButton, BEXT_PRESSED_EVT);
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
		Error_Handler();
		loggerE("Button Service Event Flags object not created!");
		return EXIT_FAILURE;
	}
	xEventButtonExt = osEventFlagsNew(NULL);
		if (xEventButtonExt == NULL) {
			Error_Handler();
			loggerE("Button 2 Service Event Flags object not created!");
			return EXIT_FAILURE;
		}

	xOnboardButtonServiceTaskHandle = osThreadNew(vOnBoardButtonServiceTask, NULL, &xOnBoardButtonServiceTask_attributes);
	if (xOnboardButtonServiceTaskHandle == NULL) {
		Error_Handler();
		loggerE("Button Service Task not created");
		return EXIT_FAILURE;
	}

	xButton2ServiceTaskHandle = osThreadNew(vButton2ServiceTask, NULL, &xButton2ServiceTask_attributes);
	if (xButton2ServiceTaskHandle == NULL) {
		Error_Handler();
		loggerE("Button 2 Service Task not created");
		return EXIT_FAILURE;
	}

	loggerI("Initializing Button Service... Success!");
	return EXIT_SUCCESS;
}

