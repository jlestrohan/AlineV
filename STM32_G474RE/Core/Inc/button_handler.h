/**
 ******************************************************************************
 * @file    button_handler.h
 * @author  Jack Lestrohan
 * @brief   Button handler service header file
 ******************************************************************************
 */
#ifndef INC_BUTTON_HANDLER_H_
#define INC_BUTTON_HANDLER_H_

#include "cmsis_os2.h"

#define BTN_PRESSED_FLAG		0x01

osEventFlagsId_t evt_usrbtn_id;

uint8_t buttonService_initialize();

#endif /* INC_BUTTON_HANDLER_H_ */
