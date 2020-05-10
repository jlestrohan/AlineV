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
#include "uvLed_service.h"
#include <stdio.h>

#define NUMBER_INIT_ATTEMPTS 2 /* how many trimes we retry before taking an action of avoidance */

/********************************************************************
 * Extern variables from the different sensors and devices
 */
MotorData_t MotorData; /* extern */
osEventFlagsId_t evt_Mg90sMotionControlFlag; /* extern */
xServoPosition_t xServoPosition; /* extern */
osMessageQueueId_t xMessageQueueMotorMotion; /* extern */
osMessageQueueId_t xQueuePWMControlHnd; /* extern */

/********************************************************************/

/* functions definitions */
static void _vMotionForwardRules();
static void _vMotionBackwardRules();
static void _vMotionIdleRules();

static MotorMotion_t motorMotion;
static xServoPattern_t xServopattern;
static HR04_SensorsActive_t xSensorActive;

osEventFlagsId_t xEventFlagNavControlMainComn, xEventFlagDecisionControlMainCom;

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
/** to the current motion.... */
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
/** special event occurs */
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
	printf("Starting navControl task...\n\r");

	for (;;)
	{
		/*******************************************************************************************************************/
		/** FRONT SERVO CONTROL + HCVSR ACTIVATION   																	   */

		if (MC_MOTORS_FORWARD) {
			_vMotionForwardRules();

		} else if (MC_MOTORS_BACKWARD) {
			_vMotionBackwardRules();
		} else {
			_vMotionIdleRules();
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
	for(;;)
	{

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
	xEventFlagDecisionControlMainCom = osEventFlagsNew(NULL);
	if (xEventFlagDecisionControlMainCom == NULL) {
		printf("Decision Control Event Flag Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	xEventFlagNavControlMainCom = osEventFlagsNew(NULL);
	if (xEventFlagNavControlMainCom == NULL) {
		printf("Nav Control Event Flag Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
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
static void _vMotionForwardRules()
{
	/* activates the three patterns mode by default when motion is forward */
	xServopattern = SERVO_PATTERN_THREE_PROBES;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, osWaitForever);

	xSensorActive = HCSR04_US_FRONT_ONLY;
	osMessageQueuePut(xQueuePWMControlHnd, &xSensorActive, 0U, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_SET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 *
 */
static void _vMotionBackwardRules()
{
	/* front servo pattern idle */
	xServopattern = SERVO_PATTERN_RETURN_CENTER;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, osWaitForever);

	xSensorActive = HCSR04_US_REAR_ONLY;
	osMessageQueuePut(xQueuePWMControlHnd, &xSensorActive, 0U, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_SET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 *
 */
static void _vMotionIdleRules()
{
	/* servo returns to center */
	xServopattern = SERVO_PATTERN_RETURN_CENTER;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, osWaitForever);

	/* we cut down all US sensors */
	xSensorActive = HCSR04_US_ALL_STOP;
	osMessageQueuePut(xQueuePWMControlHnd, &xSensorActive, 0, osWaitForever);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_UNSET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}
