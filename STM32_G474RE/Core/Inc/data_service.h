/*******************************************************************
 * data_service.h
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *	This service collects all the data from the device, sensors etc..
 *	and forward that to the esp32serial to be sent to the esp32
 *******************************************************************/

#ifndef DATASERVICE_H_
#define DATASERVICE_H_

#include <stdint.h>
#include "configuration.h"

/**
 * Main Initialization Routine
 * @return
 */
uint8_t uDataServiceinit();

#endif
