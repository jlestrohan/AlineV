/**
 ******************************************************************************
 * @file    BMP280_service.c
 * @author  Jack Lestrohan
 * @brief   BMP280 module service file
 ******************************************************************************
 * @attention
 *				PINOUT =>
 *
 *
 *
 ******************************************************************************
 */

//https://github.com/ciastkolog/BMP280_STM32/blob/master/BMP280/bmp280.h
//https://github.com/ciastkolog/BMP280_STM32/blob/master/BMP280/bmp280.c
//https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/pressure-sensors-bmp280-1.html

#define BMP280_I2C_ADDR	0x76 << 1

#include "BMP280_service.h"
#include <stdlib.h>
#include <stdio.h>
#include "i2c.h"
#include "printf.h"

I2C_HandleTypeDef hi2c4;

/**
 * Main initialization routine
 * @return
 */
uint8_t uBmp280ServiceInit()
{
	if (HAL_I2C_IsDeviceReady(&hi2c4, BMP280_I2C_ADDR, 2, 5) != HAL_OK) {
		printf("BMP280 Device not ready");
		return (EXIT_FAILURE);
	} else {
		printf("The BMP280 device has responded normally!\n\r");
	}


	return EXIT_SUCCESS;
}

