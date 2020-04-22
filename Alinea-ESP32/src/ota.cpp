/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:22
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-22 17:17:29
 * @ Description:
 *******************************************************************************************/

#include <ota.h>
#include <FreeRTOS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "buzmusic.h"


void otaLoop_task(void *parameter);

/**
 * @brief  
 * @note   
 * @retval None
 */
void setupOTA()
{
  setupBuzzer();

  Debug.begin(thingName); // Initialize the WiFi server

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
  });
  ArduinoOTA.onEnd([]() {
    debugI("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debugI("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debugE("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      debugE("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      debugE("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      debugE("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      debugE("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      debugE("End Failed");
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
    Debug.handle();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
