/*******************************************************************
 * HC5883_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 * see https://github.com/mechasolution/Mecha_QMC5883L
 *******************************************************************/

#include <HMC5883_service.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdio.h>

// adresses
// HMC5883l - ADDRESS
#define HMC5883l_ADDRESS 0x0D << 1 // or D0

static I2C_HandleTypeDef *_hi2cxHandler;

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
	char msg[40];

	for (;;) {

		osDelay(20);
	}
}

/**
 * Main HMC5883_Initialize
 */
uint8_t HMC5883_Initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, HMC5883l_ADDRESS, 2, 5) != HAL_OK) {
			loggerE("HMC5883 Device not ready");
			return (EXIT_FAILURE);
		}

	/* creation of HMC5883Sensor_task */
	HMC5883_taskHandle = osThreadNew(HMC5883Task_Start, NULL, &HMC5883Ta_attributes);
	if (!HMC5883_taskHandle) {
		loggerE("HMC5883 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	loggerI("HMC5883 - Initialization complete");
	return (EXIT_SUCCESS);
}
