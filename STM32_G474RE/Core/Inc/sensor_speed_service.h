/*
 * sensor_speed.h
 *
 *  Created on: Mar 20, 2020
 *      Author: Jack Lestrohan
 */

#ifndef INC_SENSOR_SPEED_SERVICE_H_
#define INC_SENSOR_SPEED_SERVICE_H_

#define EVENT_SPEED_SENSOR_1	0x0AU	/* top right wheel */
#define EVENT_SPEED_SENSOR_2	0x0BU	/* bottom right wheel */
#define EVENT_SPEED_SENSOR_3	0x0CU	/* bottom left wheel */
#define EVENT_SPEED_SENSOR_4	0x0DU	/* top left wheel */

#include "cmsis_os2.h"
#include <stdbool.h>

osEventFlagsId_t evt_speed_sensor; /* this event flag is raised everytime one of the four speed sensors gets triggered */

bool sensor_speed_initialize();

#endif /* INC_SENSOR_SPEED_SERVICE_H_ */
