/*******************************************************************
 * command_service.h
 *
 *  Created on: Apr 28, 2020
 *      Author: Jack Lestrohan
 *
 *	treats commands from and to esp32
 *******************************************************************/

#ifndef INC_COMMAND_SERVICE_H_
#define INC_COMMAND_SERVICE_H_

#define UART_PAYLOAD_MAX_LGTH 512 /* max command length MUST BE 100 below MAX_HDLC_FRAME_LENGTH */

#include "FreeRTOS.h"
#include "cmsis_os2.h"

extern osMessageQueueId_t xQueueCommandParse;

uint8_t uCmdParseServiceInit();

#endif /* INC_COMMAND_SERVICE_H_ */
