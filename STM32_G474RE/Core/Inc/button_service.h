/**
 ******************************************************************************
 * @file    button_handler.h
 * @author  Jack Lestrohan
 * @brief   Button handler service header file
 ******************************************************************************
 */
#ifndef INC_BUTTON_SERVICE_H_
#define INC_BUTTON_SERVICE_H_

#include "cmsis_os2.h"

#define B_ONBOARD_PRESSED_FLAG		0x01U
#define B_EXT_PRESSED_FLAG			0x02U

#define BTN_DEBOUNCE_MS			150 /*  ms */

extern osEventFlagsId_t xEventOnBoardButton,xEventButtonExt;

uint8_t uButtonServiceInit();

#endif /* INC_BUTTON_SERVICE_H_ */
