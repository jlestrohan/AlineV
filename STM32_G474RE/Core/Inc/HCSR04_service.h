#ifndef INC_HCSR04_SERVICE_H_
#define INC_HCSR04_SERVICE_H_
/**
 * *********************************************************************************
 * sensor_hr04_service.h
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 * *********************************************************************************
 */

#include <stdint.h>
#include "cmsis_os2.h"

#define HC_SR04_SONARS_CNT	   	3 			/* Current number of sonars that are connected to the board */
#define MICROSECONDS_TO_CM 		29/2 /* have to divide µsec by this number to get distance in cm */

#define HR04_SONAR_REAR			0x01U
#define HR04_SONAR_FRONT		0x02U
#define HR04_SONAR_BOTTOM		0x03U

/* event flag to activate the sensors */
#define FLG_SONAR_REAR_ACTIVE		(1 << 0)
#define FLG_SONAR_FRONT_ACTIVE		(1 << 1)
#define FLG_SONAR_BOTTOM_ACTIVE		(1 << 2)

typedef struct {
	uint8_t		distance;
	uint8_t		sonarNum;
} HR04_SensorsData_t;

extern HR04_SensorsData_t HR04_SensorsData;
extern osEventFlagsId_t xHcrSr04ControlFlag;
extern osMessageQueueId_t queue_HC_SR04Handle;

/**
 * Main initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit();

#endif /* INC_HCSR04_SERVICE_H_ */
