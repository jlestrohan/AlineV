/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:03
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 23:46:14
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_AUTOCONNECT_SERVICE_H
#define _IC_AUTOCONNECT_SERVICE_H

#include "configuration_esp32.h"
#include <stdint.h>
#include "configuration_esp32.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "73727170";

uint8_t uSetupAutoConnect();

#endif /* _IC_AUTOCONNECT_SERVICE_H */
