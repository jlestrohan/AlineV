/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:10
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 01:12:07
 * @ Description:   General command parser, check if any command string given to the main
 *                  entry point is valid and then dispatches accordingly
 *******************************************************************************************/

#ifndef _IC_COMMAND_PARSER_H
#define _IC_COMMAND_PARSER_H

#include <FreeRTOS.h>
#include "configuration_esp32.h"

extern QueueHandle_t xQueueCommandParse;

#define UART_PAYLOAD_MAX_LGTH 512 /* max command length MUST BE 100 below MAX_HDLC_FRAME_LENGTH */

//uint16_t DeviceFlag;

uint8_t uSetupCmdParser();
void cmd_parse(const char *command);
uint8_t split(const char *txt, char delim, char ***tokens);

#endif /* _IC_COMMAND_PARSER_H */
