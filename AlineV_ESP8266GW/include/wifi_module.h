/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-23 02:37:39
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 02:46:51
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_WIFI_MODULE_H
#define _IC_WIFI_MODULE_H

#include <ESP8266WiFi.h>

#define HOST_NAME "AlineV"

extern WiFiClientSecure wiFiClient;                                             //WIFI Client

uint8_t uSetupWifi();
void connectToWifi();

#endif /* _IC_WIFI_MODULE_H */
