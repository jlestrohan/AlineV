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
#include "tim.h"

#define HTIM_ULTRASONIC_REAR &htim1
#define HTIM_ULTRASONIC_FRONT &htim2
#define HTIM_ULTRASONIC_BOTTOM &htim3

#define HC_SR04_SONARS_CNT	   	3 			/* Current number of sonars that are connected to the board */
#define MICROSECONDS_TO_CM 		29/2 /* have to divide µsec by this number to get distance in cm */

#define HR04_SONAR_REAR			0x01U
#define HR04_SONAR_FRONT		0x02U
#define HR04_SONAR_BOTTOM		0x03U

/*typedef enum {
	HCSR04_US_ALL_STOP,
	HCSR04_US_FRONT_ONLY,
	HCSR04_US_FRONT_BOTTOM,
	HCSR04_US_BOTTOM_ONLY,
	HCSR04_US_REAR_ONLY
} HR04_SensorsActive_t;*/

typedef struct {
	uint8_t		dist_front;
	uint8_t		dist_left45;
	uint8_t		dist_left90;
	uint8_t		dist_right45;
	uint8_t		dist_right90;
	uint8_t 	dist_bottom;
	uint8_t		dist_rear;
	//const struct HR04_SensorsData_t	*prev;	/* pointer toward the previous data */
} HR04_SensorsData_t;


/********************************************************/
/** PUBLIC Structure with general service updated infos */
extern HR04_SensorsData_t HR04_SensorsData;		/* always hold the current values on every field */
extern osMutexId_t mHR04_SensorsDataMutex;

extern HR04_SensorsData_t HR04_OldSensorsData; /* always hold the previous values on every field */
/********************************************************/

extern osEventFlagsId_t xHcrSr04ControlFlag;
osMessageQueueId_t queue_HC_SR04Handle;

/**
 * Main initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit();

#endif /* INC_HCSR04_SERVICE_H_ */
