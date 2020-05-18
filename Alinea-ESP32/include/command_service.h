/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:10
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-17 13:09:34
 * @ Description:   General command parser, check if any command string given to the main
 *                  entry point is valid and then dispatches accordingly
 *******************************************************************************************/

#ifndef _IC_COMMAND_PARSER_H
#define _IC_COMMAND_PARSER_H

#include <FreeRTOS.h>

extern QueueHandle_t xQueueCommandParse;

#define CMD_LINE_MAX_LENGTH 100
#define UART_PAYLOAD_MAX_LGTH 512 /* max command length MUST BE 100 below MAX_HDLC_FRAME_LENGTH */

typedef enum
{
    PKT_RECEIVED,
    PKT_TRANSMIT,
} commandRoute_t;

typedef enum
{
    CMD_TYPE_JSON_CMD,
    CMD_TYPE_JSON_SYN,
    CMD_TYPE_JSON_ACK,
    CMD_TYPE_JSON_TEXT,
    CMD_TYPE_JSON_DTA,
    CMD_TYPE_JSON_DTA_ATM /* atmospheric data coming from the vehicle sensors */
} command_type_t;

/* this will be sent out thru xCommandQueue, can be anything from JSON to a simple text command coming from telnet */
struct command_package_t
{
    commandRoute_t cmd_route;
    command_type_t cmd_type;
    char txtCommand[255]; /* in the case of a simple text command, we put it here, the stm will parse it as it wishes */
    uint8_t command_size;
};

//uint16_t DeviceFlag;

uint8_t uSetupCmdParser();
void cmd_parse(const char *command);
int split(const char *txt, char delim, char ***tokens);

#endif /* _IC_COMMAND_PARSER_H */
