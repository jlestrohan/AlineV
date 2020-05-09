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
	SERVO_PATTERN_IDLE,
	SERVO_PATTERN_THREE_PROBES,
	SERVO_PATTERN_HALF_RADAR
} xServoPattern_t;

typedef enum {
	SERVO_DIRECTION_LEFT90 		= 	25U,
	SERVO_DIRECTION_LEFT45 		= 	50U,
	SERVO_DIRECTION_CENTER 		= 	75U,
	SERVO_DIRECTION_RIGHT45		= 	100U,
	SERVO_DIRECTION_RIGHT90		= 	125U
} xServoPosition_t;

/********************************************************/
/** PUBLIC Structure with general service updated infos */
extern xServoPosition_t xServoPosition;
extern osMessageQueueId_t xQueueMg90sMotionOrder;
/********************************************************/

/**
 * Main Initialization function
 * @return
 */
uint8_t uMg90sServiceInit();


#endif /* INC_MG90S_SERVICE_H_ */
