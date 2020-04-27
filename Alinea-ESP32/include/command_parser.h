/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:10
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 07:43:57
 * @ Description:   General command parser, check if any command string given to the main
 *                  entry point is valid and then dispatches accordingly
 *******************************************************************************************/

#ifndef _IC_COMMAND_PARSER_H
#define _IC_COMMAND_PARSER_H

#include <FreeRTOS.h>

#define CMD_TAG_MSG_MAX_LGTH 255 /* max command length within which all comand will be considered as legit */

uint8_t setupCmdParser();
void cmd_parse(const char *command);

#endif /* _IC_COMMAND_PARSER_H */
