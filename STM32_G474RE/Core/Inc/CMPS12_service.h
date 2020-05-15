/**
 ******************************************************************************
 * @file    CMSP12_service.h
 * @author  Jack Lestrohan
 * @brief   cmps12 magnetometer header file
 *
 * 	Pinout:		PC4 -> 	SCL
 * 				PA8 ->	SDA
 *
 *	Written from scratch from the datasheet ->
 *	http://www.robot-electronics.co.uk/files/cmps12.pdf
 *
 *	This driver ONLY includes
 ******************************************************************************
 */

#ifndef INC_CMPS12_SERVICE_H_
#define INC_CMPS12_SERVICE_H_

#include <stdint.h>
#include "cmsis_os2.h"

#define CMPS12_DEVICE_I2C_ADDRESS				0x60
#define CMPS12_DEVICE_MAGNETO_OFFSET			-8		/* offset in degrees according to the placement of the sensor, must be checked */
#define CMPS12_DEVICE_ROLLANGLE_OFFSET			-5		/* offset to Roll Angle */

/** I2C MODE REGISTER VALUES **/
#define CMPS12_REGISTER_COMMAND					0x00	/* Command register (write) / Software version (read) */
#define CMPS12_REGISTER_COMPASS_BEARING_8		0x01	/* Compass Bearing 8 bit, i.e. 0-255 for a full circle */
#define CMPS12_REGISTER_COMPASS_BEARING_16		0x02	/* Compass Bearing 16 bit, i.e. 0-3599, representing 0-359.9 degrees. register 2 being the
														 high byte. This is calculated by the processor from quaternion outputs of the BNO055 */
#define CMPS12_REGISTER_PITCH_ANGLE_8			0x04	/* Pitch angle - signed byte giving angle in degrees from the horizontal plane (+/- 90°) */
#define CMPS12_REGISTER_ROLL_ANGLE_8			0x05	/* Roll angle - signed byte giving angle in degrees from the horizontal plane (+/- 90°) */
#define CMPS12_REGISTER_MAGNETO_AXIS_X_16		0x06	/* Magnetometer X axis raw output, 16 bit signed integer (register 0x06 high byte) */
#define CMPS12_REGISTER_MAGNETO_AXIS_Y_16		0x08	/* Magnetometer Y axis raw output, 16 bit signed integer (register 0x08 high byte) */
#define CMPS12_REGISTER_MAGNETO_AXIS_Z_16		0x0A	/* Magnetometer Z axis raw output, 16 bit signed integer (register 0x0A high byte) */
#define CMPS12_REGISTER_ACCEL_AXIS_X_16			0x0C	/* Accelerometer X axis raw output, 16 bit signed integer (register 0x0C high byte) */
#define CMPS12_REGISTER_ACCEL_AXIS_Y_16			0x0E	/*  Accelerometer Y axis raw output, 16 bit signed integer (register 0x0E high byte) */
#define CMPS12_REGISTER_ACCEL_AXIS_Z_16			0x10	/* Accelerometer Z axis raw output, 16 bit signed integer (register 0x10 high byte) */
#define CMPS12_REGISTER_GYRO_AXIS_X_16			0x12	/* Gyro X axis raw output, 16 bit signed integer (register 0x12 high byte) */
#define CMPS12_REGISTER_GYRO_AXIS_Y_16			0x14	/* Gyro Y axis raw output, 16 bit signed integer (register 0x14 high byte) */
#define CMPS12_REGISTER_GYRO_AXIS_Z_16			0x16	/* Gyro Z axis raw output, 16 bit signed integer (register 0x16 high byte) */
#define CMPS12_REGISTER_SENSOR_TEMP_16			0x18	/* Temperature of the BNO055 in degrees centigrade (register 0x18 high byte) */
#define CMPS12_REGISTER_COMPASS_BEARING2_16		0x1A	/* 	Compass Bearing 16 bit This is the angle Bosch generate in the BNO055 (0-5759) divide by 16 for degrees */
#define CMPS12_REGISTER_PITCH_ANGLE_16			0x1C	/* Pitch angle 16 bit - signed byte giving angle in degrees from the horizontal plane (+/-180°) */
#define CMPS12_REGISTER_CALIBRATION_STATE_8		0x1E	/* Calibration state, bits 0 and 1 reflect the calibration status (0 un-calibrated, 3 fully calibrated) */

typedef enum {
	CMPS12_CALIB_MAGNETO_BIT_1	=	(1 << 0),
	CMPS12_CALIB_MAGNETO_BIT_2	=	(1 << 1),
	CMPS12_CALIB_ACCEL_BIT_1	=	(1 << 2),
	CMPS12_CALIB_ACCEL_BIT_2	=	(1 << 3),
	CMPS12_CALIB_GYRO_BIT_1		=	(1 << 4),
	CMPS12_CALIB_GYRO_BIT_2		=	(1 << 5),
	CMPS12_CALIB_SYSTEM_BIT_1	=	(1 << 6),
	CMPS12_CALIB_SYSTEM_BIT_2	=	(1 << 7)
} CMPS12_CalibrationBits_t;


/**
 * Main Sensor data
 */
typedef struct {
	uint8_t CalibGyro_OK;		/* set to true if ok, false otherwise */
	uint8_t CalibMagneto_OK;	/* set to true if ok, false otherwise */
	uint8_t CalibAccel_OK;		/* set to true if ok, false otherwise */
	uint16_t Temperature;		/* This is the core mcu temperature of the sensor */
	uint16_t CompassBearing;	/* 0 - 360 ready to be read as is */
	int16_t PitchAngle;
	uint8_t	RollAngle;
	int16_t GyroX;
	int16_t GyroY;
	int16_t GyroZ;
	int16_t AccelX;
	int16_t AccelY;
	int16_t AccelZ;
	int16_t	MagnetoX;
	int16_t	MagnetoY;
	int16_t	MagnetoZ;
	uint8_t Calibration_State_Flags;
} CMPS12_SensorData_t;


/****************************************************
 * Public entry point */
extern CMPS12_SensorData_t CMPS12_SensorData;
extern  osMutexId_t mCMPS12_SensorDataMutex;
/****************************************************/

uint8_t uCmps12ServiceInit();



#endif /* INC_CMPS12_SERVICE_H_ */
