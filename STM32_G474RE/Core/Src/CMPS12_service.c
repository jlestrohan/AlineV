/**
 ******************************************************************************
 * @file    CMSP12_service.c
 * @author  Jack Lestrohan
 * @brief   cmps12 magnetometer source file
 *
 * 	Pinout:		PC4 -> 	SCL
 * 				PA8 ->	SDA
 *
 *	datasheet http://www.robot-electronics.co.uk/files/cmps12.pdf
 ******************************************************************************
 */

#include "CMPS12_service.h"
#include "i2c.h"
#include "main.h"
#include "configuration.h"
#include <stdlib.h>
#include <stdio.h>
#include "printf.h"

/** main data struct */
CMPS12_SensorData_t CMPS12_SensorData;
osMutexId_t mCMPS12_SensorDataMutex;

/* functions definitions */
void vCMPS12_CalibrationStatus();
static uint8_t _read_register16(uint8_t addr, uint16_t *value);
static uint8_t _read_signed_register16(uint8_t addr, int16_t *value);
static uint8_t _read_register8(uint8_t addr, uint8_t *value);
//static uint8_t _read_data(uint8_t addr, uint8_t *value, uint8_t len);
//static uint8_t _write_register8(uint8_t addr, uint8_t value);
static uint8_t _populate_values();


I2C_HandleTypeDef hi2c2;

//osMessageQueueId_t xBMP280SensorTaskHandle;

static osThreadId_t xCMPS12SensorTaskHandle;
static osStaticThreadDef_t xCMPS12SensorTaControlBlock;
static uint32_t xCMPS12SensorTaBuffer[256];
static const osThreadAttr_t xCMPS12SensorTa_attributes = {
		.stack_mem = &xCMPS12SensorTaBuffer[0],
		.stack_size = sizeof(xCMPS12SensorTaBuffer),
		.name = "xCMPS12SensorServiceTask",
		.cb_size = sizeof(xCMPS12SensorTaControlBlock),
		.cb_mem = &xCMPS12SensorTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_CMPS12 };

/**
 * Main CMPS12 Polling task
 * @param vParameters
 */
void vCMPS12SensorTaskStart(void *vParameters)
{
	for (;;)
	{
		_populate_values();

#ifdef DEBUG_CMPS12
		MUTEX_CMPS12_TAKE
		printf("Roll:  %0*d°", 3,CMPS12_SensorData.RollAngle);
		printf("Pitch:  %0*ld°",  3, CMPS12_SensorData.PitchAngle);
		printf("gx:%0*ld, gy:%0*ld, gz:%0*ld, accx: %0*ld, accy:%0*ld, accz:%0*ld",
				3,CMPS12_SensorData.GyroX,
				3,CMPS12_SensorData.GyroY,
				3,CMPS12_SensorData.GyroZ,
				3,CMPS12_SensorData.AccelX,
				3,CMPS12_SensorData.AccelY,
				3,CMPS12_SensorData.AccelZ);
		printf("\n\r");
		MUTEX_CMPS12_GIVE
#endif

		osDelay(10);
	}
	osThreadTerminate(xCMPS12SensorTaskHandle);
}

/**
 * Main Service Initialization file
 * @return
 */
uint8_t uCmps12ServiceInit()
{
	mCMPS12_SensorDataMutex = osMutexNew(NULL);

	/* creation of CMPS12 task */
	xCMPS12SensorTaskHandle = osThreadNew(vCMPS12SensorTaskStart, NULL, &xCMPS12SensorTa_attributes);
	if (xCMPS12SensorTaskHandle == NULL) {
		printf("CMPS12 Sensor Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	if (HAL_I2C_IsDeviceReady(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, 2, 5) != HAL_OK) {
		printf("CMPS12 Device not ready\n\r");
		//Error_Handler();
		return (EXIT_FAILURE);
	} else {
		printf("** Found CMPS12 device!\n\r");
	}

	return (EXIT_SUCCESS);
}

/**
 * Refreshes the CMPS12 values struct
 * @return
 */
static uint8_t _populate_values()
{
	MUTEX_CMPS12_TAKE

	/* sensor mcu core temperature */
	_read_register16(CMPS12_REGISTER_SENSOR_TEMP_16, &CMPS12_SensorData.Temperature);

	/* compass bearing */
	_read_register16(CMPS12_REGISTER_COMPASS_BEARING2_16, &CMPS12_SensorData.CompassBearing);
	CMPS12_SensorData.CompassBearing += CMPS12_DEVICE_MAGNETO_OFFSET; /* offset adjustment */
	CMPS12_SensorData.CompassBearing = (CMPS12_SensorData.CompassBearing == 65535 ? 0 : CMPS12_SensorData.CompassBearing);
	CMPS12_SensorData.CompassBearing /= 16; /* must do to get a real bearing in degrees */

	/* pitch angle */
	_read_signed_register16(CMPS12_REGISTER_PITCH_ANGLE_16, &CMPS12_SensorData.PitchAngle);

	/* Magneto Axis */
	_read_signed_register16(CMPS12_REGISTER_MAGNETO_AXIS_X_16, &CMPS12_SensorData.MagnetoX);
	_read_signed_register16(CMPS12_REGISTER_MAGNETO_AXIS_Y_16, &CMPS12_SensorData.MagnetoY);
	_read_signed_register16(CMPS12_REGISTER_MAGNETO_AXIS_Z_16, &CMPS12_SensorData.MagnetoZ);

	/* Accel Axis */
	_read_signed_register16(CMPS12_REGISTER_ACCEL_AXIS_X_16, &CMPS12_SensorData.AccelX);
	_read_signed_register16(CMPS12_REGISTER_ACCEL_AXIS_Y_16, &CMPS12_SensorData.AccelY);
	_read_signed_register16(CMPS12_REGISTER_ACCEL_AXIS_Z_16, &CMPS12_SensorData.AccelZ);

	/* Gyro Axis */
	_read_signed_register16(CMPS12_REGISTER_GYRO_AXIS_X_16, &CMPS12_SensorData.GyroX);
	_read_signed_register16(CMPS12_REGISTER_GYRO_AXIS_Y_16, &CMPS12_SensorData.GyroY);
	_read_signed_register16(CMPS12_REGISTER_GYRO_AXIS_Z_16, &CMPS12_SensorData.GyroZ);

	/* Roll Angle */
	_read_register8(CMPS12_REGISTER_ROLL_ANGLE_8, &CMPS12_SensorData.RollAngle);
	CMPS12_SensorData.RollAngle += CMPS12_DEVICE_ROLLANGLE_OFFSET;
	CMPS12_SensorData.RollAngle = CMPS12_SensorData.RollAngle <= 180 ? CMPS12_SensorData.RollAngle :
			-(255-CMPS12_SensorData.RollAngle);

	MUTEX_CMPS12_GIVE


	return (EXIT_SUCCESS);
}

/**
 * Reads the register memory and puts the value in *value
 * @param dev
 * @param addr
 * @param value
 * @return
 */
static uint8_t _read_register8(uint8_t addr, uint8_t *value)
{
	uint8_t rx_buff[1];
	if (HAL_I2C_Mem_Read(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, addr, 1, rx_buff, 1, 5000)
			== HAL_OK) {
		*value = (uint8_t) (rx_buff[0]);
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

/**
 * Reads the register memory and puts the value in *value
 * @param dev
 * @param addr
 * @param value
 * @return
 */
static uint8_t _read_register16(uint8_t addr, uint16_t *value)
{
	uint8_t rx_buff[2];
	if (HAL_I2C_Mem_Read(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, addr, 1, rx_buff, 2, 5000)
			== HAL_OK) {
		*value = (uint16_t) ((rx_buff[0] << 8) | rx_buff[1]);
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

/**
 * Reads the register memory and puts the value in *value
 * @param dev
 * @param addr
 * @param value
 * @return
 */
static uint8_t _read_signed_register16(uint8_t addr, int16_t *value)
{
	uint8_t rx_buff[2];
	if (HAL_I2C_Mem_Read(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, addr, 1, rx_buff, 2, 5000)
			== HAL_OK) {
		*value = (int16_t) ((rx_buff[0] << 8) | rx_buff[1]);
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

/**
 * Read 8 bits data
 * @param dev
 * @param addr
 * @param value
 * @param len
 * @return
 */
/*static uint8_t _read_data(uint8_t addr, uint8_t *value, uint8_t len)
{
	if (HAL_I2C_Mem_Read(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, addr, 1, value, len, 5000) == HAL_OK) {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}*/

/**
 * Write 8 bits of data
 * @param dev
 * @param addr
 * @param value
 * @return
 */
/*static uint8_t _write_register8(uint8_t addr, uint8_t value)
{

	if (HAL_I2C_Mem_Write(&hi2c2, CMPS12_DEVICE_I2C_ADDRESS << 1, addr, 1, &value, 1, 10000) == HAL_OK)
		return EXIT_SUCCESS;

	return EXIT_FAILURE;
}*/

/**
 * Returns the sensor calibration status
 */
void vCMPS12_CalibrationStatus()
{
	//FIXME: not written yet :/
}
