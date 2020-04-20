/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-20 23:50:09
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include <IotWebConf.h>
#include "ota.h"
#include "RemoteDebug.h" //https://github.com/JoaoLopesF/RemoteDebug

#define USE_ARDUINO_OTA true
#define STATUS_PIN LED_BUILTIN
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "73727170";

DNSServer dnsServer;
WebServer server(80);

RemoteDebug Debug;
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

void handleRoot();

/**
 * @brief  
 * @note   
 * @retval None
 */
void setupOTA()
{
  ArduinoOTA.setHostname(thingName); // on donne une petit nom a notre module
  //ArduinoOTA.setPassword("sGF3Rbd9ix9oD");
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

void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:
  // -- Initializing the configuration.
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound([]() { iotWebConf.handleNotFound(); });

  setupOTA();

  Serial.println("Ready.");
}

void loop()
{
  // put your main code here, to run repeatedly:
  iotWebConf.doLoop();
  ArduinoOTA.handle();

  if ((millis() - mLastTime) >= 1000)
  {
    // Time
    mLastTime = millis();
    mTimeSeconds++;

    // Debug the time (verbose level)
    debugV("* Time: %u seconds (VERBOSE)", mTimeSeconds);
    if (mTimeSeconds % 5 == 0)
    { // Each 5 seconds
      // Debug levels
      debugV("* This is a message of debug level VERBOSE");
      debugD("* This is a message of debug level DEBUG");
      debugI("* This is a message of debug level INFO");
      debugW("* This is a message of debug level WARNING");
      debugE("* This is a message of debug level ERROR");
    }
  }

  // RemoteDebug handle

  Debug.handle();

  // Give a time for ESP
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