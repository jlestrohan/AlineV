/*******************************************************************
 * MotorsControl_service.c
 *
 *  Created on: Apr 18, 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#include "MotorsControl_service.h"

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
MOTORS_Result_t MotorsControl_Service_Initialize()
{


	return (MOTORS_Result_Ok);
}

/**
 * Accelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorAccelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange)
{

}

/**
 * Descelerate motor(s) to the target pace using motionchange rate
 * @param pace
 * @param motionchange
 */
void MotorDescelerateTo(MotorsPace_t pace, MotorsMotionChangeRate_t motionchange)
{

}

/**
 * sets motor(s) speed to the target pace using motionchange rate
 * @param pace
 */
void MotorSetSpeed(MotorsPace_t pace)
{

}

/**
 * Stop the targetted motor(s) using motionchange rate
 * @param motionchange
 */
void MotorStop(MotorsMotionChangeRate_t motionchange)
{

}

