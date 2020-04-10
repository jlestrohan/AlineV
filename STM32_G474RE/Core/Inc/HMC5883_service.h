#ifndef INC_HMC5883_SERVICE_H_
#define INC_HMC5883_SERVICE_H_
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

uint8_t HMC5883_Initialize(I2C_HandleTypeDef *hi2cx);

#endif /* INC_HMC5883_SERVICE_H_ */
