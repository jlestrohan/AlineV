/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:26
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-28 22:18:57
 * @ Description: Serial communications handler for all serial coms
 
 *     using UART2 to STM32 =>  RXD1 -> pin G16
                                TXD1 -> pin G17 
 *******************************************************************************************/

#ifndef _IC_SERIAL_SERVICE_H
#define _IC_SERIAL_SERVICE_H

#include <stdint.h>

#define RXD2 16
#define TXD2 17

uint8_t setupUARTListener();

#endif /* _IC_SERIAL_SERVICE_H */
