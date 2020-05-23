/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-21 20:40:46
 * @ Description: 
 
 *******************************************************************************************/

#include <WiFiClientSecure.h>
#include "autoconnect_service.h"
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "stm32Serial_service.h"
#include <WiFi.h>
#include <WebServer.h>
#include "AWS_Certificate.h"
#include <PubSubClient.h>
#include <AutoConnect.h>

#define MQTT_MAX_PACKET_SIZE 256

/* functions definitions */
void AWS_Connect();

static xTaskHandle xAWS_Send_Task_hnd = NULL;

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig acConfig;
WiFiClientSecure wifiClient;
PubSubClient pubSubClient(AWS_IOT_ENDPOINT, 8883, NULL, wifiClient);

static xTaskHandle xAutoConnectServiceTaskHandle = NULL;
void vAutoConnectServiceTask(void *parameter);

void rootPage()
{
  char content[] = "Hello World";
  Server.send(200, "text/plain", content);
}

/* -------------------------------- AWS SEND -------------------------------- */
/**
 * @brief  AWS SEND task. 
 *          This task prepares and encapsulates datas to send AWS to AWS. 
 *         
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
static void vAWSSendTaskCode(void *vParameter)
{
  jsonMessage_t aws_ReceiveBuf;

  for (;;)
  {
    xQueueReceive(xQueueAWS_Send, &aws_ReceiveBuf, portMAX_DELAY);
    if (pubSubClient.connected())
    {
      debugI("%s", (char *)aws_ReceiveBuf.json);
      if (!pubSubClient.publish(AWS_IOT_TOPIC, (const char *)aws_ReceiveBuf.json))
      {
        debugE("Error publishing MQTT topic");
      }
    }

    vTaskDelay(20);
  }
  vTaskDelete(NULL);
}

/* ------------------------------- MAIN SETUP ------------------------------- */
/**
 * @brief  Main Autoconnect setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupAutoConnect()
{

  /* AWS SEND QUEUE */
  xQueueAWS_Send = xQueueCreate(5, sizeof(jsonMessage_t));
  if (xQueueAWS_Send == NULL)
  {
    DEBUG_SERIAL("error creating the xQueueAWS_Send queue");

    return EXIT_FAILURE;
  }

  xTaskCreate(
      vAWSSendTaskCode,         /* Task function. */
      "vCommandParserTaskCode", /* String with name of task. */
      16384,                    /* Stack size in words. */
      NULL,                     /* Parameter passed as input of the task */
      18,                       /* Priority of the task. */
      &xAWS_Send_Task_hnd);     /* Task handle. */

  Server.on("/", rootPage);

  /* change the AP label */
  //acConfig.apid = "AlineV-" + ESP.getEfuseMac();
  // Portal.config(acConfig);

  /* Configure WiFiClientSecure to use the AWS certificates we generated */

  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);

  if (Portal.begin())
  {
    vPlayMelody(MelodyType_WifiSuccess);
    debugI("--- WiFi connected: %s ---", WiFi.localIP().toString());
    Serial.println("--- WiFi connected: " + WiFi.localIP().toString() + " ---");
    DEBUG_SERIAL("Web server started");
  }

  /** FREERTOS AutoConnect Task */
  DEBUG_SERIAL("vAutoConnectService Task ... Creating");
  xTaskCreate(
      vAutoConnectServiceTask,         /* Task function. */
      "xAutoConnectServiceTask",       /* String with name of task. */
      16384,                           /* Stack size in words. */
      NULL,                            /* Parameter passed as input of the task */
      12,                              /* Priority of the task. */
      &xAutoConnectServiceTaskHandle); /* Task handle. */

  if (xAutoConnectServiceTaskHandle == NULL)
  {
    DEBUG_SERIAL("vAutoConnectService Task ... Error!");
    return EXIT_FAILURE;
  }

  DEBUG_SERIAL("vAutoConnectService Task ... Success");
  return EXIT_SUCCESS;
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

    if (!pubSubClient.connected())
    {

      if (pubSubClient.connect(DEVICE_NAME))
      {
        Serial.println("MQTT Connected...");
        // Seria("MQTT Connected to %s", AWS_IOT_ENDPOINT);
        vPlayMelody(MelodyType_CommandReady);
      }
    }
    pubSubClient.loop();
    //debugI("Free HEAP: %lu on HEAP TOTAL: %lu", ESP.getFreeHeap(), ESP.getHeapSize());
    vTaskDelay(5);
  }
  vTaskDelete(NULL);
}
