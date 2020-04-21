/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-21 23:04:58
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_OTA_H
#define _IC_OTA_H

#define USE_ARDUINO_OTA true
#define STATUS_PIN 2

#include <ArduinoOTA.h>
#include <Arduino.h>
#include <IotWebConf.h>
#include "RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Alinea-ESP32";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "73727170";

extern RemoteDebug Debug;

void setupOTA();
void otaLoop();

#endif /* _IC_OTA_H */
