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

/* event flag to activate the servo */
#define FLG_MG90S_ACTIVE	(1 << 0)
extern osEventFlagsId_t evt_Mg90sIsActive;

/**
 * Main Initialization function
 * @return
 */
uint8_t uMg90sServiceInit();


#endif /* INC_MG90S_SERVICE_H_ */
