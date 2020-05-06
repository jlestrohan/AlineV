/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-06 16:22:22
 * @ Description:
 *******************************************************************************************/

#include "stm32Serial_service.h"
#include "configuration_esp32.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_service.h"
#include "hdlc_protocol.h"

/* Function to send out byte/char */
void send_character(uint8_t data);

/* Function to handle a valid HDLC frame */
void hdlc_frame_handler(const uint8_t *data, uint16_t length);

HDLC_Prot hdlc(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

xTaskHandle xStm32TXSerialServiceTask_hnd = NULL; /* for TX */
xTaskHandle xStm32RXSerialServiceTask_hnd = NULL; /* for RX */

extern QueueHandle_t xQueueSerialServiceTX;

/* Function to send out one 8bit character */
void send_character(uint8_t data)
{
  Serial2.print((char)data);
}

/**
 * @brief   Serial 2 Listener task (from STM32)
 *          Listens to UART2 and pass all commands to the parser queue.
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vStm32TXSerialServiceTaskCode(void *pvParameters)
{
  cmdPackage_t cmdPack;

  for (;;)
  {
    /* receiving queue for objects tro be sent over */
    xQueueReceive(xQueueSerialServiceTX, &cmdPack, portMAX_DELAY);

    /* we send the frame over */
    hdlc.sendFrame((uint8_t *)&cmdPack, sizeof(cmdPack));

    vTaskDelay(20);
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
  String nom;

  for (;;)
  {
    // Read some bytes from the USB serial port..
    if (Serial2.available() > 0)
    {
      hdlc.charReceiver(Serial2.readBytes());
    }
    vTaskDelay(10);
  }
  vTaskDelete(xStm32RXSerialServiceTask_hnd);
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

  xQueueSerialServiceTX = xQueueCreate(20, sizeof(cmdPackage_t));
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
      2,                               /* Priority of the task. */
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
      5,                               /* Priority of the task. */
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

/* Frame handler function. What to do with received data? */
void hdlc_frame_handler(const uint8_t *data, uint16_t length)
{
  char buf[512];
  snprintf(buf, length + 1, (char *)data);
  // Do something with data that is in framebuffer
  debugI("%s", buf);
  Serial.println(buf);
}
