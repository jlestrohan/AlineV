/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:22
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-23 12:17:25
 * @ Description:
 *******************************************************************************************/

#include "ota_service.h"
#include <FreeRTOS.h>
#include <ArduinoOTA.h>
#include "buzzer_service.h"
#include "remoteDebug_service.h"
#include "autoconnect_service.h"

void otaLoop_task(void *parameter);

/**
 * @brief  
 * @note   
 * @retval None
 */
void setupOTA()
{
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(thingName);

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
    debugI("Progress: %u%%\r", (progress / (total / 100)));
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
      otaLoop_task, /* Task function. */
      "OTALoop",    /* String with name of task. */
      10000,        /* Stack size in words. */
      NULL,         /* Parameter passed as input of the task */
      1,            /* Priority of the task. */
      NULL);        /* Task handle. */
}

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void otaLoop_task(void *parameter)
{
  for (;;)
  {
    ArduinoOTA.handle();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
