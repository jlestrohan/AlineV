/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 21:02:17
 * @ Description: 
 
 *******************************************************************************************/

#include "autoconnect_service.h"
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include "AWS_Certificate.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>

/* functions definitions */
void vMqttConnect();

static xTaskHandle xAWS_Send_Task_hnd = NULL;

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig acConfig;
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(1024);

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
  aws_rdy_data_t aws_ReceiveBuf;

  for (;;)
  {
    xQueueReceive(xQueueAWS_Send, &aws_ReceiveBuf, portMAX_DELAY);
    debugI(" %s", (char *)aws_ReceiveBuf.jsonstr);
    //client.publish(AWS_IOT_TOPIC, aws_ReceiveBuf.jsonstr);

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
  xQueueAWS_Send = xQueueCreate(3, sizeof(aws_rdy_data_t));
  if (xQueueAWS_Send == NULL)
  {
    DEBUG_SERIAL("error creating the xQueueAWS_Send queue");

    return EXIT_FAILURE;
  }

  xTaskCreate(
      vAWSSendTaskCode,         /* Task function. */
      "vCommandParserTaskCode", /* String with name of task. */
      10000,                    /* Stack size in words. */
      NULL,                     /* Parameter passed as input of the task */
      1,                        /* Priority of the task. */
      &xAWS_Send_Task_hnd);     /* Task handle. */

  Server.on("/", rootPage);

  /* change the AP label */
  //acConfig.apid = "AlineV-" + ESP.getEfuseMac();
  // Portal.config(acConfig);

  // Configure WiFiClientSecure to use the AWS certificates we generated
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  if (Portal.begin())
  {
    vPlayMelody(MelodyType_WifiSuccess);
    debugI("--- WiFi connected: %s ---", WiFi.localIP().toString());
    Serial.println("--- WiFi connected: " + WiFi.localIP().toString() + " ---");
    DEBUG_SERIAL("Web server started");

    // Connect to the MQTT broker on the AWS endpoint we defined earlier
    client.begin(AWS_IOT_ENDPOINT, 8883, net);

    // Try to connect to AWS and count how many times we retried.
    /*int retries = 0;
    DEBUG_SERIAL("Connecting to AWS IOT");

    while (!client.connect(DEVICE_NAME) && retries < AWS_MAX_RECONNECT_TRIES)
    {
      Serial.print(".");
      delay(100);
      retries++;
    }

    // Make sure that we did indeed successfully connect to the MQTT broker
    // If not we just end the function and wait for the next loop.
    if (!client.connected())
    {
      Serial.println(" Timeout!");
      debugE("MQTT Timeout!...");
    }
    else
    {
      DEBUG_SERIAL("MQTT Connected!");
      vPlayMelody(MelodyType_CommandReady);
    }*/
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
    //client.loop();

    /*if (!client.connected())
    {
      vMqttConnect();
    }*/

    vTaskDelay(60);
  }
  vTaskDelete(xAutoConnectServiceTaskHandle);
}

/**
 * @brief  
 * @note   
 * @retval None
 */
void vMqttConnect()
{
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    vTaskDelay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "try", "try"))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  //client.subscribe("/hello");
  // client.unsubscribe("/hello");
}
