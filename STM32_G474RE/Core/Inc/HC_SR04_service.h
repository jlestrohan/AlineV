#ifndef INC_HC_SR04_SERVICE_H_
#define INC_HC_SR04_SERVICE_H_
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

#define HR04_SONAR_1	0x01U
#define HR04_SONAR_2	0x02U
#define HR04_SONAR_3	0x03U

osMessageQueueId_t queue_HC_SR04Handle;

typedef struct {
	uint8_t		HR04_1_Distance;
	uint8_t		HR04_2_Distance;
	uint8_t		HR04_3_Distance;
} HR04_SensorsData_t;


/**
 * Main initialization function
 * @return
 */
uint8_t HC_SR04_initialize();

#endif /* INC_HC_SR04_SERVICE_H_ */
