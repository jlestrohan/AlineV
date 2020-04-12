#ifndef INC_SENSOR_HR04_SERVICE_H_
#define INC_SENSOR_HR04_SERVICE_H_
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

#define MAX_SONAR   3 			/* Current number of sonars that are connected to the board */

#define HR04_SONAR_1	0x01U
#define HR04_SONAR_2	0x02U
#define HR04_SONAR_3	0x03U

osMessageQueueId_t queue_icValueHandle;

typedef struct {
	uint8_t		sonar_number;
	uint32_t 	echo_capture_S1;
	uint32_t 	echo_capture_S2;
	uint8_t		HR04_1_Distance;
	uint8_t		HR04_2_Distance;
} HR04_SensorsData_t;
HR04_SensorsData_t HR04_SensorsData;

/**
 * Main initialization function
 * @return
 */
uint8_t sensor_HR04_initialize();

#endif /* INC_SENSOR_HR04_SERVICE_H_ */
