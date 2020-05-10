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

#define BTN_DEBOUNCE_MS			150 /*  ms */

typedef enum {
	BTN_BUTTON_ONBOARD_PRESSED,
	BTN_BUTTON_ONBOARD_UNPRESSED,
	BTN_BUTTON_EXT_PRESSED,
	BTN_BUTTON_EXT_UNPRESSED,
} ButtonEvent_t;

/** Public */
extern osMessageQueueId_t xQueueButtonEvent;

uint8_t uButtonServiceInit();

#endif /* INC_BUTTON_SERVICE_H_ */
