/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 08:59:11
 * @ Description:
 *******************************************************************************************/

#include "FreeRTOS.h"
#include "stm32Serial_service.h"
#include "configuration_esp32.h"
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_service.h"
#include "hdlc_protocol.h"
#include "data_service.h"

static xTaskHandle xStm32TXSerialServiceTask_hnd = NULL; /* for TX */
static xTaskHandle xStm32RXSerialServiceTask_hnd = NULL; /* for RX */

SemaphoreHandle_t xSemaphoreSerial2Mutex;
QueueHandle_t xQueueDataJson;

/* function definitions */
static void hdlc_frame_handler(const uint8_t *data, uint16_t length);
static void send_character(uint8_t data);

HDLC_Prot hdlc(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

/* ------------------------------ MAIN TX TASK ------------------------------ */
/**
 * @brief   Serial 2 TX Transmit task (from STM32)
 *          Transmit to UART2.
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
static void vStm32TXSerialServiceTaskCode(void *pvParameters)
{
  jsonMessage_t jsonMsg;

  for (;;)
  {
    /* receiving queue for objects tro be sent over */
    xQueueReceive(xQueueSerialServiceTX, &jsonMsg, portMAX_DELAY);

    /* we send the json over */
    debugI("SENT: %.*s", jsonMsg.msg_size, (char *)jsonMsg.json);
    //Serial2.println(jsonMsg.json);

    hdlc.sendFrame(jsonMsg.json, jsonMsg.msg_size);

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}

/* ------------------------------ MAIN RX TASK ------------------------------ */
/**
 * @brief  Main RX Receiver Task using the minimal protocol
 * @note   
 * @param  *parameter: 
 * @retval None
 */
static void vStm32RXSerialServiceTaskCode(void *pvParameters)
{
  String RXJson;
  char inChar;

  for (;;)
  {
    //debugI("Free HEAP: %lu on HEAP TOTAL: %lu", ESP.getFreeHeap(), ESP.getHeapSize());

    // Read some bytes from the USB serial port..
    if (Serial2.available() > 0)
    {
      xSemaphoreTake(xSemaphoreSerial2Mutex, portMAX_DELAY);
      inChar = (char)Serial2.read();
      hdlc.charReceiver(inChar);
      xSemaphoreGive(xSemaphoreSerial2Mutex);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

/* ----------------------------- SETUP FUNCTION ----------------------------- */
/**
 * @brief  Main Serial Listener setup
 * @note   
 * @retval None
 */
uint8_t uSetupSTM32SerialService()
{
  xQueueSerialServiceTX = xQueueCreate(5, sizeof(jsonMessage_t));
  if (xQueueSerialServiceTX == NULL)
  {
    debugE("error creating the xQueueSerialServiceTX queue\n\r");
    Serial.println("error creating the xQueueSerialServiceTX queue\n\r");

    vTaskDelete(NULL);
  }

  xSemaphoreSerial2Mutex = xSemaphoreCreateMutex();
  if (xSemaphoreSerial2Mutex == NULL)
  {
    debugE("error creating the xSemaphoreSerial2Mutex Semaphore (mutex)");
    Serial.println("error creating the xSemaphoreSerial2Mutex Semaphore (mutex)");

    return EXIT_FAILURE;
  }

  /* setup UART communications to and from STM32 on UART2 port */
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  /* we attempt to create the serial listener task */
  DEBUG_SERIAL("xStm32TXSerialService Task ... Creating");
  xTaskCreate(
      vStm32TXSerialServiceTaskCode,   /* Task function. */
      "vStm32TXSerialServiceTaskCode", /* String with name of task. */
      10000,                           /* Stack size in words. */
      NULL,                            /* Parameter passed as input of the task */
      5,                               /* Priority of the task. */
      &xStm32TXSerialServiceTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xStm32TXSerialServiceTask_hnd == NULL)
  {
    debugE("Error creating xStm32TXSerialService task!");
    Serial.println("Error creating xStm32TXSerialService task!");

    /* cannot create task, remove all created stuff and exit failure */
    vTaskDelete(NULL);
  }
  else
  {
    debugD("xStm32TXSerialService Task ... Success!");
    Serial.println("xStm32TXSerialService Task ... Success!");
  }

  DEBUG_SERIAL("xStm32RXSerialService Task ... Creating");
  /* we attempt to create the serial 2 listener task */
  xTaskCreate(
      vStm32RXSerialServiceTaskCode,   /* Task function. */
      "vStm32RXSerialServiceTaskCode", /* String with name of task. */
      10000,                           /* Stack size in words. */
      NULL,                            /* Parameter passed as input of the task */
      8,                               /* Priority of the task. */
      &xStm32RXSerialServiceTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xStm32RXSerialServiceTask_hnd == NULL)
  {
    debugE("xStm32RXSerialService Task ... Error!");
    Serial.println("xStm32RXSerialService Task ... Error!");

    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }
  else
  {
    debugD("xStm32RXSerialService Task ... Success!");
    Serial.println("xStm32RXSerialService Task ... Success!");
  }

  return EXIT_SUCCESS;
}

/* ------------------------------ SEND ONE CHAR ----------------------------- */
/**
 * @brief  Function to send out one 8bit character
 * @note   
 * @param  data: 
 * @retval None
 */
static void send_character(uint8_t data)
{
  xSemaphoreTake(xSemaphoreSerial2Mutex, portMAX_DELAY);
  Serial2.print((char)data);
  xSemaphoreGive(xSemaphoreSerial2Mutex);
}

/* --------------------------- HDLC FRAME HANDLER --------------------------- */
/**
 * @brief   Frame handler function. What to do with received data? 
 * @note   
 * @param  *data: 
 * @param  length: 
 * @retval None
 */
static void hdlc_frame_handler(const uint8_t *data, uint16_t length)
{
  /* Do something with data that is in framebuffer */
  //debugI("RECEIVED: %.*s for %d bytes", length, (char *)data, length);

  /* we got a json from the stm32, we direct it to the data service to be rearranged by the ESP32, then
  encapsulated thru the AWS service to ne sent over to the cloud */
  xJsonPackage_t json_pack;
  memcpy(json_pack.json_str, data, length);
  json_pack.length = length;

  if (xQueueDataJson != NULL)
  {
    // debugI("resending STM32 JSON to Data Service: %.*s", length, json_pack.json_str);
    xQueueSend(xQueueDataJson, &json_pack, portMAX_DELAY); /* send to the command parser that will form the json and forward it to the postman */
  }
  else
  {
    debugE("Failed resending STM32 json packet to data service...");
  }
}
