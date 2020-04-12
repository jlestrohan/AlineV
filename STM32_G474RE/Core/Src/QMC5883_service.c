/*******************************************************************
 * HC5883_service.c
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *******************************************************************/

/**
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

		/*sprintf(msg, "x: %d - y: %d - z: %d - temp: %d - HDG: %d°",
				qmc1.DataX, qmc1.DataY, qmc1.DataZ,  qmc1.DataTemperature,
				(uint8_t)QMC5883l_Azimuth(qmc1.DataX, qmc1.DataY));*/
		//loggerI(msg);

		osDelay(200);
	}
}

/**
 * Main QMC5883_Initialize
 * @param hi2cx
 * @return
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
	if (QCM5883l_Init() != QMC5883l_Result_Ok) {
		loggerE("QMC5883 Init procedure Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing QMC5883 Service... Success!");
	return (EXIT_SUCCESS);
}

/**
 * Sensor main initialization function
 * @return
 */
QMC5883_Result QCM5883l_Init()
{
	uint8_t temp = 0;

	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, QMC5883l_ADDRESS, 2, 5) != HAL_OK) {
		return (QMC5883l_Result_DeviceNotConnected);
	}

	/* Receive multiple byte */
	if (HAL_I2C_Master_Receive(_hi2cxHandler, QMC5883l_ADDRESS, &temp, 1, 1000) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}

	/* sets modes */
	if (QMC5883l_SetMode(QMC5883l_MODE_CONTINUOUS, QMC5883l_ODR_200Hz, QMC5883l_RNG_8G, QMC5883l_OSR_512) != QMC5883l_Result_Ok) {
		return (QMC5883l_Result_Error_Cannot_Set_Mode);
	}

	/* disables interrupts */
	if (QMC5883l_SetInterrupt(false) != QMC5883l_Result_Ok) {
		return (QMC5883l_Result_Error_Cannot_Set_Interrupt);
	}

	/* set reset period as per recommended in the DSHT */
	if (QMC5883l_SetResetPeriod() != QMC5883l_Result_Ok) {
		return (QMC5883l_Result_Error_Cannot_Set_ResetPeriod);
	}

	return QMC5883l_Result_Ok;
}


/**
 * Update all magnetometer data
 * @param DataStruct
 * @return
 */
QMC5883_Result QMC5883l_ReadData(QMC5883 *DataStruct)
{
	uint8_t data[6];

	/* check status register, 1 = ready */
	/*if ( HAL_I2C_Mem_Read(_hi2cxHandler, QMC5883l_ADDRESS, QMC5883l_STATUS_REG_ADD, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY)) {
		return (QHM5883_Result_Error);
	}*/

	//DataStruct->DataAvailable = data[0];

	/*if (data[0] != 0) {*/
	//data[0] = QMC5883l_DATA_OUTPUT_X_LSB_REG;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, 0x00, 1, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}

	/* receives 6 bits of data X Y Z */
	if ( HAL_I2C_Master_Receive(_hi2cxHandler, QMC5883l_ADDRESS, data, 6, HAL_MAX_DELAY)) {
		return (QMC5883l_Result_Error);
	}

	DataStruct->DataX = data[0] | data[1] << 8;
	DataStruct->DataY = data[2] | data[3] << 8;
	DataStruct->DataZ = data[4] | data[5] << 8;

	data[0] = QMC5883l_TEMP_OUTPUT_16REG;
	/* Try to transmit via I2C */
	/*if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 1, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}*/

	/* receives 2 bytes of data temp LSB temp MSB */
	/*if ( HAL_I2C_Master_Receive(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY)) {
		return (QMC5883l_Result_Error);
	}*/
	HAL_I2C_Mem_Read(_hi2cxHandler, QMC5883l_ADDRESS, QMC5883l_TEMP_OUTPUT_16REG, I2C_MEMADD_SIZE_8BIT, data, 2, HAL_MAX_DELAY);
	DataStruct->DataTemperature = data[0] | data[1] << 8 ;

	//}

	return QMC5883l_Result_Ok;
}

/**
 * Sets the device in standby mode
 * @return
 */
QMC5883_Result QMC5883l_StandBy()
{
	uint8_t data[2];
	data[0] = QMC5883l_CNTRL_REG_1_ADD;
	data[1] = QMC5883l_MODE_STANDBY;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}
	return (QMC5883l_Result_Ok);
}

/**
 * Soft Resets the Device
 * @return
 */
QMC5883_Result QMC5883l_SoftReset()
{
	uint8_t data[2];
	data[0] = QMC5883l_CNTRL_REG2_ADD;
	data[1] = QCM5883l_SOFT_RST;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}
	return (QMC5883l_Result_Ok);
}

/**
 * Sets device modes
 * See .h
 * @param mode
 * @param odr
 * @param rng
 * @param osr
 * @return
 */
QMC5883_Result QMC5883l_SetMode(QMC5883l_MODE_t mode, QMC5881l_ODR_t odr, QMC5883l_RNG_t rng, QMC5883l_OSR_t osr)
{
	uint8_t data[2];
	data[0] = QMC5883l_CNTRL_REG_1_ADD;
	data[1] = mode | odr | rng | osr;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}
	return (QMC5883l_Result_Ok);
}

/**
 * returns azimuth
 * @param x
 * @param y
 * @return
 */
uint8_t QMC5883l_Azimuth(uint16_t *x, uint16_t *y)
{
	float azimuth = (uint8_t)(atan2((int)*y,(int)*x) * 180.0/M_PI);
	return (azimuth < 0?360 + azimuth:azimuth);
}

/**
 * Enables/disable interrupt flag
 * @param flag
 * @return
 */
QMC5883_Result QMC5883l_SetInterrupt(uint8_t flag) /* true/false */
{
	uint8_t data[2];
	data[0] = QMC5883l_CNTRL_REG2_ADD;
	uint8_t rcvdt = 0;

	HAL_I2C_Mem_Read(_hi2cxHandler, QMC5883l_ADDRESS, data[0], I2C_MEMADD_SIZE_8BIT, &rcvdt, 1, HAL_MAX_DELAY);

	data[1] = flag ? rcvdt | QCM5883l_INT_ENB : rcvdt;
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}

	return (QMC5883l_Result_Ok);
}

/**
 * Set/Reset Period (recommended 0x01)
 * @return
 */
QMC5883_Result QMC5883l_SetResetPeriod()
{
	uint8_t data[2];
	data[0] = QMC5883l_SET_RESET_PERIOD_REG;
	data[1] = 0x01;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK) {
		return (QMC5883l_Result_Error);
	}
	return (QMC5883l_Result_Ok);
}



