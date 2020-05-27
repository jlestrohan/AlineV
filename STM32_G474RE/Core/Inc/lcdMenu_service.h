/*******************************************************************
 * lcdMenu_service.h
 *
 *  Created on: May 1, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#ifndef INC_LCDMENU_SERVICE_H_
#define INC_LCDMENU_SERVICE_H_

#include <stdint.h>
#include "lcd_i2c.h"
#include <FreeRTOS.h>
#include "cmsis_os2.h"

#define EVNT_BTN_PRESSED			(1 << 0)
#define EVNT_STATUS_AVOIDING		(1 << 1)
#define EVNT_STATUS_DISINFECT		(1 << 2)
#define EVNT_STATUS_DISINFECT_OFF	(1 << 3)

extern osEventFlagsId_t xEventLcdDisplay;

/**
 * Menu Related Defines
 */
typedef enum {
	LCD_SCREEN_READY,
	LCD_SCREEN_HCSR04,
	LCD_SCREEN_CMPS12_1,
	LCD_SCREEN_CMPS12_2,
	LCD_SCREEN_BME280,
	LCD_STATUS_AVOIDING_MODE,
	LCD_STATUS_DISINFECT_MODE,
	LCD_STATUS_DISINFECT_OFF_MODE
} LcdTypeScreen_t;

 struct MENUITEMS_t {
	char first_line_text[16];
	uint8_t first_line_col;
	uint8_t first_line_row;
	char second_line_text[16];
	uint8_t second_line_col;
	uint8_t second_line_row;
	void (*func)(void);                 /* Pointer to the item function */
	struct MENUITEMS_t *prev;           /* Pointer to the previous */
	struct MENUITEMS_t *next;           /* Pointer to the next */
	LcdTypeScreen_t LcdTypeScreen;		/* holds a track of the current selected screen (cannot switch over pointers alas... ) */
};

extern struct MENUITEMS_t *pCurrentItem;
struct MENUITEMS_t MenuItem_Ready;
struct MENUITEMS_t MenuItem_Avoiding;

uint8_t uLcdMenuServiceInit();
void vShowLCDText();

#endif /* INC_LCDMENU_SERVICE_H_ */

