/*******************************************************************
 * esp32_serial.h
 *
 *  Created on: Apr 28, 2020
 *      Author: Jack Lestrohan
 *
 *      Handles all serial communications on USART3_TX/RX pins that
 *      are dedicated to communications with the ESP32
 *      Pinout:
 *      	PB11 -> RX
 *      	PB9 -> TX
 *
 *******************************************************************/

#ifndef INC_ESP32_SERIAL_H_
#define INC_ESP32_SERIAL_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"


/**
 * Main Initialization function
 * @return
 */
uint8_t uEsp32SerialServiceInit();

uint8_t uEsp32Serial_SendMSG(const char *msg);


#endif /* INC_ESP32_SERIAL_H_ */
