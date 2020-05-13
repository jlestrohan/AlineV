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
#include <stdbool.h>
#include "IRQ_Handler.h"

#define NUMBER_INIT_ATTEMPTS 2 /* how many trimes we retry before taking an action of avoidance */

/********************************************************************
 * Extern variables from the different sensors and devices
 */
MotorData_t MotorData; /* extern */
osEventFlagsId_t evt_Mg90sMotionControlFlag; /* extern */
xServoPosition_t xServoPosition; /* extern */
osMessageQueueId_t xMessageQueueMotorMotion; /* extern */

/********************************************************************/

/* functions definitions */
static void _vMotionForwardRules();
static void _vMotionBackwardRules();
static void _vMotionIdleRules();
//static uint8_t _vMotionSeekEscapePath();

static MotorMotion_t motorMotion;
static xServoPattern_t xServopattern;
static HR04_SensorsActive_t xSensorActive;
IrqHCSRAcquisition_t sensorData;
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
	printf("Starting navControl task...\n\r");
	uint8_t avoidance = false; /* avoidance process ongoing ? */

	/* FINITE STATE MACHINE TO CLEAN UP AND REWRITE THE PROPER WAY !!! */

	for (;;)
	{
		/*******************************************************************************************************************/
		/** FRONT SERVO CONTROL + HCVSR ACTIVATION
		 */

		if (MC_MOTORS_FORWARD && !avoidance) {
			_vMotionForwardRules(); /* only sensors and servos */
		}
		if (MC_MOTORS_BACKWARD) {
			_vMotionBackwardRules();/* only sensors and servos */
		}
		if (MC_MOTORS_IDLE) {
			_vMotionIdleRules();/* only sensors and servos */
		}

		/*******************************************************************************************************************/
		/** GROUND HOLE AVOIDANCE CONTROL
		 */
		if (MC_MOTORS_FORWARD) {
			if (HR04_SensorsData.dist_bottom > US_BOTTOM_SENSOR_HOLE_MIN_STOP_CM) {
				motorMotion = MOTOR_MOTION_IDLE;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
			}
		}


		/*******************************************************************************************************************/
		/** FRONT MOTION OBSTACLE AVOIDANCE CONTROL
		 *
		 *TODO: The ESP32 is able to send commands for an immediate stop when its 2 I2C TOF sensors detect a low obstacle in front
		 * In such case an order is transmitted to activate avoidance.
		 */

		/* FRONT SENSOR BEHAVIOUR */
		if ((MC_MOTORS_FORWARD) && (!avoidance) && HR04_SensorsData.dist_front > 2) { /* forward and not already avoiding ? */


			if (sensorData.last_capt_sensor == HR04_SONAR_FRONT) {

				/* no obstacle anywhere we go full speed */
				/*if (sensorData.last_capt_value >= US_FRONT_MIN_WARNING_CM)
				{
					motorMotion = MOTOR_SPEED_NORMAL;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
				/*}*/

				if (sensorData.last_capt_value < US_FRONT_MIN_WARNING_CM) {
					motorMotion = MOTOR_SPEED_REDUCE_WARNING;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
					//printf("incident found front, servo at %lu, distance at: %d", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
				}
				if (sensorData.last_capt_value < US_FRONT_MIN_DANGER_CM) {
					motorMotion = MOTOR_SPEED_REDUCE_DANGER;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
					//printf("incident found front, servo at %lu, distance at: %d", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
				}
				if ((sensorData.last_capt_value < US_FRONT_MIN_STOP_CM) || (HR04_SensorsData.dist_left45 < US_FRONT_MIN_STOP_CM) ||
						(HR04_SensorsData.dist_front < US_FRONT_MIN_STOP_CM)) {
					motorMotion = MOTOR_MOTION_IDLE;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever); /* completely stop the motors*/
					printf("incident found front, servo at %lu, distance at: %d\n\r", htim5.Instance->CCR1, HR04_SensorsData.dist_front);
					avoidance = true; /* we set that flag to be able to try to avoid the obstacle */
				}
			}


			/* FRONT DERIVATION IF OBSTACLE IS DETECTED ON THE ANGLES */
			/* once detected a warning zone, we try to follow a heading which is parallel to the obstacle detected at the angle */

		}




		/* avoid flag is set, we try some emergency measures */
		if (avoidance) {
			/* sensors */
			_vMotionBackwardRules();

			/* first we go back for a little */
			osDelay(1000);
			motorMotion = MOTOR_MOTION_BACKWARD;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			_vMotionIdleRules();
			osDelay(1000);
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			/* interrogates sensor to retrieve the best solution to escape, right ? or left ? */

			_vMotionForwardRules();
			/* tru to determine if angle right obstacle is nearer that left */
			//uint8_t direction = _vMotionSeekEscapePath();
			//if (direction == 0) {
			if (HR04_SensorsData.dist_left45 > HR04_SensorsData.dist_right45) { //FIXME Inverted here seek why
				printf("More room to the right");
				osDelay(1000); motorMotion = MOTOR_MOTION_TURN_RIGHT;
			} else {
				printf("More room to the left");
				osDelay(1000); motorMotion = MOTOR_MOTION_TURN_LEFT;
			}
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			osDelay(1000);

			avoidance = false; /* we start again */

			/* idle.. */
			_vMotionIdleRules();
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			/* Formward Rules */
			_vMotionForwardRules();

			/* then forward again */
			_vMotionForwardRules();
			osDelay(1000);	motorMotion = MOTOR_MOTION_FORWARD;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
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
	NavSpecialEvent_t special_event;
	char msg[30];

	for(;;)
	{
		osMessageQueueGet(xMessageQueueDecisionControlMainCom, &special_event, 0U, osWaitForever);

		switch (special_event) {
		case START_EVENT:
			sprintf(msg, "Initiating disinfection program....");
			motorMotion = MOTOR_MOTION_FORWARD;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);

			//if (xQueueEspSerialTX != NULL)
			//	osMessageQueuePut(xQueueEspSerialTX, &msg, 0U, 0U);
			break;

		case STOP_EVENT: default:
			sprintf(msg, "Stopping disinfection program....");
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
			//if (xQueueEspSerialTX != NULL)
			//	osMessageQueuePut(xQueueEspSerialTX, &msg, 0U, 0U);
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
	xMessageQueueDecisionControlMainCom = osMessageQueueNew(1, sizeof(uint8_t), NULL);
	if (xMessageQueueDecisionControlMainCom == NULL) {
		printf("Decision Control MessageQUeue Initialization Failed\n\r");
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

	HAL_TIM_PWM_Start(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_SET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}

/**
 *
 */
static void _vMotionBackwardRules()
{
	/* front servo pattern backward */
	xServopattern = SERVO_PATTERN_RETURN_CENTER;
	osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, osWaitForever);

	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3);

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
	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);

	/* Activates the front LEDS */
	UV_LedStatus_t led_status = UV_LED_STATUS_UNSET;
	osMessageQueuePut(xQueueUVLedStatus, &led_status, 0U, osWaitForever);
}


