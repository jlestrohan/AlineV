/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-21 01:20:36
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_OTA_H
#define _IC_OTA_H

#include <ArduinoOTA.h>
#include "RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Aline-ESP32";

extern RemoteDebug Debug;

void setupOTA();

#endif /* _IC_OTA_H */
