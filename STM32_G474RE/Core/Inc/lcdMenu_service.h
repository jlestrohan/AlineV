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

#define BEXT_PRESSED_EVT	0x01U

extern osEventFlagsId_t xEventMenuNavButton;

/**
 * Menu Related Defines
 */
typedef struct {
	char first_line_text[16];
	uint8_t first_line_col;
	uint8_t first_line_row;
	char second_line_text[16];
	uint8_t second_line_col;
	uint8_t second_line_row;
	void (*func)(void);                 /* Pointer to the item function */
	const struct MENUITEMS_t *prev;           /* Pointer to the previous */
	const struct MENUITEMS_t *next;           /* Pointer to the next */
} MENUITEMS_t;



uint8_t uLcdMenuServiceInit();
void vShowLCDText();

#endif /* INC_LCDMENU_SERVICE_H_ */

