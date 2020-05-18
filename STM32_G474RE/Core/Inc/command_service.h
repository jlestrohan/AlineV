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
#define ESP32_UUID
#define CMD_LINE_MAX_LENGTH 100

#include "FreeRTOS.h"
#include "cmsis_os2.h"

typedef enum
{
    CMD_TYPE_JSON_CMD,
    CMD_TYPE_JSON_SYN,
    CMD_TYPE_JSON_ACK,
    CMD_TYPE_JSON_TEXT,
    CMD_TYPE_JSON_DTA,
	CMD_TYPE_JSON_SPE, /* see data_service.h for explanations */
	CMD_TYPE_JSON_NAV,
	CMD_TYPE_JSON_ATM,
	CMD_TYPE_JSON_SYS,

} command_type_t;

extern osMessageQueueId_t xQueueCommandParse;

uint8_t uCmdParseServiceInit();

#endif /* INC_COMMAND_SERVICE_H_ */
