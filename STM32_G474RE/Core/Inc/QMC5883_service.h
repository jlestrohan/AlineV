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

#define QMC5883l_I2C_ADDRESS 		0x0DU << 1 /* device I2C Address */

#define QMC5883L_DATA_READ_X_LSB	0x00
#define QMC5883L_DATA_READ_X_MSB	0x01
#define QMC5883L_DATA_READ_Y_LSB	0x02
#define QMC5883L_DATA_READ_Y_MSB	0x03
#define QMC5883L_DATA_READ_Z_LSB	0x04
#define QMC5883L_DATA_READ_Z_MSB	0x05
#define QMC5883L_TEMP_READ_LSB		0x07
#define QMC5883L_TEMP_READ_MSB		0x08
#define QMC5883L_STATUS		        0x06 // DOR | OVL | DRDY
#define QMC5883L_CONFIG_1			0x09 // OSR | RNG | ODR | MODE
#define QMC5883L_CONFIG_2			0x0A // SOFT_RST | ROL_PNT | INT_ENB
#define QMC5883_CONFIG_RESET		0x0B // SET/RESET Period FBR [7:0]
#define QMC5883_CONFIG_RESERVED		0x0C

#define QMC5883L_SCALE_FACTOR 			0.732421875f
#define QMC5883L_CONVERT_GAUSS_2G 		12000.0f
#define QMC5883L_CONVERT_GAUSS_8G 		3000.0f
#define QMC5883L_CONVERT_MICROTESLA 	100
#define QMC5883L_DECLINATION_ANGLE		17.45/1000  // radian, Toulouse/France

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288f
#endif

typedef enum OSR_VARIABLES
{
	OVER_SAMPLE_RATIO_512 = 0b00000000,
	OVER_SAMPLE_RATIO_256 = 0b01000000,
	OVER_SAMPLE_RATIO_128 = 0b10000000,
	OVER_SAMPLE_RATIO_64  = 0b11000000
}_qmc5883l_OSR;

typedef enum RNG_VARIABLES
{
	FULL_SCALE_2G = 0b00000000,
	FULL_SCALE_8G = 0b00010000
}_qmc5883l_RNG;

typedef enum ODR_VARIABLES
{
	OUTPUT_DATA_RATE_10HZ = 0b00000000,
	OUTPUT_DATA_RATE_50HZ = 0b00000100,
	OUTPUT_DATA_RATE_100HZ = 0b00001000,
	OUTPUT_DATA_RATE_200HZ = 0b00001100
}_qmc5883l_ODR;

typedef enum MODE_VARIABLES
{
	MODE_CONTROL_STANDBY = 0b00000000,
	MODE_CONTROL_CONTINUOUS = 0b00000001
}_qmc5883l_MODE;

typedef enum STATUS_VARIABLES
{
	NORMAL,
	NO_NEW_DATA,
	NEW_DATA_IS_READY,
	DATA_OVERFLOW,
	DATA_SKIPPED_FOR_READING
}_qmc5883l_status;

typedef enum INTTERRUPT_VARIABLES
{
	INTERRUPT_DISABLE,
	INTERRUPT_ENABLE
}_qmc5883l_INT;

uint8_t uQmc5883lServiceInit(I2C_HandleTypeDef *hi2cx);
void QMC5883L_Configure(_qmc5883l_MODE MODE, _qmc5883l_ODR ODR, _qmc5883l_RNG RNGE, _qmc5883l_OSR OSR);
void QMC5883L_Write_Reg(uint8_t reg, uint8_t data);
uint8_t QMC5883L_Heading(int16_t Xraw,int16_t Yraw);
int16_t QMC5883L_Read_Temperature();
uint8_t QMC5883L_Read_Reg(uint8_t reg);
void QMC5883L_Read_Data(int16_t *MagX,int16_t *MagY,int16_t *MagZ);
void QMC5883L_Reset();

#endif /* INC_QMC5883_SERVICE_H_ */
