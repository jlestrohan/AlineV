/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:26
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-17 11:34:45
 * @ Description: Serial communications handler for all serial coms
 
 *     using UART2 to STM32 =>  RXD1 -> pin G16
                                TXD1 -> pin G17 
 *******************************************************************************************/

#ifndef _IC_SERIAL_SERVICE_H
#define _IC_SERIAL_SERVICE_H

#include <stdint.h>
#include <cstddef>
#include <FreeRTOS.h>

#define MAX_HDLC_FRAME_LENGTH 256 /* this is the main frame length available */
#define MAX_JSON_MSG_SIZE 256

#define RXD2 16
#define TXD2 17

typedef struct
{
    size_t msg_size;
    uint8_t json[MAX_JSON_MSG_SIZE];
} jsonMessage_t;

uint8_t uSetupSTM32SerialService();

#endif /* _IC_SERIAL_SERVICE_H */
