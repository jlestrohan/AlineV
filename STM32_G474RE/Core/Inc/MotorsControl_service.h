/*******************************************************************
 * MotorsControl_service.h
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#ifndef INC_MOTORSCONTROL_SERVICE_H_
#define INC_MOTORSCONTROL_SERVICE_H_

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
 * Motor speed
 */
typedef enum {
	MOTOR_AHEAD_FULL,
	MOTOR_AHEAD_HALF,
	MOTOR_AHEAD_SLOW,
	MOTOR_STOP,
	MOTOR_ASTERN_SLOW,
	MOTOR_ASTERN_HALF,
	MOTOR_ASTERN_FULL,
} MotorsPace_t;

/**
 * Acceleration/Desceleration Rate
 */
typedef enum {
	MOTOR_MOTIONCHANGE_VERY_SLOW,//!< MOTOR_ACCEL_VERY_SLOW
	MOTOR_MOTIONCHANGE_SLOW,     //!< MOTOR_ACCEL_SLOW
	MOTOR_MOTIONCHANGE_NORMAL,   //!< MOTOR_ACCEL_NORMAL
	MOTOR_MOTIONCHANGE_FAST,     //!< MOTOR_ACCEL_FAST
	MOTOR_MOTIONCHANGE_VERY_FAST,//!< MOTOR_ACCEL_VERY_FAST
	MOTOR_MOTIONCHANGE_SLOWER,   //!< MOTOR_ACCEL_SLOWER
	MOTOR_MOTIONCHANGE_FASTER,    //!< MOTOR_ACCEL_FASTER
	MOTOR_MOTIONCHANGE_IMMEDIATE
} MotorsMotionChangeRate_t;

/**
 * Main datastructure for motors control
 */
typedef struct {
	MotorsDef_t motorNumber;
	MotorsPace_t currentSetPace;
	uint8_t sensorDirection;
	uint8_t sensorSpeed;
} MotorData_t;

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
MOTORS_Result_t MotorsControl_Service_Initialize();

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange);

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange);

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param pace
 */
void MotorSetSpeed(MotorsPace_t pace);

/**
 * Stop the targetted motor(s) using motionchange rate
 * @param motionchange
 */
void MotorStop(MotorsMotionChangeRate_t motionchange);



#endif /* INC_MOTORSCONTROL_SERVICE_H_ */

