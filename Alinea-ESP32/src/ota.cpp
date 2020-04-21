/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 00:30:22
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-21 23:11:06
 * @ Description:
 *******************************************************************************************/

#include <ota.h>
#include "buzmusic.h"

void handleRoot();
// -- Callback method declarations.
void wifiConnected_cb();

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

/**
 * @brief  
 * @note   
 * @retval None
 */
void setupOTA()
{
  setupBuzzer();
  // -- Initializing the configuration.
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setWifiConnectionCallback(&wifiConnected_cb);
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound([]() { iotWebConf.handleNotFound(); });

  Debug.begin(thingName);            // Initialize the WiFi server
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
  iotWebConf.doLoop();
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 01 Minimal</title></head><body>Hello world!";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

/**
 * @brief  WiFio connected callback
 * @note   
 * @retval None
 */
void wifiConnected_cb()
{
  debugI("WiFi connected.");
  digitalWrite(STATUS_PIN, LOW); /* to avoid stuck lit led */ 
  wifiSuccessBuz();
}