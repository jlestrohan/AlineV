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

/********************************************************************
 * Extern variables from the different sensors and devices
 */
MotorData_t MotorData; /* extern */
osEventFlagsId_t evt_Mg90sMotionControlFlag; /* extern */
xServoPosition_t xServoPosition; /* extern */
osMessageQueueId_t xMessageQueueMotorMotion; /* extern */

/********************************************************************/

FSM_Status_t FSM_IA_STATUS; /* default idle */

static MotorMotion_t motorMotion;
static xServoPattern_t xServopattern;


/* a few macros */
#define MC_MOTORS_FORWARD	(((MotorData.motorMotion_Left == MOTOR_MOTION_FORWARD) && (MotorData.motorMotion_Right == MOTOR_MOTION_FORWARD)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_BACKWARD	(((MotorData.motorMotion_Left == MOTOR_MOTION_BACKWARD) && (MotorData.motorMotion_Right == MOTOR_MOTION_BACKWARD)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_IDLE 		(((MotorData.motorMotion_Left == MOTOR_MOTION_IDLE) && (MotorData.motorMotion_Right == MOTOR_MOTION_IDLE)) && \
		((MotorData.currentSpeedLeft == 0) && (MotorData.currentSpeedRight == 0)))


/**
 * FINITE STATE MACHINE THAT RECORDS THE CURRENT STATUS OF THE ANDROID
 *
 *
 */



/*********************************************************************/
/* NORMAL MOTION TASK DEFINITION */
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


/**
 * Main Nav Service Task
 * @param vParameters
 */
void vNavControlNormalMotionTask(void *vParameters)
{
	printf("Starting navControl task...\n\r");

	for (;;)
	{
		/* monitoring the HC-SR04 and handling the motors */
		//if (MotorData.motorMotion_Left == MotorMotion_Forward && MotorData.motorMotion_Right == MotorMotion_Forward) {
		/* handling of the front sonar */
		//if (HR04_SensorsData.distance < 20) {
		/* distance detector under 30 cm, shall we stop ? */
		//	osEventFlagsClear(xEventMotorsForward, MOTORS_FORWARD_ACTIVE); /* stop the motors*/
		//} else {
		//	osEventFlagsSet(xEventMotorsForward, MOTORS_FORWARD_ACTIVE); /* Starts the motors*/
		//}
		//}

		/* activates the front servo in case of motion forward */

		/*******************************************************************************************************************/
		/** FRONT SERVO CONTROL + HCVSR ACTIVATION   																	   */

		if (MC_MOTORS_FORWARD) {
			/* activates the three patterns mode by default when motion is forward */
			xServopattern = SERVO_PATTERN_THREE_PROBES;
			osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);

			/* we stop the rear US sensor */
			HAL_TIM_PWM_Start(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3);

		} else if (MC_MOTORS_BACKWARD) {
			//FIXME: factorize all that */
			HAL_TIM_PWM_Start(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3); /* we start rear ultrasonic sensor measure */
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3); /* and we stop the other two */
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);

		} else {
			xServopattern = SERVO_PATTERN_IDLE;
			osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);

			/* we cut down all US sensors */
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_REAR, TIM_CHANNEL_3); /* we start rear ultrasonic sensor measure */
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3); /* and we stop the other two */
			HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_BOTTOM, TIM_CHANNEL_3);
		}

		/**
		 * * Bottom flag prevents the android to fall to its death, let's handle this */
		if ((HR04_SensorsData.sonarNum == HR04_SONAR_BOTTOM) && (HR04_SensorsData.distance > 10)) {
			motorMotion = MOTOR_MOTION_IDLE;
			osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U); /* completely stop the motors*/
		}

		/*******************************************************************************************************************/
		/** FRONT MOTION OBSTACLE AVOIDANCE CONTROL 																	   */

		if ((MC_MOTORS_FORWARD) && (FSM_IA_STATUS == statusRUNNING)) { /* forward and not already avoiding ? */
			/* front sensing, let's check when servo is directed to the front */
			if (xServoPosition == SERVO_DIRECTION_CENTER) {
				if ((HR04_SensorsData.sonarNum == HR04_SONAR_FRONT) && (HR04_SensorsData.distance < US_FRONT_MIN_STOP_CM)) {
					/* distance front is less than 20cm we need to take action... */
					FSM_IA_STATUS = statusAVOIDANCE; /* let's change mode */

					/* first we set motors idle */
					motorMotion = MOTOR_MOTION_IDLE;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);

					xServopattern = SERVO_PATTERN_IDLE;
					osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);

					/* blink front leds */
					osEventFlagsClear(xEventUvLed, FLG_UV_LED_ACTIVE);
					osDelay(150);
					osEventFlagsSet(xEventUvLed, FLG_UV_LED_ACTIVE);
					osDelay(150);
					osEventFlagsClear(xEventUvLed, FLG_UV_LED_ACTIVE);
					osDelay(150);
					osEventFlagsSet(xEventUvLed, FLG_UV_LED_ACTIVE);
					osDelay(150);
					osEventFlagsClear(xEventUvLed, FLG_UV_LED_ACTIVE);
					osDelay(2000);

					/* we go back */
					motorMotion = MOTOR_MOTION_BACKWARD;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
					osDelay(1750);

					/* we idle again */
					motorMotion = MOTOR_MOTION_IDLE;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
					osDelay(1000);

					/* next we turn left */
					motorMotion = MOTOR_MOTION_TURN_LEFT;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
					osDelay(1000); /* for 2 sec for now, waiting for BMP280 to be operational */
					//FIXME: add magnetometer stuff here */

					/* we idle again */
					motorMotion = MOTOR_MOTION_IDLE;
					osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
					osDelay(1000);

					/* we check the sensors for a second */
					HAL_TIM_PWM_Start(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3); /* start acquisition */
					xServopattern = SERVO_PATTERN_THREE_PROBES;
					osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);
					osDelay(1500);
					HAL_TIM_PWM_Stop(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3); /* stop acquisition */

					//FIXME: add here a check of a struct keeping track of what's center, right and left to be able to takle a decision */
					if ((HR04_SensorsData.sonarNum == HR04_SONAR_FRONT) && (HR04_SensorsData.distance > US_FRONT_MIN_STOP_CM)) {
						/* and we restart */
						motorMotion = MOTOR_MOTION_FORWARD;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						xServopattern = SERVO_PATTERN_THREE_PROBES;
						osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);
					} else {
						/* yet no room, let's go back again and turn a little bit */
						motorMotion = MOTOR_MOTION_BACKWARD;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						osDelay(2500);

						/* turn again */
						motorMotion = MOTOR_MOTION_TURN_LEFT;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						osDelay(1500); /* for 2 sec for now, waiting for BMP280 to be operational */
						//FIXME: add magnetometer stuff here */

						motorMotion = MOTOR_MOTION_IDLE;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						osDelay(1000);

						/* lets resume */
						HAL_TIM_PWM_Start(HTIM_ULTRASONIC_FRONT, TIM_CHANNEL_3); /* start acquisition */
						motorMotion = MOTOR_MOTION_FORWARD;
						osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
						xServopattern = SERVO_PATTERN_THREE_PROBES;
						osMessageQueuePut(xQueueMg90sMotionOrder, &xServopattern, 0U, 0U);
						FSM_IA_STATUS = statusRUNNING;
					}
				}
			}

			/* Servo a gauche 45° ou a droite 45° on détecte un obstacle a moins de 35cm on ralentit */
			//if ((HR04_SensorsData.sonarNum == HR04_SONAR_FRONT) && (HR04_SensorsData.distance < 35)) {
			//	motorMotion = MOTOR_SPEED_REDUCE_20
			//	osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
			//} else {
			//	motorMotion = MOTOR_SPEED_NORMAL;
			//	osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
			//}
		}

		/* when motors are backward we need to sop that motion if the rear sensor detects an obstacle < 20 cm */
		if (MC_MOTORS_BACKWARD) {
			if ((HR04_SensorsData.sonarNum == HR04_SONAR_REAR) && (HR04_SensorsData.distance < 20)) {
				/* distance front is less than 20cm we need to take action... */
				/* first we set motors idle */
				motorMotion = MOTOR_MOTION_IDLE;
				osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, 0U);
			}
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
	/* creation of xNavControlNormalMotion Task */
	xNavControlNormalMotionTaskHandle = osThreadNew(vNavControlNormalMotionTask, NULL, &NavControlNormalMotionTa_attributes);
	if (xNavControlNormalMotionTaskHandle == NULL) {
		printf("Nav Control Normal Motion Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Nav Control Service... Success!\n\r");
	return (EXIT_SUCCESS);
}

/**
 *	Asks the servo to set itself to the 45 right angle and sample a measure, returns true if distance < what's set in the preferences
 * @return
 */
uint8_t uCheckObstacleRight45()
{

}

