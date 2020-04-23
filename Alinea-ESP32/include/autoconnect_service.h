/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:03
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-23 12:16:26
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_AUTOCONNECT_SERVICE_H
#define _IC_AUTOCONNECT_SERVICE_H

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Alinea-ESP32";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "73727170";

void setupAutoConnect();

#endif /* _IC_AUTOCONNECT_SERVICE_H */
