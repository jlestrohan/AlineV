/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-05 20:56:14
 * @ Description: https://hieromon.github.io/AutoConnect/otaupdate.html
 * //https://hieromon.github.io/AutoConnect/howtoembed.html
 *******************************************************************************************/

#include "autoconnect_service.h"
#include "remoteDebug_service.h"
#include "configuration_esp32.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include "buzzer_service.h"

WebServer Server;
AutoConnect Portal(Server);

xTaskHandle xAutoConnectServiceTaskHandle = NULL;
void vAutoConnectServiceTask(void *parameter);

void rootPage()
{
  char content[] = "Hello World";
  Server.send(200, "text/plain", content);
}

/**
 * @brief  Main Autoconnect setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupAutoConnect()
{
  Server.on("/", rootPage);

  if (Portal.begin())
  {
    Serial.println();
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    Serial.println("Web server started");
    wifiSuccessTune();
    vPlayMelody(MelodyType_WifiSuccess);
  }
  /** FREERTOS AutoConnect Task */
  DEBUG_SERIAL("vAutoConnectService Task ... Creating");
  xTaskCreate(
      vAutoConnectServiceTask,         /* Task function. */
      "xAutoConnectServiceTask",       /* String with name of task. */
      10000,                           /* Stack size in words. */
      NULL,                            /* Parameter passed as input of the task */
      1,                               /* Priority of the task. */
      &xAutoConnectServiceTaskHandle); /* Task handle. */

  if (xAutoConnectServiceTaskHandle == NULL)
  {
    DEBUG_SERIAL("vAutoConnectService Task ... Error!");
    return EXIT_FAILURE;
  }

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void vAutoConnectServiceTask(void *parameter)
{
  for (;;)
  {
    Portal.handleClient();
    vTaskDelay(10);
  }
  vTaskDelete(xAutoConnectServiceTaskHandle);
}
