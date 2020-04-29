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
#include "configuration.h"
#include <FreeRTOS.h>
#include <QMC5883_service.h>
#include "freertos_logger_service.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "printf.h"

static I2C_HandleTypeDef *_hi2cxHandler;
//static QMC5883 qmc1; /* Main data structs holding the constantly updated data */

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xQmc5883TaskHandle;
static osStaticThreadDef_t QMC5883TaControlBlock;
static uint32_t QMC5883TaBuffer[512];
static const osThreadAttr_t QMC5883Ta_attributes = {
		.name = "QMC5883ServiceTask",
		.stack_mem = &QMC5883TaBuffer[0],
		.stack_size = sizeof(QMC5883TaBuffer),
		.cb_mem = &QMC5883TaControlBlock,
		.cb_size = sizeof(QMC5883TaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_QMC5883, };

static float Xmin,Xmax,Ymin,Ymax;
static int16_t X, Y, Z;

/**
 *	QMC5883 Main task
 * @param argument
 */
static void vQmc5883lTaskStart(void *argument)
{
	char msg[60];
	loggerI("Starting QMC5883l Service task...");

	//QMC5883_Result res;

	for (;;) {

		QMC5883L_Read_Data(&X, &Y, &Z);

		sprintf(msg, "x: %d - y: %d - z: %d - temp: %d - HDG: %d",
				X, Y, Z,
				QMC5883L_Read_Temperature(),
				QMC5883L_Heading(X, Y));
		loggerI(msg);

		osDelay(500);
	}
}

/**
 * Main Initialization routine
 */
uint8_t uQmc5883lServiceInit(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, QMC5883l_I2C_ADDRESS, 2, 5) != HAL_OK) {
		loggerE("QMC5883 Device not ready");
		return (EXIT_FAILURE);
	}

	QMC5883L_Configure(MODE_CONTROL_CONTINUOUS, OUTPUT_DATA_RATE_50HZ, FULL_SCALE_2G, OVER_SAMPLE_RATIO_512);

	/* creation of QMC5883Sensor_task */
	xQmc5883TaskHandle = osThreadNew(vQmc5883lTaskStart, NULL, &QMC5883Ta_attributes);
	if (!xQmc5883TaskHandle) {
		loggerE("QMC5883 Task Initialization Failed");
		return (EXIT_FAILURE);
	}


	return EXIT_SUCCESS;
}

/**
 * Initializes the sensor parameters
 * @param MODE
 * @param ODR
 * @param RNGE
 * @param OSR
 */
void QMC5883L_Configure(_qmc5883l_MODE MODE, _qmc5883l_ODR ODR, _qmc5883l_RNG RNGE, _qmc5883l_OSR OSR)
{
	QMC5883L_Reset();
	QMC5883L_Write_Reg(QMC5883L_CONFIG_1, MODE | ODR | RNGE | OSR);
}

/**
 * Writes to I2C channel
 * @param reg
 * @param data
 */
void QMC5883L_Write_Reg(uint8_t reg, uint8_t data)
{
	uint8_t Buffer[2]={reg,data};
	HAL_I2C_Master_Transmit(_hi2cxHandler, QMC5883l_I2C_ADDRESS, Buffer, 2, 10);
}

/**
 * Reads I2C register
 * @param reg
 * @return
 */
uint8_t QMC5883L_Read_Reg(uint8_t reg)
{
	uint8_t Buffer[1];
	HAL_I2C_Mem_Read(_hi2cxHandler, QMC5883l_I2C_ADDRESS, reg, 1, Buffer, 1, 10);
	return Buffer[0];
}

/**
 * Reads data from sensor
 * @param MagX
 * @param MagY
 * @param MagZ
 */
void QMC5883L_Read_Data(int16_t *MagX,int16_t *MagY,int16_t *MagZ) /* (-32768 / +32768) */
{
	*MagX=((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_X_LSB) | (((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_X_MSB))<<8));
	*MagY=((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_Y_LSB) | (((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_Y_MSB))<<8));
	*MagZ=((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_Z_LSB) | (((int16_t)QMC5883L_Read_Reg(QMC5883L_DATA_READ_Z_MSB))<<8));
}


/**
 * Returns the current heading
 * @param Xraw
 * @param Yraw
 * @param Zraw
 * @return
 */
uint8_t QMC5883L_Heading(int16_t Xraw, int16_t Yraw)
{
	int16_t X = Xraw, Y = Yraw;
	uint8_t Heading;

	if(X < Xmin) {
		Xmin = X;
	} else if(X > Xmax) {
		Xmax = X;
	}

	if (Y < Ymin) {
		Ymin = Y;
	} else if (Y > Ymax) {
		Ymax = Y;
	}

	/* Bail out if not enough data is available. */
	if (Xmin == Xmax || Ymin == Ymax) {
		return 0.0;
	}

	/* Recenter the measurement by subtracting the average */
	X -= (Xmax + Xmin)/2;
	Y -= (Ymax + Ymin)/2;

	 /* Rescale the measurement to the range observed. */
	float fx = (float)X/(Xmax - Xmin);
	float fy = (float)Y/(Ymax - Ymin);

	/*X = X/(Xmax - Xmin);
	Y = Y/(Ymax - Ymin);*/

	Heading = 180.0*atan2(fy,fx)/M_PI;
	if (Heading <= 0) {
		Heading += 360;
	}

	//EAST
	Heading += QMC5883L_DECLINATION_ANGLE;
	//WEST
	//Heading -= QMC5883L_DECLINATION_ANGLE;

	/*if(Heading <0)
	{Heading += 2*M_PI;}
	else if(Heading > 2*M_PI)
	{Heading -= 2*M_PI;}*/

	return Heading;
}

/**
 * returns the temperature
 * @return
 */
int16_t QMC5883L_Read_Temperature()
{
	return (((int16_t)QMC5883L_Read_Reg(QMC5883L_TEMP_READ_LSB)) | (((int16_t)QMC5883L_Read_Reg(QMC5883L_TEMP_READ_MSB))<<8))/100;
}

/**
 * Resets the Unit
 */
void QMC5883L_Reset()
{
	QMC5883L_Write_Reg(QMC5883_CONFIG_RESET, 0x01);
}

void QMC5883L_InterruptConfig(_qmc5883l_INT INT)
{
	if (INT==INTERRUPT_ENABLE) {
		QMC5883L_Write_Reg(QMC5883L_CONFIG_2, 0x00);
	} else {
		QMC5883L_Write_Reg(QMC5883L_CONFIG_2, 0x01);
	}
}


_qmc5883l_status QMC5883L_DataIsReady()
{
	uint8_t Buffer=QMC5883L_Read_Reg(QMC5883L_STATUS);
	if((Buffer&0x00)==0x00)	  {return NO_NEW_DATA;}
	else if((Buffer&0x01)==0X01){return NEW_DATA_IS_READY;}
	return NORMAL;
}

_qmc5883l_status QMC5883L_DataIsSkipped()
{
	uint8_t Buffer=QMC5883L_Read_Reg(QMC5883L_STATUS);
	if((Buffer&0x00)==0X00)	  {return NORMAL;}
	else if((Buffer&0x04)==0X04){return DATA_SKIPPED_FOR_READING;}
		return NORMAL;
}

_qmc5883l_status QMC5883L_DataIsOverflow()
{
	uint8_t Buffer=QMC5883L_Read_Reg(QMC5883L_STATUS);
	if((Buffer&0x00)==0X00)	  {return NORMAL;}
	else if((Buffer&0x02)==0X02){return DATA_OVERFLOW;}
		return NORMAL;
}


void QMC5883L_ResetCalibration()
{
	Xmin=Xmax=Ymin=Ymax=0;
}

void QMC5883L_Scale(int16_t *X,int16_t *Y,int16_t *Z)
{
	*X*=QMC5883L_SCALE_FACTOR;
	*Y*=QMC5883L_SCALE_FACTOR;
	*Z*=QMC5883L_SCALE_FACTOR;
}



