/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:22
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-21 22:41:25
 * @ Description:
 *******************************************************************************************/

#include <ota.h>

/**
 * @brief  
 * @note   
 * @retval None
 */
void setupOTA()
{
  Debug.begin(thingName); // Initialize the WiFi server
  ArduinoOTA.setHostname(thingName); // on donne une petit nom a notre module
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    debugI("Start updating %s", type.c_str());
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
}

/**
 * @brief  To be called on loop()
 * @note   
 * @retval 
 */
void otaLoop() 
{
  Debug.handle();
  ArduinoOTA.handle();
}