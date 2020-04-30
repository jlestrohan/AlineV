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

#define BTN_PRESSED_FLAG		0x0002AU

extern osEventFlagsId_t xEventOnBoardButton;

uint8_t uButtonServiceInit();

#endif /* INC_BUTTON_SERVICE_H_ */
