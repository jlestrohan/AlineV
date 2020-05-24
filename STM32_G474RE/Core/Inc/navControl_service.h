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
#include "cmsis_os2.h"

/**
 * Finite State Machine Flags indicating the current status of the "mission"
 */
#define FLAG_NAV_STATUS_IDLE							(1 << 0)
#define FLAG_NAV_STATUS_STARTING						(1 << 1)
#define FLAG_NAV_STATUS_STARTING_DIST_CHECK1_SUCCESS	(1 << 2) /* we did a first round with the front servo that did not reveal anything wrong */
#define FLAG_NAV_STATUS_STARTING_DIST_CHECK1_ERROR		(1 << 3) /* an obstacle has been detected at a certain angle during this check we need to take action */
#define FLAG_NAV_STATUS_RUNNING							(1 << 4)
#define FLAG_NAV_STATUS_AVOIDING						(1 << 5)

/**
 * Nav Status - handled by a rolling no suspended task
 */
typedef enum {
	NAV_STATUS_IDLE ,    //!< NAV_STATUS_IDLE
	NAV_STATUS_STARTING, //!< NAV_STATUS_STARTING
	NAV_STATUS_EXPLORING,//!< NAV_STATUS_EXPLORING
	NAV_STATUS_TURNING_TO_DEST_BEARING, /* bot is turning to head to the set direction */
	NAV_STATUS_AVOIDING, //!< NAV_STATUS_AVOIDING
	NAV_STATUS_CORRECT_HEADING,	/* heading correction mode, back to exploring every time */
	NAV_STATUS_CANCELLING//!< NAV_STATUS_CANCELLING
} NavigationStatus_t;

/**
 * These for action buttons
 * handled by queued suspended task
 */
typedef enum {
	START_EVENT,    //!< START_EVENT
	STOP_EVENT,     //!< STOP_EVENT
	AVOIDANCE_EVENT,//!< AVOIDANCE_EVENT
	UNGROUD_EVENT   //!< UNGROUD_EVENT
} NavSpecialEvent_t;

extern osEventFlagsId_t xEventFlagNavControlMainCom; /* used to communicate with the nav control (buttons ?..) */
extern osMessageQueueId_t xMessageQueueDecisionControlMainCom;

extern NavigationStatus_t xCurrentNavStatus;
extern osMutexId_t mCurrentNavStatusMutex;

uint8_t uNavControlServiceInit();

#endif /* INC_NAVCONTROL_SERVICE_H_ */
