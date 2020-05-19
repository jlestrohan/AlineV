/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:22
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 20:30:44
 * @ Description:
 *******************************************************************************************/

#include "ota_service.h"
#include "configuration_esp32.h"
#include <FreeRTOS.h>
#include <ArduinoOTA.h>
#include "buzzer_service.h"
#include "remoteDebug_service.h"
#include "autoconnect_service.h"

void vOtaLoopTask(void *parameter);

xTaskHandle xTaskOtaHandle;

/**
 * @brief  
 * @note   
 * @retval None
 */
uint8_t uSetupOTA()
{
    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(THINGNAME);

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        debugI("Start updating %s", type);
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        debugI("\nEnd");
        ESP.restart();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.println("Progress: " + (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        debugE("Error[%u]: ", error);
        Serial.println("Error[%u]: " + error);
        if (error == OTA_AUTH_ERROR)
        {
            debugE("Auth Failed");
            Serial.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            debugE("Begin Failed");
            Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            debugE("Connect Failed");
            Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            debugE("Receive Failed");
            Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            debugE("End Failed");
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();

    /** FREERTOS OTA Task */
    xTaskCreate(
        vOtaLoopTask,     /* Task function. */
        "vOtaLoopTask",   /* String with name of task. */
        10000,            /* Stack size in words. */
        NULL,             /* Parameter passed as input of the task */
        1,                /* Priority of the task. */
        &xTaskOtaHandle); /* Task handle. */

    if (xTaskOtaHandle == NULL)
    {
        DEBUG_SERIAL("Failed to create xTaskOtaHandle");
        return EXIT_FAILURE;
    }

    DEBUG_SERIAL("xTaskOtaHandle created succesfully and running...");
    return EXIT_SUCCESS;
}

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void vOtaLoopTask(void *parameter)
{
    for (;;)
    {
        ArduinoOTA.handle();
        //debugI("Free HEAP: %lu on HEAP TOTAL: %lu", ESP.getFreeHeap(), ESP.getHeapSize());
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}
