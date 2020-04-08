/*
 * sensor_speed.h
 *
 *  Created on: Mar 20, 2020
 *      Author: Jack Lestrohan
 *
 *      CONNECT TO A0 SENSOR SIDE, DO NOT USE D0!!
 *
 *      SENSOR 1 (TOP RIGHT looking up) to PC9
 *      SENSOR 2 (BTM RIGHT looking up) to PC10
 *      SENSOR 3 (BTM LEFT  looking up) to PC11
 *      SENSOR 4 (TOP LEFT  looking up) to PC12
 *
 */
#include "cmsis_os2.h"

#ifndef INC_SENSOR_SPEED_SERVICE_H_
#define INC_SENSOR_SPEED_SERVICE_H_

#define EVENT_SPEED_SENSOR_1	0x0AU	/* top right wheel */
#define EVENT_SPEED_SENSOR_2	0x0BU	/* bottom right wheel */
#define EVENT_SPEED_SENSOR_3	0x0CU	/* bottom left wheel */
#define EVENT_SPEED_SENSOR_4	0x0DU	/* top left wheel */

osEventFlagsId_t evt_speed_sensor; /* this event flag is raised everytime one of the four speed sensors gets triggered @suppress("Avoid global variables ") */

uint8_t sensor_speed_initialize();

#endif /* INC_SENSOR_SPEED_SERVICE_H_ */
