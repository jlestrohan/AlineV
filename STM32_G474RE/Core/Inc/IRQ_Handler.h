/*
 * IRQ_Handler.h
 *
 *  Created on: Feb 26, 2020
 *      Author: jack
 */

#ifndef INC_IRQ_HANDLER_H_
#define INC_IRQ_HANDLER_H_

#include "HCSR04_service.h"

typedef struct {
	uint8_t rear;
	uint8_t front;
	uint8_t bottom;
} hcSensorsTimersValue_t;


#endif /* INC_IRQ_HANDLER_H_ */
