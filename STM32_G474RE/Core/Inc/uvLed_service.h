/*
 * uvLed_service.h
 *
 *  Created on: May 8, 2020
 *      Author: aez03
 */

#ifndef INC_UVLED_SERVICE_H_
#define INC_UVLED_SERVICE_H_

#include "FreeRTOS.h"
#include "cmsis_os2.h"

typedef enum {
	UV_LED_STATUS_UNSET,
	UV_LED_STATUS_SET,
	UV_LED_STATUS_BLINK
} UV_LedStatus_t;

/*********************************************************/
/** PUBLIC 												 */
extern osMessageQueueId_t xQueueUVLedStatus;

uint8_t uUvLedServiceInit();

#endif /* INC_UVLED_SERVICE_H_ */
