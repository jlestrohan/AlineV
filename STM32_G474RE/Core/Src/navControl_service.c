/*******************************************************************
 * navControl_service.c
 *
 *  Created on: 2 mai 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include "navControl_service.h"
#include "HCSR04_service.h"
#include <stdlib.h>
#include "configuration.h"
#include "MG90S_service.h"
#include "freertos_logger_service.h"
#include "MotorsControl_service.h"

/********************************************************************
 * Extern variables from the different sensors and devices
 */
HR04_SensorsData_t HR04_SensorsData;
MotorData_t MotorData;
xServoPosition_t xServoPosition;
/********************************************************************/

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
	for (;;)
	{
		/* monitoring the HC-SR04 and handling the motors */
		/*if (MotorData.motorMotion_Left == MotorMotion_Forward && MotorData.motorMotion_Right == MotorMotion_Forward) {
			/* handling of the front sonar */
			//if (HR04_SensorsData.distance < 20) {
				/* distance detector under 30 cm, shall we stop ? */
			//	osEventFlagsClear(xEventMotorsForward, MOTORS_FORWARD_ACTIVE); /* stop the motors*/
			//} else {
			//	osEventFlagsSet(xEventMotorsForward, MOTORS_FORWARD_ACTIVE); /* Starts the motors*/
			//}
		//}

		/* activates the front servo in case of motion forward */
		if ((MotorData.motorMotion_Left == MotorMotion_Forward && MotorData.motorMotion_Right == MotorMotion_Forward) &&
				(MotorData.currentSpeedLeft > 0 && MotorData.currentSpeedRight > 0)) {
			osEventFlagsSet(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
		} else {
			osEventFlagsClear(evt_Mg90sIsActive, FLG_MG90S_ACTIVE);
		}

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
		loggerE("Nav Control Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing Nav Control Service... Success!");
	return (EXIT_SUCCESS);
}
