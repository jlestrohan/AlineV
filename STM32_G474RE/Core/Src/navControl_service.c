/*******************************************************************
 * navControl_service.c
 *
 *  Created on: 2 mai 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include "navControl_service.h"
#include "main.h"
#include "HCSR04_service.h"
#include <stdlib.h>
#include "configuration.h"
#include "MG90S_service.h"
#include "printf.h"
#include "MotorsControl_service.h"
#include "CMPS12_service.h"
#include "uvLed_service.h"
#include <stdio.h>
#include <stdbool.h>
#include "IRQ_Handler.h"

#define NUMBER_INIT_ATTEMPTS 2 /* how many trimes we retry before taking an action of avoidance */

NavigationStatus_t xCurrentNavStatus; /* made extern to share with datacenter */
osMutexId_t mCurrentNavStatusMutex;

/********************************************************************
 * Extern variables from the different sensors and devices
 */
MotorData_t MotorData; /* extern */

osMessageQueueId_t xQueueMg90sMotionOrder; /* extern */
osMessageQueueId_t xMessageQueueMotorMotion; /* extern */


/* mutexed variables */
HR04_SensorsData_t HR04_SensorsData; /* extern */
osMutexId_t mHR04_SensorsDataMutex; /* extern */

CMPS12_SensorData_t CMPS12_SensorData; /* extern */
osMutexId_t mCMPS12_SensorDataMutex; /* extern */

/********************************************************************/

/* functions definitions */
static void _vServoLedMotionForwardRules();
static void _vServoLedMotionBackwardRules();
static void _vServoLedMotionIdleRules();
static uint8_t _uCheckFrontDangerZone();
static uint8_t uGetNewBearing(int16_t deviationDegrees,  uint16_t *bearing);
uint8_t u_is_ref_bearing_right_of_actual(uint16_t *ref_bearing, uint16_t *actual_bearing);
//static uint8_t _vMotionSeekEscapePath();

MotorData_t lastMotorData; /* extern - records the last before the last events */
osEventFlagsId_t xEventFlagNavControlMainComn;
osMessageQueueId_t xMessageQueueDecisionControlMainCom;

/* a few macros */
#define MC_MOTORS_FORWARD	(((MotorData.motorMotion_Left == MOTOR_MOTION_FORWARD) && (MotorData.motorMotion_Right == MOTOR_MOTION_FORWARD)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_BACKWARD	(((MotorData.motorMotion_Left == MOTOR_MOTION_BACKWARD) && (MotorData.motorMotion_Right == MOTOR_MOTION_BACKWARD)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_IDLE 		(((MotorData.motorMotion_Left == MOTOR_MOTION_IDLE) && (MotorData.motorMotion_Right == MOTOR_MOTION_IDLE)) && \
		((MotorData.currentSpeedLeft == 0) && (MotorData.currentSpeedRight == 0)))



/*********************************************************************/
/* NORMAL NAV MOTION CONTROL TASK DEFINITION */
/** This task controls juste the normal behaviour of the android according
 * to the current motion.... */
static osThreadId_t xNavControlNormalMotionTaskHandle;
static osStaticThreadDef_t NavControlNormalMotionTaControlBlock;
static uint32_t NavControlNormalMotionTaBuffer[256];
static const osThreadAttr_t NavControlNormalMotionTa_attributes = {
		.name = "NavControlNormalMotionTask",
		.stack_mem = &NavControlNormalMotionTaBuffer[0],
		.stack_size = sizeof(NavControlNormalMotionTaBuffer),
		.cb_mem = &NavControlNormalMotionTaControlBlock,
		.cb_size = sizeof(NavControlNormalMotionTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_NAVCONTROL_NORM_MOTION, };

/*********************************************************************/
/* DECISION CONTROL TASK DEFINITION */
/** This task controls the sequence of actions to undertake when any
 * special event occurs */
static osThreadId_t xNavDecisionControlTaskHandle;
static osStaticThreadDef_t xNavDecisionControlTaControlBlock;
static uint32_t xNavDecisionControlTaBuffer[256];
static const osThreadAttr_t xNavDecisionControlTa_attributes = {
		.name = "xNavDecisionControlTask",
		.stack_mem = &xNavDecisionControlTaBuffer[0],
		.stack_size = sizeof(xNavDecisionControlTaBuffer),
		.cb_mem = &xNavDecisionControlTaControlBlock,
		.cb_size = sizeof(xNavDecisionControlTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_NAVCONTROL_DECISION, };


/**
 * Main Nav Service Task - This task controls the behaviour of the robot according to any motion
 * @param vParameters
 */
static void vNavControlNormalMotionTask(void *vParameters)
{
	printf("Starting Navigation Control task...\n\r");

	uint16_t xCurrentBearing, xDestinationBearing;
	//NavigationStatus_t lastNavigationStatus; /* used to keep track of the last known state, ie: turning motion */
	MotorMotion_t motorMotion;

	for (;;)
	{
		/* BEARING - we need to constantly uipdate this */
		MUTEX_CMPS12_TAKE
		xCurrentBearing = CMPS12_SensorData.CompassBearing;
		MUTEX_CMPS12_GIVE


		/** FINITE STATE MACHINE **/
		switch (xCurrentNavStatus) {

		/*---------------------------------------------------------------------------------------------------- */
		/* TRAJECTORY CORRECTION MODE */
		/*---------------------------------------------------------------------------------------------------- */
		case NAV_STATUS_CORRECT_HEADING:

			motorMotion = u_is_ref_bearing_right_of_actual(&xDestinationBearing, &xCurrentBearing)
			? MOTOR_MOTION_FORWARD_LEFT : MOTOR_MOTION_FORWARD_RIGHT;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			MUTEX_NAVSTATUS_TAKE
			xCurrentNavStatus = NAV_STATUS_EXPLORING; /* back to exploring */
			MUTEX_NAVSTATUS_GIVE
			break;

			/*---------------------------------------------------------------------------------------------------- */
			/* EXPLORING MODE */
			/*---------------------------------------------------------------------------------------------------- */
		case NAV_STATUS_EXPLORING:

			/* we try to keep the heading */
			if (xCurrentBearing != xDestinationBearing) {
				MUTEX_NAVSTATUS_TAKE
				xCurrentNavStatus = NAV_STATUS_CORRECT_HEADING; /* back to exploring */
				MUTEX_NAVSTATUS_GIVE
			}

			/* front sensor data check, if we hit an obstacle, we stop and enter avoiding mode */
			MUTEX_HCSR04_TAKE
			if (HR04_SensorsData.dist_front < US_FRONT_MIN_STOP_CM) {
				MUTEX_HCSR04_GIVE

				MUTEX_NAVSTATUS_TAKE
				xCurrentNavStatus = NAV_STATUS_AVOIDING; /* we set that flag to be able to try to avoid the obstacle */
				MUTEX_NAVSTATUS_GIVE

				printf("Obstacle detected front: %d cm\n\r", HR04_SensorsData.dist_front);
				printf("Entering AVOIDING mode..\n\r");
			} else

				/* front sensor detected something at warning zone, we reduce speed */
				if (HR04_SensorsData.dist_front < US_FRONT_MIN_DANGER_CM) {
					MUTEX_HCSR04_GIVE
					printf("Warning ahead distance: %d cm\n\r", HR04_SensorsData.dist_front);
					motorMotion = MOTOR_SPEED_REDUCE_DANGER;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
				} else {
					/* or we set back to front if we're ok */
					MUTEX_HCSR04_GIVE
					/* front sensor detected nada we set full speed */
					/*if (HR04_SensorsData.dist_front >= US_FRONT_MIN_DANGER_CM) { */
					motorMotion = MOTOR_SPEED_NORMAL;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
				}
			MUTEX_HCSR04_GIVE

			break;


			/*---------------------------------------------------------------------------------------------------- */
			/* STARTING MODE */
			/*---------------------------------------------------------------------------------------------------- */
		case NAV_STATUS_STARTING:
			/* we wait for all data from the 3 front sensors are collected */
			MUTEX_HCSR04_TAKE
			_vServoLedMotionForwardRules();
			if (HR04_SensorsData.dist_left45 > 0 && HR04_SensorsData.dist_right45 > 0 && HR04_SensorsData.dist_front > 0)
			{
				/* saves the current bearing */
				xDestinationBearing = xCurrentBearing; /* we sync both */

				/* at this condition we can change special event status to "exploring */
				motorMotion = MOTOR_MOTION_FORWARD;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

				/* we have initiated the start sequence, time to switch to explore mode */
				MUTEX_NAVSTATUS_TAKE
				xCurrentNavStatus = NAV_STATUS_EXPLORING;
				MUTEX_NAVSTATUS_GIVE
			}
			MUTEX_HCSR04_GIVE
			break;

			/*---------------------------------------------------------------------------------------------------- */
			/*  TURN TO SET BEARING */
			/*---------------------------------------------------------------------------------------------------- */
		case  NAV_STATUS_TURNING_TO_DEST_BEARING:
			/* here we should have backuped our last navigation status, we will set it back when we're done turning */

			if (xCurrentBearing != xDestinationBearing) {
				if (xCurrentBearing < xDestinationBearing) {
					motorMotion = MOTOR_MOTION_TURN_RIGHT;
				} else {
					motorMotion = MOTOR_MOTION_TURN_LEFT;
				}
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			} else {
				motorMotion = MOTOR_MOTION_IDLE;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
				osDelay(2000);

				/* and we start again */
				_vServoLedMotionForwardRules();
				motorMotion = MOTOR_MOTION_FORWARD;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

				MUTEX_NAVSTATUS_TAKE
				xCurrentNavStatus = NAV_STATUS_EXPLORING;
				MUTEX_NAVSTATUS_GIVE
			}

			break;

			/*---------------------------------------------------------------------------------------------------- */
			/*  AVOIDANCE MODE */
			/*---------------------------------------------------------------------------------------------------- */
		case NAV_STATUS_AVOIDING:

			/* first IDLE! */
			_vServoLedMotionIdleRules(); /* idle servo + leds rules */
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			osDelay(2000);

			/* then backward */
			if (HR04_SensorsData.dist_rear > US_REAR_MIN_STOP_CM) {
				motorMotion = MOTOR_MOTION_BACKWARD;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
				_vServoLedMotionBackwardRules();
				osDelay(1500); /* for one second */
			}


			/* idle again */
			_vServoLedMotionIdleRules(); /* idle servo + leds rules */
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			_vServoLedMotionForwardRules(); /* starts the sensors again */
			osDelay(1000); /* for a second */

			/* we take a decision, right or left ? */
			MUTEX_HCSR04_TAKE
			if (HR04_SensorsData.dist_left45 < HR04_SensorsData.dist_right45) {
				printf("Detected More space to the right, turning right 30°...");
				xDestinationBearing = uGetNewBearing(40, &xCurrentBearing);
			} else {
				printf("Detected More space to the left, turning left 30°...");
				xDestinationBearing = uGetNewBearing(-40, &xCurrentBearing);
			}
			MUTEX_HCSR04_GIVE

			MUTEX_NAVSTATUS_TAKE
			xCurrentNavStatus = NAV_STATUS_TURNING_TO_DEST_BEARING;
			MUTEX_NAVSTATUS_GIVE

			/* now everything will happen in the FINITE STATE above */
			break;
			/*---------------------------------------------------------------------------------------------------- */



		}
		osDelay(1);
	}
	osThreadTerminate(NULL);
}

/**
 * This task controls the sequence of actions to undertake when any
 * special event occurs
 * @param vParameter
 */
static void vNavDecisionControlTask(void *vParameter)
{
	printf("Starting Navigation Decision Control Task...\n\r");

	xServoPattern_t xSrvpattrn;
	NavSpecialEvent_t special_event;
	MotorMotion_t motorMotion;
	char msg[30];
	uint16_t currentBearing;

	xMessageQueueDecisionControlMainCom = osMessageQueueNew(10, sizeof(uint8_t), NULL);
	if (xMessageQueueDecisionControlMainCom == NULL) {
		printf("Decision Control MessageQueue Initialization Failed\n\r");
		Error_Handler();
	}

	for(;;)
	{
		osMessageQueueGet(xMessageQueueDecisionControlMainCom, &special_event, 0U, osWaitForever);

		/*******************************************************************************************************************/
		/** MAIN PROGRAM CONTROL FINITE STATE MACHINE **/

		switch (special_event) {
		case START_EVENT:
			printf("\n\rInitiating disinfection program....\n\r");

			MUTEX_NAVSTATUS_TAKE
			xCurrentNavStatus = NAV_STATUS_STARTING;
			MUTEX_NAVSTATUS_GIVE

			break;

		case STOP_EVENT: default:
			_vServoLedMotionIdleRules();
			printf("\n\rStopping disinfection program....\n\r");
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			xSrvpattrn = SERVO_PATTERN_IDLE;
			osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);

			MUTEX_NAVSTATUS_TAKE
			xCurrentNavStatus = NAV_STATUS_IDLE;
			MUTEX_NAVSTATUS_GIVE

			break;
		}


		osDelay(1);
	}
	osThreadTerminate(NULL);
}

/**
 * Main Nav Control Initialization routine
 * @return
 */
uint8_t uNavControlServiceInit()
{
	mCurrentNavStatusMutex = osMutexNew(NULL);

	MUTEX_NAVSTATUS_TAKE
	xCurrentNavStatus = NAV_STATUS_IDLE;
	MUTEX_NAVSTATUS_GIVE

	xEventFlagNavControlMainCom = osEventFlagsNew(NULL);
	if (xEventFlagNavControlMainCom == NULL) {
		printf("Nav Control Event Flag Initialization Failed\n\r");
		Error_Handler();
	}

	/* creation of xNavControlNormalMotion Task */
	xNavControlNormalMotionTaskHandle = osThreadNew(vNavControlNormalMotionTask, NULL, &NavControlNormalMotionTa_attributes);
	if (xNavControlNormalMotionTaskHandle == NULL) {
		printf("Nav Control Normal Motion Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xNavDecisionControlTaskHandle Task */
	xNavDecisionControlTaskHandle = osThreadNew(vNavDecisionControlTask, NULL, &xNavDecisionControlTa_attributes);
	if (xNavDecisionControlTaskHandle == NULL) {
		printf("Nav Decision Control Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Nav Control Service... Success!\n\r");
	return (EXIT_SUCCESS);
}

/**
 *
 */
static void _vServoLedMotionForwardRules()
{
	xServoPattern_t xSrvpattrn;

	/* activates the three patterns mode by default when motion is forward */
	xSrvpattrn = SERVO_PATTERN_THREE_PROBES;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_SET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 *
 */
static void _vServoLedMotionBackwardRules()
{
	xServoPattern_t xSrvpattrn;

	/* front servo pattern backward */
	xSrvpattrn = SERVO_PATTERN_IDLE;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_UNSET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 *
 */
static void _vServoLedMotionIdleRules()
{
	xServoPattern_t xSrvpattrn;

	/* servo returns to center */
	xSrvpattrn = SERVO_PATTERN_IDLE;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_UNSET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 * Returns true if we still have danger front and angles
 * @return
 */
static uint8_t _uCheckFrontDangerZone()
{
	MUTEX_HCSR04_TAKE
	if ((HR04_SensorsData.dist_left45 >= US_ANGLE_MIN_WARNING_CM) &&
			(HR04_SensorsData.dist_right45 >= US_ANGLE_MIN_WARNING_CM)	 &&
			(HR04_SensorsData.dist_front >= US_FRONT_MIN_WARNING_CM))
	{
		MUTEX_HCSR04_GIVE
		return EXIT_SUCCESS;
	}
	MUTEX_HCSR04_GIVE
	return EXIT_FAILURE;
}

/**
 *
 * @param newBearing destination bearing
 * @param bearing pointer to the actual variable
 * @return
 */
static uint8_t uGetNewBearing(int16_t deviationDegrees,  uint16_t *bearing)
{
	int16_t result;
	result = *bearing + deviationDegrees;

	if (result > 360) return result - 360;
	else if (result < 0) return (-360-result)*-1;

	return result;
}

/**
 * Return true if the reference bearing is right from the actual bearing, or false of left
 */
uint8_t u_is_ref_bearing_right_of_actual(uint16_t *ref_bearing, uint16_t *actual_bearing)
{
	// how many degrees are we off
	int16_t diff = actual_bearing - ref_bearing;

	if (diff > 180) {
		diff = -360 + diff;
	} else if (diff < -180) {
		diff = 360 + diff;
	}
	return (diff > 0);
}
