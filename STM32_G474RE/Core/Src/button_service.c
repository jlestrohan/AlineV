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
#include "debug.h"
#include <stdlib.h>
#include "stdint.h"
#include <stdbool.h>
#include <stdio.h>
#include "gpio.h"
#include "usart.h"
#include <string.h>
#include "lcdMenu_service.h"
#include <assert.h>
#include "uvLed_service.h"

//temp
#include "MotorsControl_service.h"

char msg[50];

osEventFlagsId_t xEventOnBoardButton,xEventButtonExt;
osMessageQueueId_t xQueueEspSerialTX;

typedef StaticTask_t osStaticThreadDef_t;

/**
 * returns true if not a bounce while releasing
 * @param tick
 * @return
 */
static uint8_t uButtonDebounce(uint32_t tick)
{
	return (HAL_GetTick() - tick > BTN_DEBOUNCE_MS ? true : false);
}

/**
 * Definitions for the onBoard button B1
 */
static osThreadId_t xOnboardButtonServiceTaskHandle;
static const osThreadAttr_t xOnBoardButtonServiceTask_attributes = {
		.name = "buttonServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ONBOARD,
		.stack_size = 512
};

/**
 * Definitions for the Additional button B2
 */
static osThreadId_t xButton2ServiceTaskHandle;
static const osThreadAttr_t xButton2ServiceTask_attributes = {
		.name = "button2ServiceTask",
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ADD,
		.stack_size = 512
};

/**
 * OnBoard Button service main task
 * @param argument
 */
static void vOnBoardButtonServiceTask(void *argument)
{
	uint32_t uOnboardBtnLastPressedTick = 0;
	dbg_printf("Starting Button ONBOARD Service task...");

	for (;;)
	{
		if (xEventOnBoardButton != NULL) {
			osEventFlagsWait(xEventOnBoardButton,B_ONBOARD_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);

			if (uButtonDebounce(uOnboardBtnLastPressedTick) || uOnboardBtnLastPressedTick == 0) {

				uOnboardBtnLastPressedTick = HAL_GetTick();
				HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

				//FIXME:
				// Test to see if servo is receving the active/inactive flag
				if (HAL_GPIO_ReadPin(GPIOA, LD2_Pin)) {
					strcpy(msg, "STM32 - Starting Motors\n");
					if (xEventMotorsForward != NULL) {
						osEventFlagsSet(xEventMotorsForward, MOTORS_FORWARD_ACTIVE);
						osEventFlagsSet(xEventUvLed, FLG_UV_LED_ACTIVE);
					}
					if (xQueueEspSerialTX != NULL) {
						osMessageQueuePut(xQueueEspSerialTX, &msg, 0U, 0U);
					}
				} else {
					strcpy(msg, "STM32 - Stopping Motors\n");
					if (xEventMotorsForward != NULL) {
						osEventFlagsClear(xEventMotorsForward, MOTORS_FORWARD_ACTIVE);
						osEventFlagsClear(xEventUvLed, FLG_UV_LED_ACTIVE);
					}
					if (xQueueEspSerialTX != NULL) {
						osMessageQueuePut(xQueueEspSerialTX, &msg, 0U, 0U);
					}
				}

				/* TODO: remove this, it's just for debugging purposes */
				//HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
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
	dbg_printf("Starting Button EXT Service task...");

	for (;;)
	{
		if (xEventButtonExt != NULL) {
			osEventFlagsWait(xEventButtonExt, B_EXT_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);
			if (uButtonDebounce(uBtn2LastPressedTick) || uBtn2LastPressedTick == 0) {
				uBtn2LastPressedTick = HAL_GetTick();
				osEventFlagsSet(xEventMenuNavButton, BEXT_PRESSED_EVT);
			}
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
		dbg_printf("Button Service Event Flags object not created!");
		Error_Handler();
		return EXIT_FAILURE;
	}
	xEventButtonExt = osEventFlagsNew(NULL);
	if (xEventButtonExt == NULL) {
		dbg_printf("Button 2 Service Event Flags object not created!");
		Error_Handler();
		return EXIT_FAILURE;
	}

	xOnboardButtonServiceTaskHandle = osThreadNew(vOnBoardButtonServiceTask, NULL, &xOnBoardButtonServiceTask_attributes);
	if (xOnboardButtonServiceTaskHandle == NULL) {
		dbg_printf("Button Service Task not created");
		Error_Handler();
		return EXIT_FAILURE;
	}

	xButton2ServiceTaskHandle = osThreadNew(vButton2ServiceTask, NULL, &xButton2ServiceTask_attributes);
	if (xButton2ServiceTaskHandle == NULL) {
		dbg_printf("Button 2 Service Task not created");
		Error_Handler();
		return EXIT_FAILURE;
	}

	dbg_printf("Initializing Button Service... Success!");
	return EXIT_SUCCESS;
}

