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

#define FLG_UV_LED_ACTIVE		(1 << 0)

extern osEventFlagsId_t xEventUvLed;

uint8_t uUvLedServiceInit();

#endif /* INC_UVLED_SERVICE_H_ */
