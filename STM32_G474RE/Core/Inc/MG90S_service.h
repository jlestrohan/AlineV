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
	SERVO_PATTERN_IDLE,				/* stops any motion stays in place */
	SERVO_PATTERN_RETURN_CENTER,	/* returns to center */
	SERVO_PATTERN_LEFT45,
	SERVO_PATTERN_LEFT90,
	SERVO_PATTERN_RIGHT45,
	SERVO_PATTERN_RIGHT90,
	SERVO_PATTERN_THREE_PROBES,		/* starts -45 0 +45 pattern */
	SERVO_PATTERN_HALF_RADAR		/* moves from right to left in a circular motion */
} xServoPattern_t;

typedef enum {
	SERVO_DIRECTION_RIGHT90 		= 	25U,
	SERVO_DIRECTION_RIGHT45 		= 	40U,
	SERVO_DIRECTION_CENTER 			= 	74U,
	SERVO_DIRECTION_LEFT45			= 	110U,
	SERVO_DIRECTION_LEFT90			= 	125U
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
