/*******************************************************************
 * MotorsControl_service.h
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *
 *	PINOUT:
 *			MOTOR1: PWM16 -> Pin PA12 - IN1 -> PA11 - IN2 -> PA10
 *			MOTOR2: PWM17 -> Pin PA7 - IN3 -> PC10 - IN4 -> PD2
 *******************************************************************/

#ifndef INC_MOTORSCONTROL_SERVICE_H_
#define INC_MOTORSCONTROL_SERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"

#define MOTORS_DEFAULT_FW_SPEED			25
#define MOTORS_DEFAULT_WARNING_SPEED	16
#define MOTORS_DEFAULT_DANGER_SPEED		12
#define MOTORS_DEFAULT_TURN_SPEED		18
#define MOTORS_DEFAULT_BW_SPEED			18
#define MOTORS_DEFAULT_CORRECT_BIAS				4

#define MOTORS_IDLE		(1 << 0) /* if this flag is set the others are ignored */
#define MOTORS_FORWARD	(1 << 1)
#define MOTORS_BACKWARD	(1 << 2)

#ifndef bool
typedef enum {
    false,
    true
} bool;
#endif

/**
 * Motor speed
 */
typedef enum {
	MOTOR_MOTION_IDLE,
	MOTOR_MOTION_FORWARD,
	MOTOR_MOTION_BACKWARD,
	MOTOR_SPEED_REDUCE_WARNING,	/* reduce speed to warning speed */
	MOTOR_SPEED_REDUCE_DANGER,	/* reduce speed to danger speed */
	MOTOR_SPEED_NORMAL,		/* reset to normal speed */
	MOTOR_MOTION_TURN_RIGHT, /* FIXME: here we need to have MOTOR_MOTION_TURN_HEADING once BMP280 is working as intended */
	MOTOR_MOTION_TURN_LEFT,
	MOTOR_MOTION_CORRECT_RIGHT,
	MOTOR_MOTION_CORRECT_LEFT,
	MOTOR_MOTION_FORWARD_RIGHT, /* keep motion but a little to the right */
	MOTOR_MOTION_FORWARD_LEFT /* keep motion but a little to the left */
} MotorMotion_t;

/**
 * Main datastructure for motors control
 */
typedef struct {
	MotorMotion_t motorMotion_Left;		/* stop, forward, backward */
	MotorMotion_t motorMotion_Right;
	uint8_t currentSpeedLeft;				/* 0 - 100 */
	uint8_t currentSpeedRight;				/* 0 - 100 */

} MotorData_t;

/* extern variables */
extern osMessageQueueId_t xQueueMotorMotionOrder;
extern MotorData_t MotorData, lastMotorData;
extern osMutexId_t mMotorDataMutex, mLastMotorDataMutex;

/**
 * @brief  MPU6050 result enumeration
 */
typedef enum
{
	MOTORS_Result_Ok = 0x00, /*!< Everything OK */
	MOTORS_Result_Error, /*!< Unknown error */
	MOTORS_Result_ErrorHandlerNotInitialized /*!< I2C Handler not initialized (initialize() function hasn't been called ? */
} MOTORS_Result_t;

/**
 * which motor. Use bitwise if both motors
 */
typedef enum {
	MOTOR_LEFT	=	0x01U, //!< MOTOR_LEFT
	MOTOR_RIGHT =	0x02U,//!< MOTOR_RIGHT
} MotorsDef_t;


/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
uint8_t uMotorsControlServiceInit();


#endif /* INC_MOTORSCONTROL_SERVICE_H_ */

