/*******************************************************************
 * ALINEA_CONFIG.h
 *
 *  Created on: 26 avr. 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#ifndef INC_ALINEA_CONFIG_H_
#define INC_ALINEA_CONFIG_H_

/**
 * BEHAVIOUR CONTROL
 */
 /* on a forward motion, the rover will stop if the US sensor goes above the defined distance */
#define ULTRASONIC_HOLE_MIN_DISTANCE_STOP_CM			10
/* on a backward motion distance under which the rover will stop going backward */
#define ULTRASONIC_BACK_SENSOR_STOP_REAR_MOTION_CM		15



#endif /* INC_ALINEA_CONFIG_H_ */
