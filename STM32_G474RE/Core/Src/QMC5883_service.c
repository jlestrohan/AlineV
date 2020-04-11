/*******************************************************************
 * HC5883_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *******************************************************************/

/*
 * 	10.1 Continuous Mode Setup Example
		• Write Register 0BH by 0x01 (Define Set/Reset period)
		• Write Register 09H by 0x1D (Define OSR = 512, Full Scale Range = 8 Gauss, ODR = 200Hz, set
			continuous measurement mode)
	10.2 Measurement Example
		• Check status register 06H[0] ,”1” means ready.
		• Read data register 00H ~ 05H.
	10.3 Standby Example
		• Write Register 09H by 0x00
	10.4 Soft Reset Example
		• Write Register 0AH by 0x80
	9.2.1.5 Interrupt
		• An interrupt is generated on DRDY pin each time that magnetic field is measured.
			The interrupt can be disabled by set 0AH[0] = 1
 */

#include <stdlib.h>
#include <FreeRTOS.h>
#include <QMC5883_service.h>
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdio.h>
#include "math.h"

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
static void QMC5883lTask_Start(void *argument)
{
	char msg[60];
	osPriority_t prior = osThreadGetPriority(QMC5883_taskHandle);
	sprintf(msg, "Starting QMC5883 service task with priority: %d",  prior);
	loggerI(msg);

	QMC5883_Result result;

	for (;;) {

		result = QMC5883l_ReadData(&qmc1);

		sprintf(msg, "x: %d - y: %d - z: %d - temp: %d - HDG: %d°",
				qmc1.DataX, qmc1.DataY, qmc1.DataZ,  qmc1.DataTemperature,
				(uint8_t)(atan2(qmc1.DataY, qmc1.DataX) * 180 / M_PI));
		loggerI(msg);

		osDelay(200);
	}
}

/**
 * Main QMC5883_Initialize
 */
uint8_t QMC5883l_Initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	/* creation of QMC5883Sensor_task */
	QMC5883_taskHandle = osThreadNew(QMC5883lTask_Start, NULL, &QMC5883Ta_attributes);
	if (!QMC5883_taskHandle) {
		loggerE("QMC5883 Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* default QHM5883 settings */
	if (QCM5883l_Init() != QHM5883_Result_Ok) {
		loggerE("QMC5883 Init procedure Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing QMC5883 Service... Success!");
	return (EXIT_SUCCESS);
}

/**
 * Sensor main initialization function
 */
QMC5883_Result QCM5883l_Init()
{
	uint8_t data[3];
	uint8_t temp = 0;

	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, QMC5883l_ADDRESS, 2, 5) != HAL_OK) {
		return (QHM5883_Result_DeviceNotConnected);
	}

	/* Receive multiple byte */
	if (HAL_I2C_Master_Receive(_hi2cxHandler, QMC5883l_ADDRESS, &temp, 1, 1000) != HAL_OK) {
		return (QHM5883_Result_Error);
	}

	/* Set in continuous mode */
	data[0] = QMC5883l_SET_RESET_PERIOD_REG;
	data[1] = 0x01;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, (uint8_t *)data, 2, 1000) != HAL_OK) {
		return (QHM5883_Result_Error);
	}

	data[0] = QMC5883l_CNTRL_REG_1_ADD;
	data[1] = temp | QMC5883l_MODE_CONTINUOUS | QMC5883l_ODR_200Hz | QMC5883l_RNG_8G | QMC5883l_OSR_512;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, (uint8_t *)data, 2, 1000) != HAL_OK) {
		return (QHM5883_Result_Error);
	}

	return QHM5883_Result_Ok;
}


/**
 * Update all magnetometer data
 */
QMC5883_Result QMC5883l_ReadData(QMC5883 *DataStruct)
{
	uint8_t data[3];
	uint8_t receive_buffer[10];
	char test[40];

	/* check status register, 1 = ready */
	data[0] = QMC5883l_STATUS_REG_ADD;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, (uint8_t *)data, 1, 1000) != HAL_OK) {
		return (QHM5883_Result_Error);
	}

	/* read back value, we're trying to check if there's any data available */
	if (HAL_I2C_Master_Receive(_hi2cxHandler, QMC5883l_ADDRESS, receive_buffer, 1, 1000) != HAL_OK) {
		return (QHM5883_Result_Error);
	}

	DataStruct->DataAvailable = receive_buffer[0];

	if (receive_buffer != 0) {
		data[0] = QMC5883l_DATA_OUTPUT_X_LSB_REG;

		/* Try to transmit via I2C */
		/*if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 1, 1000) != HAL_OK) {
			return (QHM5883_Result_Error);
		}*/

		/* receives 6 bits of data X Y Z */
		if ( HAL_I2C_Mem_Read(_hi2cxHandler, QMC5883l_ADDRESS, QMC5883l_DATA_OUTPUT_X_LSB_REG, I2C_MEMADD_SIZE_8BIT, receive_buffer, 9, 1000)) {
			return (QHM5883_Result_Error);
		}
		//receive_buffer[0] : MSB X data
		//receive_buffer[1] : LSB X data
		DataStruct->DataX = receive_buffer[0] | receive_buffer[1]<<8; /* combine the two in 1 16 bits */
		DataStruct->DataY = receive_buffer[2] | receive_buffer[3]<<8; /* combine the two in 1 16 bits */
		DataStruct->DataZ = receive_buffer[4] | receive_buffer[5]<<8; /* combine the two in 1 16 bits */

		DataStruct->DataTemperature = receive_buffer[7] | receive_buffer[8]<<8; /* combine the two in 1 16 bits */

	}

	return QHM5883_Result_Ok;
}



