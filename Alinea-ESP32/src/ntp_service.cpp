/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 23:19:45
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-22 23:29:11
 * @ Description:
 *******************************************************************************************/

#include "ntp_service.h"
#include <FreeRTOS.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

void ntpService_task(void *parameter);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/**
 * @brief  Main Setup Routine
 * @note   
 * @retval None
 */
void setupNTPService()
{
    // Initialize a NTPClient to get time
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(3600 * 2);

    /** FREERTOS NTPService Task */
    xTaskCreate(
        ntpService_task,   /* Task function. */
        "ntpService_task", /* String with name of task. */
        10000,             /* Stack size in words. */
        NULL,              /* Parameter passed as input of the task */
        10,                /* Priority of the task. */
        NULL);             /* Task handle. */
}

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void ntpService_task(void *parameter)
{
    for (;;)
    {
        /** by defaut updated every 60 seconds but we can force an update here */
        while (!timeClient.update())
        {
            timeClient.forceUpdate();
        }

        vTaskDelay(600);
    }
    vTaskDelete(NULL);
}
