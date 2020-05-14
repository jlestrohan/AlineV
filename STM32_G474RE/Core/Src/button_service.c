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
#include "navControl_service.h"
#include "MG90S_service.h"
#include "HCSR04_service.h"

//temp
#include "MotorsControl_service.h"

char msg[50];

osMessageQueueId_t xQueueButtonEvent;

osMessageQueueId_t xQueueEspSerialTX; /*extern */
osEventFlagsId_t xEventFlagNavControlMainCom; /* extern */
osMessageQueueId_t xQueuePWMControlHnd; /* extern */

static MotorMotion_t motorMotion;
typedef StaticTask_t osStaticThreadDef_t;
osMessageQueueId_t xMessageQueueDecisionControlMainCom; /* extern */


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
 * returns true if not a bounce while releasing
 * @param tick
 * @return
 */
static uint8_t uButtonDebounce(uint32_t tick)
{
	return (HAL_GetTick() - tick > BTN_DEBOUNCE_MS ? true : false);
}



/**
 * OnBoard Button service main task
 * @param argument
 */
static void vOnBoardButtonServiceTask(void *argument)
{
	printf("Starting Onboard Button Service task...\n\r");

	osStatus_t status;
	ButtonEvent_t btn_event;
	uint32_t uOnboardBtnLastPressedTick = HAL_GetTick();
	uint32_t uExternalBtnLastPressedTick = HAL_GetTick();

	for (;;)
	{
		status = osMessageQueueGet(xQueueButtonEvent, &btn_event, 0U, osWaitForever);
		if (status == osOK) {
			switch (btn_event) {
			case BTN_BUTTON_ONBOARD_PRESSED:
				if (uButtonDebounce(uOnboardBtnLastPressedTick)) {
					uOnboardBtnLastPressedTick = HAL_GetTick();
					printf("Onboard button was pressed ...\n\r");
					HAL_GPIO_TogglePin(GPIOA, LD2_Pin);

					//FIXME:
					if (HAL_GPIO_ReadPin(GPIOA, LD2_Pin)) {
						NavSpecialEvent_t event = START_EVENT;
						osMessageQueuePut(xMessageQueueDecisionControlMainCom, &event, 0U, 0U);
					}
				} else {
					NavSpecialEvent_t event = STOP_EVENT;
					osMessageQueuePut(xMessageQueueDecisionControlMainCom, &event, 0U, 0U);
				}
				break;

			case BTN_BUTTON_EXT_PRESSED:
				if (uButtonDebounce(uExternalBtnLastPressedTick)) {
					uExternalBtnLastPressedTick = HAL_GetTick();
					printf("External button was pressed ...\n\r");
					if (xQueueMotorMotionOrder != NULL) {
						motorMotion = MOTOR_MOTION_BACKWARD;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
					}
				}
				break;
			default: break;
			}
		}
		osDelay(1);
	}
	osThreadTerminate(xOnboardButtonServiceTaskHandle);
}

/**
 * Main initialization function
 */
uint8_t uButtonServiceInit()
{
	xQueueButtonEvent = osMessageQueueNew(1, sizeof(uint8_t), NULL);
	if (xQueueButtonEvent == NULL) {
		printf("Button Service Queue initialization failed!\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	xOnboardButtonServiceTaskHandle = osThreadNew(vOnBoardButtonServiceTask, NULL, &xOnBoardButtonServiceTask_attributes);
	if (xOnboardButtonServiceTaskHandle == NULL) {
		printf("Button Service Task not created\n\r");
		Error_Handler();
		return EXIT_FAILURE;
	}

	printf("Initializing Button Service... Success!\n\r");
	return EXIT_SUCCESS;
}

