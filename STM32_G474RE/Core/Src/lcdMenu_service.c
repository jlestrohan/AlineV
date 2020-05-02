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
#include "freertos_logger_service.h"
#include "configuration.h"
#include <FreeRTOS.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LCD_I2C_ADDRESS		0x27
#define LCD_NB_COL			16
#define LCD_NB_ROW			2

/*void fcn_roverInit()
{

}*/



MENUITEMS_t MenuItem_Init = {"Alinea v0.35",1,0,"Initializing...",0,2};
MENUITEMS_t MenuItem_InitComplete = {"Alinea v0.35",1,0,"Complete!",3,2};
MENUITEMS_t MenuItem_Ready = {"Alinea v0.35  >",1,0,"02/05/20 08:50",0,2};

osMessageQueueId_t xLcdMenuServiceQueue;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xLcdMenuServiceTaskHandle;
static osStaticThreadDef_t xLcdMenuServiceTaControlBlock;
static uint32_t xLcdMenuServiceTaBuffer[256];
static const osThreadAttr_t xLcdMenuServiceTa_attributes = {
		.name = "xLcdMenuServiceTask",
		.stack_mem = &xLcdMenuServiceTaBuffer[0],
		.stack_size = sizeof(xLcdMenuServiceTaBuffer),
		.cb_mem = &xLcdMenuServiceTaControlBlock,
		.cb_size = sizeof(xLcdMenuServiceTaControlBlock),
		.priority = (osPriority_t)OSTASK_PRIORITY_LCDMENU
};


/**
 * LCD Menu main task
 * @param argument
 */
void vLcdMenuServiceTask(void *argument)
{
	MENUITEMS_t MenuItem;
	osStatus_t osStatus;
	loggerI("Starting LCD Menu Service task...");

	lcdInit(&hi2c1, (uint8_t)LCD_I2C_ADDRESS, (uint8_t)LCD_NB_ROW, (uint8_t)LCD_NB_COL);
	vPrepareLCDText(&MenuItem_Init);
	osDelay(5000);
	lcdDisplayClear();
	vPrepareLCDText(&MenuItem_InitComplete);
	osDelay(2000);
	vPrepareLCDText(&MenuItem_Ready);

	for (;;) {

		/* receives a full string */
		osStatus = osMessageQueueGet(xLcdMenuServiceQueue, &MenuItem, NULL, osWaitForever);
		if (osStatus == osOK) {
			/* https://stackoverflow.com/questions/43963019/arduino-lcd-generate-a-navigation-menu-by-reading-an-array */
			/*osSemaphoreAcquire(sem_lcdService, osWaitForever);
			lcd_send_string(MenuItem.first_line);
			lcd_put_cur(0, 0);
			lcd_send_string(MenuItem.second_line);
			osSemaphoreRelease(sem_lcdService);*/
		}
		osDelay(50);
	}
}

/**
 * Main LCD Menu Service Initialization routine
 * @return
 */
uint8_t uLcdMenuServiceInit()
{
	/* first we check if LCD I2C stuff is available */
	/*if (!lcd_isAvail()) {
		loggerE("LCD Device not ready");
		return (EXIT_FAILURE);
	}*/

	xLcdMenuServiceQueue = osMessageQueueNew(10, sizeof(MENUITEMS_t), NULL);
	if (xLcdMenuServiceQueue == NULL) {
		return (EXIT_FAILURE);
	}

	/* creation of LoggerServiceTask */
	xLcdMenuServiceTaskHandle = osThreadNew(vLcdMenuServiceTask, NULL, &xLcdMenuServiceTa_attributes);
	if (xLcdMenuServiceTaskHandle == NULL) {
		loggerE("Initializing LCD Menu Service - Failed");
		return (EXIT_FAILURE);
	}

	/*osSemaphoreAcquire(sem_lcdService, osWaitForever);
	lcd_send_string(MenuItem_Home.first_line);
	lcd_put_cur(1, 0);
	lcd_send_string(MenuItem_Home.second_line);
	osSemaphoreRelease(sem_lcdService);*/

	loggerI("Initializing LCD Menu Service - Success!");
	return (EXIT_SUCCESS);

}

/**
 * Sets the whole LCD content
 * @param items
 */
void vPrepareLCDText(MENUITEMS_t *items)
{
	lcdSetCursorPosition(items->first_line_col, items->first_line_row);
	lcdPrintStr((uint8_t*)items->first_line_text, strlen(items->first_line_text));
	lcdSetCursorPosition(items->second_line_col, items->second_line_row);
	lcdPrintStr((uint8_t*)items->second_line_text, strlen(items->second_line_text));
}
