/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 23:19:45
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-05 20:34:23
 * @ Description:
 *******************************************************************************************/

#include "ntp_service.h"
#include "configuration_esp32.h"
#include "remoteDebug_service.h"
#include <FreeRTOS.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

void vNtpServiceTask(void *parameter);
xTaskHandle xNtpServiceTaskHandle;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/**
 * @brief  Main Setup Routine
 * @note   
 * @retval None
 */
uint8_t uSetupNTPService()
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
    DEBUG_SERIAL("vAutoConnectService Task ... Creating!");
    xTaskCreate(
        vNtpServiceTask,         /* Task function. */
        "vNtpServiceTask",       /* String with name of task. */
        10000,                   /* Stack size in words. */
        NULL,                    /* Parameter passed as input of the task */
        10,                      /* Priority of the task. */
        &xNtpServiceTaskHandle); /* Task handle. */

    if (xNtpServiceTaskHandle == NULL)
    {
        DEBUG_SERIAL("vAutoConnectService Task ... Error!");
        return EXIT_FAILURE;
    }

    DEBUG_SERIAL("vAutoConnectService Task ... Success!");
    return EXIT_SUCCESS;
}

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void vNtpServiceTask(void *parameter)
{
    for (;;)
    {
        /** by defaut updated every 60 seconds but we can force an update here */
        while (!timeClient.update())
        {
            DEBUG_SERIAL("Updating NTP...");
            if (timeClient.forceUpdate())
            {
                DEBUG_SERIAL("NTP updated succesfully!");
            }
            else
            {
                debugE("Error updating NTP time!");
            }
        }

        vTaskDelay(600);
    }
    vTaskDelete(NULL);
}

/**
 * @brief  returns epoch time
 * @note   
 * @retval 
 */
uint32_t ulGetEpochTime()
{
    return timeClient.getEpochTime();
}