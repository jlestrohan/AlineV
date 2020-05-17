/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-17 06:56:49
 * @ Description:
 *******************************************************************************************/

#include "stm32Serial_service.h"
#include "configuration_esp32.h"
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_service.h"
#include "hdlc_protocol.h"

xTaskHandle xStm32TXSerialServiceTask_hnd = NULL; /* for TX */
xTaskHandle xStm32RXSerialServiceTask_hnd = NULL; /* for RX */
extern QueueHandle_t xQueueSerialServiceTX;

/* function definitions */
void hdlc_frame_handler(const uint8_t *data, uint16_t length);
void send_character(uint8_t data);

HDLC_Prot hdlc(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

/**
 * @brief   Serial 2 TX Transmit task (from STM32)
 *          Transmit to UART2.
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vStm32TXSerialServiceTaskCode(void *pvParameters)
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
  vTaskDelete(xStm32TXSerialServiceTask_hnd);
}

/**
 * @brief  Main RX Receiver Task using the minimal protocol
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vStm32RXSerialServiceTaskCode(void *pvParameters)
{
  String RXJson;

  for (;;)
  {
    // Read some bytes from the USB serial port..
    if (Serial2.available() > 0)
    {
      //RXJson = Serial2.readString();
      // get the new byte:
      char inChar = (char)Serial2.read();
      //debugI("%c", inChar);
      // Pass all incoming data to hdlc char receiver
      hdlc.charReceiver(inChar);

      //debugI("%s", RXJson.c_str());
    }
    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}

/**
 * @brief  Main Serial Listener setup
 * @note   
 * @retval None
 */
uint8_t uSetupSTM32SerialService()
{
  /* setup UART communications to and from STM32 on UART2 port */
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  xQueueSerialServiceTX = xQueueCreate(5, sizeof(command_package_t));
  if (xQueueSerialServiceTX == NULL)
  {
    debugE("error creating the xQueueSerialServiceTX queue");
    return EXIT_FAILURE;
  }

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
    DEBUG_SERIAL("Error creating xStm32TXSerialService task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }
  DEBUG_SERIAL("xStm32TXSerialService Task ... Success!");

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
    DEBUG_SERIAL("xStm32RXSerialService Task ... Error!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }

  DEBUG_SERIAL("xStm32RXSerialService Task ... Success!..");
  return EXIT_SUCCESS;
}

/**
 * @brief  Function to send out one 8bit character
 * @note   
 * @param  data: 
 * @retval None
 */
void send_character(uint8_t data)
{
  Serial2.print((char)data);
}

/**
 * @brief   Frame handler function. What to do with received data? 
 * @note   
 * @param  *data: 
 * @param  length: 
 * @retval None
 */
void hdlc_frame_handler(const uint8_t *data, uint16_t length)
{
  // Do something with data that is in framebuffer
  debugV("RECEIVED: %.*s", length, (char *)data);
  //Serial.println(buf);
}
