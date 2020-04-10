#ifndef INC_QMC5883_SERVICE_H_
#define INC_QMC5883_SERVICE_H_
/*******************************************************************
 * HC5883_service.h
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack Lestrohan
 *      see https://github.com/jrowberg/i2cdevlib/tree/master/STM32/HMC5883
 *
 *******************************************************************/
#include <stdint.h>
#include "i2c.h"
#include <stdbool.h>

#define QMC5883l_ADDRESS 			0x0DU << 1 /* device I2C Address */

/* Control Register 1 */
/**********************************************************************************************/
#define QMC5883l_CNTRL_REG_1_ADD	0x09U
#define QMC5883l_MODE_STANDBY    	0b00000000
#define QMC5883l_MODE_CONTINUOUS 	0b00000001

#define QMC5883l_ODR_10Hz        	0b00000000
#define QMC5883l_ODR_50Hz        	0b00000100
#define QMC5883l_ODR_100Hz       	0b00001000
#define QMC5883l_ODR_200Hz       	0b00001100

#define QMC5883l_RNG_2G          	0b00000000
#define QMC5883l_RNG_8G          	0b00010000

#define QMC5883l_OSR_512         	0b00000000
#define QMC5883l_OSR_256         	0b01000000
#define QMC5883l_OSR_128         	0b10000000
#define QMC5883l_OSR_64          	0b11000000

/* Control Register 2 */
/**********************************************************************************************/
#define QMC5883l_CNTRL_REG_2_ADD	0x0AH

/*Interrupt enabling is controlled by register INT_ENB in control register 2. Once the interrupt is
enabled, it will flag when new data is in Data Output Registers.
INT_ENB: “0”: enable interrupt PIN, “1”: disable interrupt PIN*/
#define QCM5883l_INT_ENB			(1 << 0)

/* Pointer roll-over function is controlled by ROL_PNT register. When the point roll-over function is
enabled, the I2C data pointer automatically rolls between 00H ~ 06H, if I²C read begins at any
address among 00H~06H.
ROL_PNT: “0”: Normal, “1”: En ROL_PNT: “0”: Normal, “1”: Enable pointer roll-over function */
#define QCM5883l_ROL_PNT			(1 << 6)

/* Soft Reset can be done by changing the register SOFT_RST to set. Soft reset can be invoked at
any time of any mode. For example, if soft reset occurs at the middle of continuous mode reading,
QMC5883L immediately switches to standby mode due to mode register is reset to “00” in default.
SOFT_RST: “0”: Normal “1”: Soft reset, restore default value of all registers.*/
#define QCM5883l_ROL_PNT			(1 << 7)

/* Output data registers */
/**********************************************************************************************/
/* Registers 00H <-> 05H store the measurement data from each axis in cont measurement */
#define QMC5883l_DATA_OUTPUT_X_LSB_REG	0x00U	/* Data Output X LSB Register XOUT[7:0]  */
#define QMC5883l_DATA_OUTPUT_X_MSB_REG	0x01U	/* Data Output X MSB Register XOUT[15:8] */
#define QMC5883l_DATA_OUTPUT_Y_LSB_REG	0x02U	/* Data Output Y LSB Register YOUT[7:0]  */
#define QMC5883l_DATA_OUTPUT_Y_MSB_REG	0x03U	/* Data Output Y MSB Register YOUT[15:8] */
#define QMC5883l_DATA_OUTPUT_Z_LSB_REG	0x04U	/* Data Output Z LSB Register ZOUT[7:0]  */
#define QMC5883l_DATA_OUTPUT_Z_MSB_REG	0x05U	/* Data Output Z MSB Register ZOUT[15:8] */


/* status registers */
/**********************************************************************************************/
#define QMC5883l_STATUS_REG_ADD			0x06U

/* Data Ready Register (DRDY), it is set when all three axis data is ready, and loaded to the output
data registers in the continuous measurement mode. It is reset to “0” by reading any data register
(00H~05H) through I²C commends
DRDY: “0”: no new data, “1”: new data is ready */
#define QMC5883l_DRDY_STATUS_REG		(1 << 0)

/* Overflow flag (OVL) is set to “1” if any data of three axis magnetic sensor channels is out of range.
The output data of each axis saturates at -32768 and 32767, if any of the axis exceeds this range,
OVL flag is set to “1”. This flag is reset to “0” if next measurement goes back to the range of
(-32768, 32767), otherwise, it keeps as “1”.
OVL: “0”: normal, “1”: data overflow */
#define QMC5883l_OVL_STATUS_REG			(1 << 1)

/* Data Skip (DOR) bit is set to “1” if all the channels of output data registers are skipped in reading in D
the continuous-measurement mode. It is reset to “0” by reading any data register (00H~05H)
through I²C.
DOR: “0”: normal, “1”: data skipped for reading */
#define QMC5883l_DOR_STATUS_REG			(1 << 2)


/* 12.2.3 Temperature Data Registers */
/**********************************************************************************************/
/* Registers 07H-08H store temperature sensor output data. 16 bits temperature sensor output is in Registers
2’s complement. Temperature sensor gain is factory-calibrated, but its offset has not been compensated, only relative temperature value is accurate.
The temperature coefficient is about 100 LSB/°C */
#define QMC5883l_TEMP_OUTPUT_LSB_REG	0x07U
#define QMC5883l_TEMP_OUTPUT_MSB_REG	0x08U

/* 12.2.5 SET/RESET Period Register */
/* SET/RESET Period is controlled by FBR [7:0], it is recommended that the register 0BH is written by 0x01.*/
#define QMC5883l_SET_RESET_PERIOD_REG	0x0BU

/**
 * @brief  QMC5883 result enumeration
 */
typedef enum
{
	QHM5883_Result_Ok = 0x00, /*!< Everything OK */
	QHM5883_Result_Error, /*!< Unknown error */
	QHM5883_Result_DeviceNotConnected, /*!< There is no device with valid slave address */
	QHM5883_Result_DeviceInvalid, /*!< Connected device with address is not MPU6050 */
	QHM5883_Result_ErrorHandlerNotInitialized /*!< I2C Handler not initialized (initialize() function hasn't been called ? */
} QHM5883_Result;

uint8_t HMC5883_Initialize(I2C_HandleTypeDef *hi2cx);

#endif /* INC_QMC5883_SERVICE_H_ */
