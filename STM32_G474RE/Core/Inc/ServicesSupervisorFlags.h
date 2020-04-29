/*******************************************************************
 * ServicesSupervisorFlags.h
 *
 *  Created on: Apr 29, 2020
 *      Author: Jack Lestrohan
 *
 *	Sets of enums & structs to help build an enveloppe of msg to be
 *	queued to the supervisor task everytime that a service changes
 *	its status
 *******************************************************************/

#ifndef INC_SERVICESSUPERVISORFLAGS_H_
#define INC_SERVICESSUPERVISORFLAGS_H_

/**
 * Flags used by the general service supervisor
 */
typedef enum {
	xSERVICE_LOGGER,
	xSERVICE_LCD,
	xSERVICE_BUTTON,
	xSERVICE_HCSR04,
	xSERVICE_V53L0X,
	xSERVICE_MPU6050,
	xSERVICE_SDCARD,
	xSERVICE_MOTORS,
	xSERVICE_HCM5883,
	xSERVICE_UART,
	xSERVICE_FRONT_SERVO
} xSupervisorsServices_t;

/**
 * Service status
 */
typedef enum {
	xSERVICE_STATUS_STARTED,  //!< xSERVICE_STATUS_STARTED
	xSERVICE_STATUS_STOPPED,  //!< xSERVICE_STATUS_STOPPED
	xSERVICE_STATUS_ERROR_INIT,//!< xSERVICE_STATUS_ERROR_INIT
	xSERVICE_STATUS_ERROR_EVENT_FLAG_INIT,
	xSERVICE_STATUS_ERROR_TASK_INIT,
	xSERVICE_STATUS_ERROR_QUEUE_INIT,
	xSERVICE_STATUS_ERROR_DEVICE_NOT_READY,
	xSERVICE_STATUS_ERROR_SEM_ERROR
} xSupervisorServiceStatus_t;

typedef struct {
	xSupervisorsServices_t service;
	xSupervisorServiceStatus_t status;
	char message[200];
} xSupervisorServicesData_t;

#endif /* INC_SERVICESSUPERVISORFLAGS_H_ */
