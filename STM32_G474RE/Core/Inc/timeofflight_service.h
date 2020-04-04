/*
 * timeofflight_service.h
 *
 *  Created on: Apr 3, 2020
 *      Author: aez03
 */

#ifndef INC_TIMEOFFLIGHT_SERVICE_H_
#define INC_TIMEOFFLIGHT_SERVICE_H_

#include "i2c.h"

/**
 * @param current I2C Handler
 */
uint8_t timeofflight_initialize(I2C_HandleTypeDef *hi2cx);

/**
 * Set Reset (XSDN) state of a given "id" device
 * @param  DevNo The device number use  @ref XNUCLEO53L0A1_dev_e. Char 't' 'c' 'r' can also be used
 * @param  state  State of the device reset (xsdn) pin @warning reset  pin is active low
 * @return 0 on success
 */
uint8_t ST53L0A1_ResetId(uint8_t DevNo,  uint8_t state );

#endif /* INC_TIMEOFFLIGHT_SERVICE_H_ */
