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

extern osMessageQueueId_t xQueueHCSR04DataSend; /* used by IRQ handler to send out the data */

/* opaque type (encapsulation) */
struct HCSR04_Data_s;
typedef struct HCSR04_Data_s HCSR04_Data;

/**
 * Public accessors
 * @return
 */
uint16_t HCSR04_get_dist_bottom();
uint16_t HCSR04_get_dist_rear();
uint16_t HCSR04_get_dist_front();
uint16_t HCSR04_get_dist_left45();
uint16_t HCSR04_get_dist_right45();

typedef struct {
	uint8_t sensor_number;
	uint16_t distance_data;
} HR04_SensorRaw;

/********************************************************/
/** PUBLIC Structure with general service updated infos */
extern osMutexId_t mHR04_SensorsDataMutex;

//FIXME: extern HR04_SensorsData_t HR04_OldSensorsData; /* always hold the previous values on every field */
/********************************************************/

extern osEventFlagsId_t xHcrSr04ControlFlag;

/**
 * Main initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit();

#endif /* INC_HCSR04_SERVICE_H_ */
