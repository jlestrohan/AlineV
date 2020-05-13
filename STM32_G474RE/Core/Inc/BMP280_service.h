/**
 ******************************************************************************
 * @file    BMP280_service.h
 * @author  Jack Lestrohan
 * @brief   BMP280 module service file
 ******************************************************************************
 * @attention
 * 				I2C4
 *				PINOUT => 	PC6-SCL
 *							PC7-SDA
 *
 *
 *
 ******************************************************************************
 */
#ifndef __BMP280_SERVICE_H__
#define __BMP280_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

uint8_t uBmp280ServiceInit();



#endif  // __BMP280_SERVICE_H__
