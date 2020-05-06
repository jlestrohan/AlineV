/*******************************************************************
 * MG90S_service.h
 *
 *  Created on: 23 avr. 2020
 *      Author: Jack Lestrohan
 *
 *      PWM Output = TIM5 (20ms PWM)
 *
 *******************************************************************/

#ifndef INC_MG90S_SERVICE_H_
#define INC_MG90S_SERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"

typedef enum {
	ServoDirection_Left = 	0x32U,
	ServoDirection_Center = 0x04BU,
	ServoDirection_Right = 	0x64U
} xServoPosition_t;

extern xServoPosition_t xServoPosition;

/* event flag to activate the servo */
#define FLG_MG90S_ACTIVE	(1 << 0)

extern osEventFlagsId_t evt_Mg90sMotionControlFlag;

/**
 * Main Initialization function
 * @return
 */
uint8_t uMg90sServiceInit();


#endif /* INC_MG90S_SERVICE_H_ */
