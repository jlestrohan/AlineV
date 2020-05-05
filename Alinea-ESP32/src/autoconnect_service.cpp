/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-02 16:30:10
 * @ Description: https://hieromon.github.io/AutoConnect/otaupdate.html
 * //https://hieromon.github.io/AutoConnect/howtoembed.html
 *******************************************************************************************/

#include "autoconnect_service.h"
#include "configuration_esp32.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include "buzzer_service.h"

WebServer Server;
AutoConnect Portal(Server);

xTaskHandle xAutoConnectTask_hnd = NULL;
void autoConnectLoop_task(void *parameter);

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
void setupAutoConnect()
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
  xTaskCreate(
      autoConnectLoop_task,   /* Task function. */
      "autoConnectLoop_task", /* String with name of task. */
      10000,                  /* Stack size in words. */
      NULL,                   /* Parameter passed as input of the task */
      1,                      /* Priority of the task. */
      &xAutoConnectTask_hnd); /* Task handle. */
}

/**
 * @brief  OTA Task, handles OTA + Webserver + Debug routines
 * @note   
 * @retval 
 */
void autoConnectLoop_task(void *parameter)
{
  for (;;)
  {
    Portal.handleClient();
    vTaskDelay(10);
  }
  vTaskDelete(xAutoConnectTask_hnd);
}
