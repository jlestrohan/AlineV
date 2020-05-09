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
#include "main.h"
#include "configuration.h"
#include "cmsis_os2.h"
#include "ServicesSupervisorFlags.h"
#include <FreeRTOS.h>
#include "printf.h"
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
#include "navControl_service.h"

//temp
#include "MotorsControl_service.h"

char msg[50];

osEventFlagsId_t xEventOnBoardButton,xEventButtonExt;
osMessageQueueId_t xQueueEspSerialTX; /*extern */
osMessageQueueId_t xQueueMotorMotionOrder; /* extern */
FSM_Status_t FSM_IA_STATUS; /* extern */

static MotorMotion_t motorMotion;
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
static osStaticThreadDef_t  xOnboardButtonServiceTaControlBlock;
static uint32_t xOnboardButtonServiceTaBuffer[256];
static const osThreadAttr_t xOnBoardButtonServiceTask_attributes = {
		.name = "buttonServiceTask",
		.stack_mem = &xOnboardButtonServiceTaBuffer[0],
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ONBOARD,
		.cb_mem = &xOnboardButtonServiceTaControlBlock,
		.stack_size = sizeof(xOnboardButtonServiceTaBuffer),
		.cb_size = sizeof(xOnboardButtonServiceTaControlBlock),
};

/**
 * Definitions for the Additional button B2
 */
static osThreadId_t xButton2ServiceTaskHandle;
static osStaticThreadDef_t  xButton2ServiceTaControlBlock;
static uint32_t xButton2ServiceTaBuffer[256];
static const osThreadAttr_t xButton2ServiceTask_attributes = {
		.name = "button2ServiceTask",
		.stack_mem = &xButton2ServiceTaBuffer[0],
		.priority = (osPriority_t) OSTASK_PRIORITY_BUTTON_ADD,
		.cb_mem = &xButton2ServiceTaControlBlock,
		.stack_size = sizeof(xButton2ServiceTaBuffer),
		.cb_size = sizeof(xButton2ServiceTaControlBlock),
};

/**
 * OnBoard Button service main task
 * @param argument
 */
static void vOnBoardButtonServiceTask(void *argument)
{
	uint32_t uOnboardBtnLastPressedTick = 0;
	printf("Starting Onboard Button Service task...\n\r");

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
					if (xQueueMotorMotionOrder != NULL) {
						/* let's pretend we're starting an exploration mission */
						FSM_IA_STATUS = statusRUNNING;
						motorMotion = MOTOR_MOTION_FORWARD;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						osEventFlagsSet(xEventUvLed, FLG_UV_LED_ACTIVE);
					}
					if (xQueueEspSerialTX != NULL) {
						osMessageQueuePut(xQueueEspSerialTX, &msg, 0U, 0U);
					}
				} else {
					strcpy(msg, "STM32 - Stopping Motors\n");
					if (xQueueMotorMotionOrder != NULL) {
						motorMotion = MOTOR_MOTION_IDLE;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
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
	osThreadTerminate(xOnboardButtonServiceTaskHandle);
}


/**
 * Additional Button service main task B2
 * @param argument
 */
static void vButton2ServiceTask(void *argument)
{
	uint32_t uBtn2LastPressedTick = 0;
	printf("Starting Button EXT Service task...\n\r");

	xEventButtonExt = osEventFlagsNew(NULL);
	if (xEventButtonExt == NULL) {
		printf("Button 2 Service Event Flags object not created!\n\r");
		Error_Handler();
		osThreadSuspend(xButton2ServiceTaskHandle);
	}

	for (;;)
	{
		if (xEventButtonExt != NULL) {
			osEventFlagsWait(xEventButtonExt, B_EXT_PRESSED_FLAG, osFlagsWaitAny, osWaitForever);
			if (uButtonDebounce(uBtn2LastPressedTick) || uBtn2LastPressedTick == 0) {
				uBtn2LastPressedTick = HAL_GetTick();
				//osEventFlagsSet(xEventMenuNavButton, BEXT_PRESSED_EVT);
				if (xQueueMotorMotionOrder != NULL) {
					motorMotion = MOTOR_MOTION_BACKWARD;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
				}
			}
		}
		osDelay(100);
	}
	osThreadTerminate(xButton2ServiceTaskHandle);
}

/**
 * Main initialization function
 */
uint8_t uButtonServiceInit()
{
	xEventOnBoardButton = osEventFlagsNew(NULL);
	if (xEventOnBoardButton == NULL) {
		printf("Button Service Event Flags object not created!\n\r");
		Error_Handler();
		osThreadSuspend(xOnboardButtonServiceTaskHandle);
	}

	xOnboardButtonServiceTaskHandle = osThreadNew(vOnBoardButtonServiceTask, NULL, &xOnBoardButtonServiceTask_attributes);
	if (xOnboardButtonServiceTaskHandle == NULL) {
		printf("Button Service Task not created\n\r");
		Error_Handler();
		return EXIT_FAILURE;
	}

	xButton2ServiceTaskHandle = osThreadNew(vButton2ServiceTask, NULL, &xButton2ServiceTask_attributes);
	if (xButton2ServiceTaskHandle == NULL) {
		printf("Button 2 Service Task not created\n\r");
		Error_Handler();
		return EXIT_FAILURE;
	}

	printf("Initializing Button Service... Success!\n\r");
	return EXIT_SUCCESS;
}

