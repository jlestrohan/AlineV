/*******************************************************************
 * HC5883_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *******************************************************************/

#include <HMC5883_service.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include <stdbool.h>

/* Default I2C address */
#define HMC5883_I2C_ADDR			0xD0

static I2C_HandleTypeDef *_hi2cxHandler;

static uint8_t devAddr;
static uint8_t buffer[6];
static uint8_t mode;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t HMC5883_taskHandle;
static osStaticThreadDef_t HMC5883TaControlBlock;
static uint32_t HMC5883TaBuffer[256];
static const osThreadAttr_t HMC5883Ta_attributes = {
		.name = "HMC5883ServiceTask",
		.stack_mem = &HMC5883TaBuffer[0],
		.stack_size = sizeof(HMC5883TaBuffer),
		.cb_mem = &HMC5883TaControlBlock,
		.cb_size = sizeof(HMC5883TaControlBlock),
		.priority = (osPriority_t) osPriorityLow1, };


/**
 *	HC5883 Main task
 * @param argument
 */
static void HMC5883Task_Start(void *argument)
{

	for (;;) {

		osDelay(10);
	}
}

/**
 * Main HMC5883_Initialize
 */
uint8_t HMC5883_Initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	/* creation of HMC5883Sensor_task */
	HMC5883_taskHandle = osThreadNew(HMC5883Task_Start, NULL, &HMC5883Ta_attributes);
	if (!HMC5883_taskHandle) {
		loggerE("HMC5883 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HMC5883 - Initialization complete");
	return (EXIT_SUCCESS);
}
