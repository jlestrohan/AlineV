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

/**
 * Menu Related Defines
 */
typedef struct MENUITEMS_t{
	char first_line_text[16];
	uint8_t first_line_col;
	uint8_t first_line_row;
	char second_line_text[16];
	uint8_t second_line_col;
	uint8_t second_line_row;
	//void (*func)(void);                 /* Pointer to the item function */
	//const struct item * prev;           /* Pointer to the previous */
	//const struct item * next;           /* Pointer to the next */
} MENUITEMS_t;

uint8_t uLcdMenuServiceInit();
void vPrepareLCDText(MENUITEMS_t *items);

#endif /* INC_LCDMENU_SERVICE_H_ */

