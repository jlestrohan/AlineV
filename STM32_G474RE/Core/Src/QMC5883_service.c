/*******************************************************************
 * HC5883_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 * see https://github.com/mechasolution/Mecha_QMC5883L
 *******************************************************************/

#include <stdlib.h>
#include <FreeRTOS.h>
#include <QMC5883_service.h>
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdio.h>

static I2C_HandleTypeDef *_hi2cxHandler;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t QMC5883_taskHandle;
static osStaticThreadDef_t QMC5883TaControlBlock;
static uint32_t QMC5883TaBuffer[256];
static const osThreadAttr_t QMC5883Ta_attributes = {
		.name = "QMC5883ServiceTask",
		.stack_mem = &QMC5883TaBuffer[0],
		.stack_size = sizeof(QMC5883TaBuffer),
		.cb_mem = &QMC5883TaControlBlock,
		.cb_size = sizeof(QMC5883TaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };


/**
 *	QMC5883 Main task
 * @param argument
 */
static void QMC5883Task_Start(void *argument)
{
	char msg[60];
	osPriority_t prior = osThreadGetPriority(QMC5883_taskHandle);
	sprintf(msg, "Starting QMC5883 service task with priority: %d",  prior);
	loggerI(msg);

	for (;;) {

		osDelay(20);
	}
}

/**
 * Main QMC5883_Initialize
 */
uint8_t QMC5883_Initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, QMC5883l_ADDRESS, 2, 5) != HAL_OK) {
		loggerE("QMC5883 Device not ready");
		return (EXIT_FAILURE);
	}

	/* creation of QMC5883Sensor_task */
	QMC5883_taskHandle = osThreadNew(QMC5883Task_Start, NULL, &QMC5883Ta_attributes);
	if (!QMC5883_taskHandle) {
		loggerE("QMC5883 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* default QHM5883 settings */


	loggerI("QMC5883 - Initialization complete");
	return (EXIT_SUCCESS);
}
