/*******************************************************************
 * lcdMenu_service.c
 *
 *  Created on: May 1, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/
/* LCD image generator here: http://avtanski.net/projects/lcd/ */

#include "lcdMenu_service.h"
#include "i2c.h"
#include "printf.h"
#include "main.h"
#include "configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HCSR04_service.h"
#include "CMPS12_service.h"

#define LCD_I2C_ADDRESS		0x27
#define LCD_NB_COL			16
#define LCD_NB_ROW			2

HR04_SensorsData_t HR04_SensorsData;	/* extern  */
osMutexId_t mHR04_SensorsDataMutex;	/* extern */

CMPS12_SensorData_t CMPS12_SensorData; /* extern */
osMutexId_t mCMPS12_SensorDataMutex;	/* extern */

/* functions declarations */
void fc_menu_hcsr04();
void fc_menu_ready();
void fc_menu_cmps();

struct MENUITEMS_t *pCurrentItem;

struct MENUITEMS_t MenuItem_HCSR04;
struct MENUITEMS_t MenuItem_CMPS2_1;
struct MENUITEMS_t MenuItem_CMPS2_2;

struct MENUITEMS_t MenuItem_Ready =
{
		"Alinea v0.35",1,0,
		"02/05/20 08:50",0,2,
		fc_menu_ready,
		NULL,
		&MenuItem_HCSR04,
		LCD_SCREEN_READY
};

struct MENUITEMS_t MenuItem_HCSR04 =
{
		"HC-SR045 Sensors",1,0,
		"infos here",0,2,
		fc_menu_hcsr04,
		NULL,
		&MenuItem_CMPS2_1,
		LCD_SCREEN_HCSR04
};

struct MENUITEMS_t MenuItem_CMPS2_1 =
{
		"CMPS12 Sensor",1,0,
		"infos here",2,2,
		fc_menu_cmps,
		&MenuItem_Ready,
		&MenuItem_CMPS2_2,
		LCD_SCREEN_CMPS12_1
};

struct MENUITEMS_t MenuItem_CMPS2_2 =
{
		"CMPS12 Sensor",1,0,
		"infos here",2,2,
		fc_menu_cmps,
		&MenuItem_CMPS2_1,
		&MenuItem_Ready,
		LCD_SCREEN_CMPS12_2
};


osEventFlagsId_t xEventMenuNavButton;

static osThreadId_t xLcdMenuServiceTaskHandle;
static osStaticThreadDef_t xLcdMenuServiceTaControlBlock;
static uint32_t xLcdMenuServiceTaBuffer[256];
static const osThreadAttr_t xLcdMenuServiceTa_attributes = {
		.name = "xLcdMenuServiceServiceTask",
		.stack_mem = &xLcdMenuServiceTaBuffer[0],
		.stack_size = sizeof(xLcdMenuServiceTaBuffer),
		.cb_mem = &xLcdMenuServiceTaControlBlock,
		.cb_size = sizeof(xLcdMenuServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_LCDMENU };

static osThreadId_t xLcdMenuLoopTaskHandle;
static osStaticThreadDef_t xLcdMenuLoopTaControlBlock;
static uint32_t xLcdMenuLoopTaBuffer[256];
static const osThreadAttr_t xLcdMenuLoopTa_attributes = {
		.name = "xLcdMenuLoopTask",
		.stack_mem = &xLcdMenuLoopTaBuffer[0],
		.stack_size = sizeof(xLcdMenuLoopTaBuffer),
		.cb_mem = &xLcdMenuLoopTaControlBlock,
		.cb_size = sizeof(xLcdMenuLoopTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_LCDMENU_LOOP };

/**
 * LCD Menu main task
 * @param argument
 */
static void vLcdMenuServiceTask(void *argument)
{
	printf("Starting LCD Menu Service task...\n\r");

	lcdInit(&hi2c1, (uint8_t)LCD_I2C_ADDRESS, (uint8_t)LCD_NB_ROW, (uint8_t)LCD_NB_COL);

	/* sets the starting screen */
	pCurrentItem = &MenuItem_Ready;
	(*pCurrentItem).func();


	xEventMenuNavButton = osEventFlagsNew(NULL);
	if (xEventMenuNavButton == NULL) {
		printf("LCD Menu Service Event Flags object not created!\n\r");
		Error_Handler();
	}
	for (;;) {

		if (xEventMenuNavButton != NULL) {
			osEventFlagsWait(xEventMenuNavButton,BEXT_PRESSED_EVT, osFlagsWaitAny, osWaitForever);

			lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
			//TODO: implement long press= call prev function
			if (pCurrentItem->next != NULL)
				pCurrentItem = pCurrentItem->next;
			(*pCurrentItem).func();
		}
		osDelay(50);
	}
	osThreadTerminate(NULL);
}

/**
 * Task - Handles the refreshing of the menu infos according to what struct element the current pointer is pointing at
 * FINITE STATE MACHINE
 * @param vParameter
 */
static void vLcdMenuLoopTask(void *vParameter)
{
	char line1[16], line2[16];

	for (;;)
	{
		switch ((intptr_t)pCurrentItem->LcdTypeScreen)
		{
		case LCD_SCREEN_HCSR04:
			lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
			lcdSetCursorPosition(0, 0);
			MUTEX_HCSR04_TAKE
			sprintf(line1, "F%0*d FL%0*d FR%0*d", 3, HR04_SensorsData.dist_front, 3, HR04_SensorsData.dist_left45,3,HR04_SensorsData.dist_right45);
			MUTEX_HCSR04_GIVE
			lcdPrintStr((uint8_t *)line1, strlen(line1));

			lcdSetCursorPosition(4, 1);
			MUTEX_HCSR04_TAKE
			sprintf(line2, "B%0*d R%0*d", 3, HR04_SensorsData.dist_bottom, 3, HR04_SensorsData.dist_rear);
			MUTEX_HCSR04_GIVE
			lcdPrintStr((uint8_t *)line2, strlen(line2));
			osDelay(20);
			break;

		case LCD_SCREEN_CMPS12_1:
			lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
			lcdSetCursorPosition(0, 0);
			lcdPrintStr((uint8_t *)"CMP12-S Mag Sens", strlen(line1));

			lcdSetCursorPosition(3, 1);
			MUTEX_CMPS12_TAKE
			sprintf(line2, "hdg: %0*d", 3, CMPS12_SensorData.CompassBearing);
			MUTEX_CMPS12_GIVE
			lcdPrintStr((uint8_t *)line2, strlen(line2));
			osDelay(50);
			break;

		case LCD_SCREEN_CMPS12_2:
			lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
			lcdSetCursorPosition(0, 0);
			lcdPrintStr((uint8_t *)"CMP12-S Mag Sens", strlen(line1));

			lcdSetCursorPosition(0, 1);
			MUTEX_CMPS12_TAKE
			sprintf(line2, "Pitch:%d Roll:%d", CMPS12_SensorData.PitchAngle, CMPS12_SensorData.RollAngle);
			MUTEX_CMPS12_GIVE
			lcdPrintStr((uint8_t *)line2, strlen(line2));
			osDelay(50);
			break;

		default: break;
		}

		osDelay(100);
	}
	osThreadTerminate(NULL);
}

/**
 * Main LCD Menu Service Initialization routine
 * @return
 */
uint8_t uLcdMenuServiceInit()
{
	/* creation of xLcdMenuServiceTaskHandle - will handle menu rotation */
	xLcdMenuServiceTaskHandle = osThreadNew(vLcdMenuServiceTask, NULL, &xLcdMenuServiceTa_attributes);
	if (xLcdMenuServiceTaskHandle == NULL) {
		printf("Initializing LCD Menu Service... failed\n\r");
		return (EXIT_FAILURE);
	}

	/* creation of xLcdMenuLoopTaskHandle - will handle information display and updating */
	xLcdMenuLoopTaskHandle = osThreadNew(vLcdMenuLoopTask, NULL, &xLcdMenuLoopTa_attributes);
	if (xLcdMenuLoopTaskHandle == NULL) {
		printf("Initializing LCD Menu Loop... failed\n\r");
		return (EXIT_FAILURE);
	}

	printf("Initializing LCD Menu Service - Success!\n\r");
	return (EXIT_SUCCESS);
}

/**
 *
 */
void fc_menu_hcsr04()
{
	lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
	/* rest is done in loop task */
}

/**
 *
 */
void fc_menu_ready()
{
	lcdCommand(LCD_CLEAR, LCD_PARAM_SET);

	lcdSetCursorPosition((*pCurrentItem).first_line_col, (*pCurrentItem).first_line_row);
	lcdPrintStr((uint8_t*)(*pCurrentItem).first_line_text, strlen((*pCurrentItem).first_line_text));
	lcdSetCursorPosition((*pCurrentItem).second_line_col, (*pCurrentItem).second_line_row);
	lcdPrintStr((uint8_t*)(*pCurrentItem).second_line_text, strlen((*pCurrentItem).second_line_text));
}

void fc_menu_cmps()
{
	lcdCommand(LCD_CLEAR, LCD_PARAM_SET);
	/* rest is done in loop task */
}
