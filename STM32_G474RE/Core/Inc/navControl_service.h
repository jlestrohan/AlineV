/*******************************************************************
 * navControl_service.h
 *
 *  Created on: 2 mai 2020
 *      Author: Jack lestrohan
 *
 *      This is the main IA navigation control for the rover.
 *      It controls every aspects of the rover's motion according to
 *      sensors datas and different situations that can possibly occur.
 *
 *******************************************************************/

#ifndef INC_NAVCONTROL_SERVICE_H_
#define INC_NAVCONTROL_SERVICE_H_

#include <stdint.h>

/**
 * Finite State Machine indicating the current status of the "mission"
 * This is public
 */
typedef enum {
	statusIDLE,
	statusRUNNING,  //!< statusRUNNING
	statusAVOIDANCE,//!< statusAVOIDANCE
} FSM_Status_t;

extern FSM_Status_t FSM_IA_STATUS; /* default idle */

uint8_t uNavControlServiceInit();

#endif /* INC_NAVCONTROL_SERVICE_H_ */
