/*
 * button_handler.h
 *
 *  Created on: 26 févr. 2020
 *      Author: jack lestrohan
 */

#ifndef INC_BUTTON_HANDLER_H_
#define INC_BUTTON_HANDLER_H_

#define BTN_PRESSED_FLAG		0x01

#include "cmsis_os2.h"


osEventFlagsId_t evt_usrbtn_id;
void buttonIRQ_cb();


#endif /* INC_BUTTON_HANDLER_H_ */
