/*
 * sensor_hr04_service.h
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *      https://controllerstech.com/hc-sr04-ultrasonic-sensor-and-stm32/
 */
#include <stdint.h>
#include "cmsis_os2.h"

#ifndef INC_SENSOR_HR04_SERVICE_H_
#define INC_SENSOR_HR04_SERVICE_H_

//#define HR04_SENSORS_NUMBERS		2		/* how many hr04 sensors in the house ? don't forget this or
//be prepared for unexpected results*/
#define EVENT_HR04_ECHO_SENSOR_1	0xA1U 	/* flags returned everytime one of both hr04 sensors echo is returned */
#define EVENT_HR04_ECHO_SENSOR_2	0xA2U

osEventFlagsId_t evt_hr04_sensor;

uint8_t sensor_HR04_initialize();

#endif /* INC_SENSOR_HR04_SERVICE_H_ */
