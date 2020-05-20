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

/**
 * Nav Status
 */
typedef enum {
	NAV_STATUS_IDLE ,    //!< NAV_STATUS_IDLE
	NAV_STATUS_STARTING, //!< NAV_STATUS_STARTING
	NAV_STATUS_EXPLORING,//!< NAV_STATUS_EXPLORING
	NAV_STATUS_AVOIDING, //!< NAV_STATUS_AVOIDING
	NAV_STATUS_CANCELLING//!< NAV_STATUS_CANCELLING
} NavigationStatus_t;
static NavigationStatus_t xCurrentNavStatus;

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
//static uint8_t _vMotionSeekEscapePath();

static MotorMotion_t motorMotion;
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
void vNavControlNormalMotionTask(void *vParameters)
{
	printf("Starting Navigation Control task...\n\r");

	uint16_t currentBearing;

	xEventFlagNavControlMainCom = osEventFlagsNew(NULL);
	if (xEventFlagNavControlMainCom == NULL) {
		printf("Nav Control Event Flag Initialization Failed\n\r");
		Error_Handler();
	}


	for (;;)
	{
		/*******************************************************************************************************************/
		/** FINITE STATE MACHINE **/
		switch (xCurrentNavStatus) {

		case NAV_STATUS_EXPLORING:

			/*******************************************************************************************************************/
			/** FRONT SERVO CONTROL + HCVSR ACTIVATION
			 */
			osMutexAcquire(mMotorDataMutex, osWaitForever);
			if (MC_MOTORS_FORWARD) {
				_vServoLedMotionForwardRules(); /* only sensors and servos */
			}
			if (MC_MOTORS_BACKWARD) {
				_vServoLedMotionBackwardRules();/* only sensors and servos */
			}
			if (MC_MOTORS_IDLE) {
				_vServoLedMotionIdleRules();/* only sensors and servos */
			}
			osMutexRelease(mMotorDataMutex);

			/*******************************************************************************************************************/
			/** GROUND HOLE AVOIDANCE CONTROL
			 */
			osMutexAcquire(mMotorDataMutex, osWaitForever);
			if (MC_MOTORS_FORWARD) {
				osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
				if (HR04_SensorsData.dist_bottom > US_BOTTOM_SENSOR_HOLE_MIN_STOP_CM) {
					motorMotion = MOTOR_MOTION_IDLE;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				}
				osMutexRelease(mHR04_SensorsDataMutex);
			}
			osMutexRelease(mMotorDataMutex);


			/*******************************************************************************************************************/
			/** FRONT MOTION OBSTACLE AVOIDANCE CONTROL
			 *
			 *TODO: The ESP32 is able to send commands for an immediate stop when its 2 I2C TOF sensors detect a low obstacle in front
			 * In such case an order is transmitted to activate avoidance.
			 */


			/* FRONT SENSOR BEHAVIOUR */
			osMutexAcquire(mMotorDataMutex, osWaitForever);

			if (HR04_SensorsData.dist_front < US_FRONT_MIN_WARNING_CM) {
				osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
				motorMotion = MOTOR_SPEED_REDUCE_WARNING;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				//printf("incident found front, servo at %lu, distance at: %d", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
			}
			if (HR04_SensorsData.dist_front < US_FRONT_MIN_DANGER_CM) {
				motorMotion = MOTOR_SPEED_REDUCE_DANGER;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				//printf("incident found front, servo at %lu, distance at: %d", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
			}
			if (HR04_SensorsData.dist_front < US_FRONT_MIN_STOP_CM) {
				motorMotion = MOTOR_MOTION_IDLE;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				printf("incident found front, servo at %lu, distance at: %d\n\r", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
				xCurrentNavStatus = NAV_STATUS_AVOIDING; /* we set that flag to be able to try to avoid the obstacle */
			}

			/* FRONT DERIVATION IF OBSTACLE IS DETECTED ON THE ANGLES */
			/* once detected a warning zone, we try to follow a heading which is parallel to the obstacle detected at the angle */
			/* FIXME: for now angle detection = stop motion! */
			//			/* WAITING FOR CMPS12 inertial sensor to make that better */

			if ((HR04_SensorsData.dist_left45 < US_ANGLE_MIN_WARNING_CM) || (HR04_SensorsData.dist_right45 < US_ANGLE_MIN_WARNING_CM)) {
				motorMotion = MOTOR_SPEED_REDUCE_WARNING;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				printf("incident found angles, reducing speed to MOTOR_SPEED_REDUCE_WARNING\n\r");
			}
			if ((HR04_SensorsData.dist_left45 < US_ANGLE_MIN_DANGER_CM) || (HR04_SensorsData.dist_right45 < US_ANGLE_MIN_DANGER_CM)) {
				motorMotion = MOTOR_SPEED_REDUCE_DANGER;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				printf("incident found angles, reducing speed to MOTOR_SPEED_REDUCE_DANGER\n\r");
			}
			if  ((HR04_SensorsData.dist_left45 < US_ANGLE_MIN_STOP_CM) || (HR04_SensorsData.dist_right45 < US_ANGLE_MIN_STOP_CM)) {
				motorMotion = MOTOR_MOTION_IDLE;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				printf("incident found angles, servo %lu, distL: %d, distR: %d\n\r", htim5.Instance->CCR1, HR04_SensorsData.dist_left45, HR04_SensorsData.dist_right45);
				xCurrentNavStatus = NAV_STATUS_AVOIDING; /* we set that flag to be able to try to avoid the obstacle */
			}
			osMutexRelease(mHR04_SensorsDataMutex);


			osMutexRelease(mMotorDataMutex);

			break;




			/* avoid flag is set, we try some emergency measures */
		case NAV_STATUS_AVOIDING :
			/* sensors */
			_vServoLedMotionBackwardRules();

			/* first we go back for a little */
			osDelay(1000);
			motorMotion = MOTOR_MOTION_BACKWARD;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			_vServoLedMotionIdleRules();
			osDelay(1000);
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			/* interrogates sensor to retrieve the best solution to escape, right ? or left ? */

			_vServoLedMotionForwardRules();
			/* tru to determine if angle right obstacle is nearer that left */
			//uint8_t direction = _vMotionSeekEscapePath();
			//if (direction == 0) {
			osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
			if (HR04_SensorsData.dist_left45 < HR04_SensorsData.dist_right45) { //FIXME Inverted here seek why
				printf("Detected More space to the right, turning right...");
				osDelay(1000);
				motorMotion = MOTOR_MOTION_TURN_RIGHT;
			} else {
				printf("Detected More space to the left, turning left...");
				osDelay(1000); motorMotion = MOTOR_MOTION_TURN_LEFT;
			}
			osMutexRelease(mHR04_SensorsDataMutex);
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			osDelay(1000);

			/* idle.. */
			_vServoLedMotionIdleRules();
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			/* saves the current bearing */
			osMutexAcquire(mCMPS12_SensorDataMutex, osWaitForever);
			//currentBearing = CMPS12_SensorData.CompassBearing;
			osMutexRelease(mCMPS12_SensorDataMutex);

			xCurrentNavStatus = NAV_STATUS_EXPLORING;

			/* Formward Rules */
			_vServoLedMotionForwardRules();

			/* then forward again */
			_vServoLedMotionForwardRules();
			osDelay(1000);	motorMotion = MOTOR_MOTION_FORWARD;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			break;
		case NAV_STATUS_CANCELLING:

			break;

		case NAV_STATUS_STARTING:
			/* we wait for all data from the 3 front sensors are collected */
			osMutexAcquire(mHR04_SensorsDataMutex, osWaitForever);
			if (HR04_SensorsData.dist_left45 > 0 && HR04_SensorsData.dist_right45 > 0 && HR04_SensorsData.dist_front > 0)
			{
				/* at this condition we can change special event status to "exploring */
				motorMotion = MOTOR_MOTION_FORWARD;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

				/* saves the current bearing */
				osMutexAcquire(mCMPS12_SensorDataMutex, osWaitForever);
				currentBearing = CMPS12_SensorData.CompassBearing;
				osMutexRelease(mCMPS12_SensorDataMutex);

				/* we have initiated the start sequence, time to switch to explore mode */
				xCurrentNavStatus = NAV_STATUS_EXPLORING;
			}
			osMutexRelease(mHR04_SensorsDataMutex);
			break;

		case NAV_STATUS_IDLE:

			break;

		}


		osDelay(10);
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
			printf("Initiating disinfection program....\n\r");
			xCurrentNavStatus = NAV_STATUS_STARTING;

			/* we save the current bearing to the bearing we wish to go */
			osMutexAcquire(mCMPS12_SensorDataMutex, osWaitForever);
			currentBearing = CMPS12_SensorData.CompassBearing;
			osMutexRelease(mCMPS12_SensorDataMutex);

			/* first we start the motion sensor + front HCSR04 tro fill up the values */
			xSrvpattrn = SERVO_PATTERN_THREE_PROBES;
			osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);
			/* next happens in the main loop above ... */
			break;

		case STOP_EVENT: default:
			sprintf(msg, "Stopping disinfection program....\n\r");
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			xSrvpattrn = SERVO_PATTERN_IDLE;
			osMessageQueuePut(xQueueMg90sMotionOrder, &xSrvpattrn, 0U, osWaitForever);

			xCurrentNavStatus = NAV_STATUS_IDLE;

			break;
		}


		osDelay(20);
	}
	osThreadTerminate(NULL);
}

/**
 * Main Nav Control Initialization routine
 * @return
 */
uint8_t uNavControlServiceInit()
{
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


