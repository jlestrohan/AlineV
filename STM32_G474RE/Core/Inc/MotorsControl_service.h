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

#define MOTORS_FORWARD_ACTIVE	(1 << 0)

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
	MotorMotion_Stop,
	MotorMotion_Forward,
	MotorMotion_Backward,
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

extern osEventFlagsId_t xEventMotorsForward;
/**
 * Main DataStruct accessible from everywhere
 */
extern MotorData_t MotorData;

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
 * Acceleration/Desceleration Rate
 */
typedef enum {
	TransitionPace_Very_Slow,
	TransitionPace_Below_Slow,
	TransitionPace_Slow,
	TransitionPace_Above_Slow,
	TransitionPace_Normal,
	TransitionPace_Below_Fast,
	TransitionPace_Fast,
	TransitionPace_Above_Fast,
	TransitionPace_Very_Fast,
	TransitionPace_Immediate
} MotorsMotionChangeRate_t;

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
uint8_t uMotorsControlServiceInit();

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionPace, uint8_t targetSpeed);

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorData_t *data, MotorsMotionChangeRate_t motionPace, uint8_t targetSpeed);

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param pace
 */
void MotorSetSpeed(MotorData_t *data, uint8_t speed_left, uint8_t speed_right);


/**
 * Sets LN298 GPIO controls for forward motion
 */
void motorSetMotionForward(MotorData_t *data);

/**
 * Sets LN298 GPIO controls for backward motion
 */
void motorSetMotionBackward(MotorData_t *data);

/**
 * Sets LN298 GPIO controls for left turn motion
 * @param data
 */
void motorSetMotionTurnLeft(MotorData_t *data);

/**
 * Sets LN298 GPIO controls for right turn motion
 * @param data
 */
void motorSetMotionTurnRight(MotorData_t *data);

/**
 * Sets motors power to 0 but does not change the recorded motion
 */
void motorsSetMotorsIdle(MotorData_t *data);

/**
 * Completely stops the motors
 */
void motorsSetMotionStop(MotorData_t *data);


#endif /* INC_MOTORSCONTROL_SERVICE_H_ */

