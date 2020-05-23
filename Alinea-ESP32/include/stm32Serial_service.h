/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:26
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-21 15:21:46
 * @ Description: Serial communications handler for all serial coms
 
 *     using UART2 to STM32 =>  RXD1 -> pin G16
                                TXD1 -> pin G17 
 *******************************************************************************************/

#ifndef _IC_SERIAL_SERVICE_H
#define _IC_SERIAL_SERVICE_H

#include <stdint.h>
#include <cstddef>
#include <FreeRTOS.h>
#include "configuration_esp32.h"

extern QueueHandle_t xQueueSerialServiceTX;

#define RXD2 16
#define TXD2 17

typedef struct
{
    uint8_t json[MAX_JSON_MSG_SIZE];
    size_t msg_size;
} jsonMessage_t;

uint8_t uSetupSTM32SerialService();

#endif /* _IC_SERIAL_SERVICE_H */
