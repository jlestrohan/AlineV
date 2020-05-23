/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-23 03:37:07
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 03:43:27
 * @ Description:
 *******************************************************************************************/

#include "ota_module.h"
#include <ArduinoOTA.h>
#include "wifi_module.h"

/**
 * @brief  Setup OTA main routine
 * @note   
 * @retval 
 */
uint8_t uSetupOta()
{
    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(HOST_NAME);

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
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        ESP.restart();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.println("Progress: " + (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        //Serial.print("Error[%u]: ", error);
        //Serial.println("Error[%u]: " + error);
        if (error == OTA_AUTH_ERROR)
        {
            Serial.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    
    return EXIT_SUCCESS;
}

/**
 * @brief  LOOP routine
 * @note   
 * @retval 
 */
uint8_t uLoopOTA()
{
    ArduinoOTA.handle();
     return EXIT_SUCCESS;   
}