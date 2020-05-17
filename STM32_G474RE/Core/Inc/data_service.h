/*******************************************************************
 * data_service.h
 *
 *  Created on: Apr 9, 2020
 *      Author: Jack lestrohan
 *
 *	This service collects all the data from the device, sensors etc..
 *	and forward that to the esp32serial to be sent to the esp32
 *******************************************************************/

#ifndef DATASERVICE_H_
#define DATASERVICE_H_

#include <stdint.h>
#include "configuration.h"

/* RULES */
/*
 * Every time the vehicle is making a special move (stop, avoiding, turn etc...) tghe data service sends the espService queue
 * a preformatted DTA JSON with the following datas:
 *
 * 		UUID
 * 		UUID ESP32
 * 		TimeStamp
 * 		NTP Last updated
 * 		Command Type (SPE)
 * 		Data: {
 * 			Navigation: {
 * 				Current mode (avoiding, cruise etc..)
 * 				UVLed Status (on/off)
 * 				CMPS12 Stuff... bearing, accel, gyro
 * 				Distance (sensors data)
 *
 * 				}
 * 			}
 *
 * 	When the vehicle is in movement, here is the data JSON :
 *
 * 		UUID STM32
 * 		UUID ESP32
 * 		NTP Last updated
 * 		Timestamp
 * 		Command Type (NAV)
 * 			Data: {
 * 				Current mode: (cruise)
 * 				UVLed Status: (on/off)
 * 				Navigation: {
 * 					Motors status (speed etc...)
 * 					CMP12S Stuff (bearing, set bearing, accel, gyro, mag etc...)
 *
 * 				}
 * 			}
 *
 * 	Every 30 seconds the vehicle sends the following datas
 *
 * 		UUID STM32
 * 		UUID ESP32
 * 		TimeStamp
 * 		NTP Last updated
 * 		Command Type (ATM)
 * 		Data: {
 * 			Atmospheric: {
 * 				BMP280 stuff
 * 				}
 * 			}
 *
 *	Every 30 seconds the vehicle sends the following datas:
 *
 *		UUID STM32
 * 		UUID ESP32
 *		TimeStamp
 *		NTP Last updated
 *		Command Type (SYS)
 *		Data: {
 *			All system info (kernel, versions etc etc...)
 *			Ram, heap stack on stm32, esp32 etc...)
 *		}
 */

/**
 * Main Initialization Routine
 * @return
 */
uint8_t uDataServiceinit();

#endif
