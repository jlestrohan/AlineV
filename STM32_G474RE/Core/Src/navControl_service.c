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

/********************************************************************
 * Extern variables from the different sensors and devices
 */
MotorData_t MotorData;
xServoPosition_t xServoPosition;
osEventFlagsId_t evt_Mg90sMotionControlFlag;

/********************************************************************/

/* a few macros */
#define MC_MOTORS_FORWARD	(((MotorData.motorMotion_Left == MotorMotion_Forward) && (MotorData.motorMotion_Right == MotorMotion_Forward)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_BACKWARD	(((MotorData.motorMotion_Left == MotorMotion_Backward) && (MotorData.motorMotion_Right == MotorMotion_Backward)) && \
		((MotorData.currentSpeedLeft > 0) && (MotorData.currentSpeedRight > 0)))

#define MC_MOTORS_IDLE 		(((MotorData.motorMotion_Left == MotorMotion_Backward) && (MotorData.motorMotion_Right == MotorMotion_Backward)) && \
		((MotorData.currentSpeedLeft == 0) && (MotorData.currentSpeedRight == 0)))


/**
 * FINITE STATE MACHINE THAT RECORDS THE CURRENT STATUS OF THE ANDROID
 *
 *
 */



/*********************************************************************/

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xNavControlServiceTaskHandle;
static osStaticThreadDef_t NavControlServiceTaControlBlock;
static uint32_t NavControlServiceTaBuffer[256];
static const osThreadAttr_t NavControlServiceTa_attributes = {
		.name = "NavControlServiceTask",
		.stack_mem = &NavControlServiceTaBuffer[0],
		.stack_size = sizeof(NavControlServiceTaBuffer),
		.cb_mem = &NavControlServiceTaControlBlock,
		.cb_size = sizeof(NavControlServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_NAVCONTROL, };

/**
 * Main Nav Service Task
 * @param vParameters
 */
void vNavControlServiceTask(void *vParameters)
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
		/** FRONT SERVO CONTROL 																						   */

		if (MC_MOTORS_FORWARD) {
			osEventFlagsSet(evt_Mg90sMotionControlFlag, FLG_MG90S_ACTIVE);
		} else {
			osEventFlagsClear(evt_Mg90sMotionControlFlag, FLG_MG90S_ACTIVE);
		}



		/*******************************************************************************************************************/
		/** HC-SR04 CONTROL
		 * - Selection of actives ones vs inactives ones */
		/* forward motion activates the front and bottom sensors */
		if (MC_MOTORS_FORWARD) {
			osEventFlagsSet(xHcrSr04ControlFlag, FLG_SONAR_FRONT_ACTIVE);
			osEventFlagsSet(xHcrSr04ControlFlag, FLG_SONAR_BOTTOM_ACTIVE);
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_REAR_ACTIVE);
		}

		/* backward motion only activates the rear sensor */
		if (MC_MOTORS_BACKWARD) {
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_FRONT_ACTIVE);
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_BOTTOM_ACTIVE);
			osEventFlagsSet(xHcrSr04ControlFlag, FLG_SONAR_REAR_ACTIVE);
		}

		/* Idle */
		if (MC_MOTORS_IDLE) {
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_FRONT_ACTIVE);
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_BOTTOM_ACTIVE);
			osEventFlagsClear(xHcrSr04ControlFlag, FLG_SONAR_REAR_ACTIVE);
		}


		/**
		 * * Bottom flag prevents the android to fall to its death, let's handle this */
		if ((HR04_SensorsData.sonarNum == HR04_SONAR_BOTTOM) && (HR04_SensorsData.distance > 10)) {
			osEventFlagsSet(xEventMotorsMotion, MOTORS_IDLE); /* completely stop the motors*/
		}



		//xHcrSr04ControlFlag

		osDelay(20);
	}
	osThreadTerminate(xNavControlServiceTaskHandle);
}

/**
 * Main Nav Control Initialization routine
 * @return
 */
uint8_t uNavControlServiceInit()
{
	/* creation of HR04Sensor1_task */
	xNavControlServiceTaskHandle = osThreadNew(vNavControlServiceTask, NULL, &NavControlServiceTa_attributes);
	if (xNavControlServiceTaskHandle == NULL) {
		printf("Nav Control Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Nav Control Service... Success!\n\r");
	return (EXIT_SUCCESS);
}
