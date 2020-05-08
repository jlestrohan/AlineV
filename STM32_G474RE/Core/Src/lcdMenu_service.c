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
#include "debug.h"
#include "configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LCD_I2C_ADDRESS		0x27
#define LCD_NB_COL			16
#define LCD_NB_ROW			2

void vSetupMenuTopics();

MENUITEMS_t *pCurrentItem = NULL;

//void fc_menu_complete() {
//	pCurrentItem = &MenuItem_Ready;
//}

void fc_menu_init() {
	//pCurrentItem = &MenuItem_Complete;
}

MENUITEMS_t MenuItem_Ready;
MENUITEMS_t MenuItem_Complete;
MENUITEMS_t MenuItem_Boot;

osEventFlagsId_t xEventMenuNavButton;

static osThreadId_t xLcdMenuServiceTaskHandle;
static const osThreadAttr_t xLcdMenuServiceTa_attributes = {
		.name = "xLcdMenuServiceTask",
		.stack_size = 512,
		.priority = (osPriority_t)OSTASK_PRIORITY_LCDMENU
};

void fc_menu_ready()
{
	//pCurrentItem = *(pCurrentItem).next = ...
	vShowLCDText();
}

/**
 * LCD Menu main task
 * @param argument
 */
void vLcdMenuServiceTask(void *argument)
{
	dbg_printf("Starting LCD Menu Service task...");

	vSetupMenuTopics();
	/* sets the starting screen */
	lcdInit(&hi2c1, (uint8_t)LCD_I2C_ADDRESS, (uint8_t)LCD_NB_ROW, (uint8_t)LCD_NB_COL);

	pCurrentItem = &MenuItem_Boot;
	vShowLCDText();

	//osDelay(3000);
	pCurrentItem = &MenuItem_Ready;
	vShowLCDText();

	//osDelay(2000);

	for (;;) {

		if (xEventMenuNavButton != NULL) {
			osEventFlagsWait(xEventMenuNavButton,BEXT_PRESSED_EVT, osFlagsWaitAny, osWaitForever);

			dbg_printf("BUTTON 2 WAS PRESSED");
			//TODO: implement long press= call prev function
			//pCurrentItem = (*pCurrentItem).next;

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
		dbg_printf("LCD Device not ready");
		return (EXIT_FAILURE);
	}*/


	xEventMenuNavButton = osEventFlagsNew(NULL);
	if (xEventMenuNavButton == NULL) {
		Error_Handler();
		dbg_printf("LCD Menu Service Event Flags object not created!");
		return EXIT_FAILURE;
	}

	/* creation of LoggerServiceTask */
	xLcdMenuServiceTaskHandle = osThreadNew(vLcdMenuServiceTask, NULL, NULL); //&xLcdMenuServiceTa_attributes);
	if (xLcdMenuServiceTaskHandle == NULL) {
		dbg_printf("Initializing LCD Menu Service - Failed");
		return (EXIT_FAILURE);
	}

	/*osSemaphoreAcquire(sem_lcdService, osWaitForever);
	lcd_send_string(MenuItem_Home.first_line);
	lcd_put_cur(1, 0);
	lcd_send_string(MenuItem_Home.second_line);
	osSemaphoreRelease(sem_lcdService);*/

	dbg_printf("Initializing LCD Menu Service - Success!");
	return (EXIT_SUCCESS);

}

/**
 * Sets the whole LCD content
 * @param items
 */
void vShowLCDText()
{
	lcdSetCursorPosition((*pCurrentItem).first_line_col, (*pCurrentItem).first_line_row);
	lcdPrintStr((uint8_t*)(*pCurrentItem).first_line_text, strlen((*pCurrentItem).first_line_text));
	lcdSetCursorPosition((*pCurrentItem).second_line_col, (*pCurrentItem).second_line_row);
	lcdPrintStr((uint8_t*)(*pCurrentItem).second_line_text, strlen((*pCurrentItem).second_line_text));
}

/**
 * Prepare all LCD static screens of the application
 */
void vSetupMenuTopics()
{
	//MenuItem_Complete = {"Alinea v0.35",1,0,"Complete!",3,2, fc_menu_complete, NULL, &MenuItem_Ready};

	/**
	 * BOOT SCREEN
	 */
	strcpy(MenuItem_Boot.first_line_text, "Alinea v0.35  >");
	MenuItem_Boot.first_line_col = 1;
	MenuItem_Boot.first_line_row = 0;
	strcpy(MenuItem_Boot.second_line_text, "Initializing...");
	MenuItem_Boot.second_line_col = 0;
	MenuItem_Boot.second_line_row = 2;
	MenuItem_Boot.func = fc_menu_init;
	MenuItem_Boot.prev = NULL;
	MenuItem_Boot.next = &MenuItem_Ready;

	/**
	 * BOOT COMPLETE
	 */
	strcpy(MenuItem_Ready.first_line_text, "Alinea v0.35  >");
	MenuItem_Ready.first_line_col = 1;
	MenuItem_Ready.first_line_row = 0;
	strcpy(MenuItem_Ready.second_line_text, "02/05/20 08:50");
	MenuItem_Ready.second_line_col = 0;
	MenuItem_Ready.second_line_row = 2;
	MenuItem_Ready.func = fc_menu_ready;
	MenuItem_Ready.prev = NULL;
	MenuItem_Ready.next = NULL;
};
