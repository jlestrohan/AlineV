/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-29 00:59:37
 * @ Description:
 *******************************************************************************************/

#include "serial_service.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"

xTaskHandle xSerial0ListenerTask_hnd = NULL; /* for debug only */
xTaskHandle xSerial2ListenerTask_hnd = NULL; /* from STM32 */

QueueHandle_t xQueueCommandParse;

/**
 * @brief   Serial 2 Listener task (from STM32)
 *          Listens to UART2 and pass all commands to the parser queue.
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vSerial2ListenerTaskCode(void *pvParameters)
{
  char uartBuffer[CMD_TAG_MSG_MAX_LGTH + 1];
  int uartBufferPos = 0;

  for (;;)
  {
    if (Serial2.available())
    {
      char ch = (char)Serial2.read();
      if (ch == '\n') // is this the terminating carriage return
      {
        uartBuffer[uartBufferPos] = 0; // terminate the string with a 0
        uartBufferPos = 0;             // reset the index ready for another string
        /* todo: here we compare uartBuffer with cmd_parser_tag_list[] to check if the beginning of the command is recognized */
        /* send it thru a msgQueue to another dedicated task */
        if (xQueueCommandParse)
          xQueueSend(xQueueCommandParse, &uartBuffer, portMAX_DELAY);
        else
          debugE("xQueueCommandParse not available or NULL - last command not processed");
        uartBuffer[0] = '\0';
      }
      else
      {
        /* checks if the command received isn't too large (security) */
        if (uartBufferPos < CMD_TAG_MSG_MAX_LGTH)
        {
          uartBuffer[uartBufferPos++] = ch; // add the character into the buffer
        }
        /* if so we trash it */
        else
        {
          uartBuffer[0] = '\0';
          uartBufferPos = 0;
        }
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(xSerial2ListenerTask_hnd);
}

/**
 * @brief  Serial 0 Listener task. Listens to UART0 and sends every command received to the command parser (debug purposes)
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vSerial0ListenerTaskCode(void *pvParameters)
{
  char uartBuffer[CMD_TAG_MSG_MAX_LGTH + 1];
  int uartBufferPos = 0;

  for (;;)
  {
    if (Serial.available())
    {
      char ch = (char)Serial.read();
      if (ch == '\n') // is this the terminating carriage return
      {
        uartBuffer[uartBufferPos] = 0; // terminate the string with a 0
        uartBufferPos = 0;             // reset the index ready for another string
        /* todo: here we compare uartBuffer with cmd_parser_tag_list[] to check if the beginning of the command is recognized */
        /* send it thru a msgQueue to another dedicated task */
        if (xQueueCommandParse)
          xQueueSend(xQueueCommandParse, &uartBuffer, portMAX_DELAY);
        else
          debugE("xQueueCommandParse not available or NULL - last command not processed");

        uartBuffer[0] = '\0';
        Serial.print(ch);
      }
      else
      {
        if (uartBufferPos < CMD_TAG_MSG_MAX_LGTH)
        {
          uartBuffer[uartBufferPos++] = ch; // add the character into the buffer
          Serial.print(ch);
        }
        else
        {
          /* message too long we trash it */
          debugE("Serial received message is too long - dumped!");
          uartBuffer[uartBufferPos] = 0; // terminate the string with a 0
          uartBufferPos = 0;             // reset the index ready for another string
          uartBuffer[0] = '\0';
        }
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(xSerial0ListenerTask_hnd);
}

/**
 * @brief  Main Serial Listener setup
 * @note   
 * @retval None
 */
uint8_t setupUARTListener()
{
  /* setup UART communications to and from STM32 on UART2 port */
  Serial2.begin(460800, SERIAL_8N1, RXD2, TXD2);

  /* we attempt to create the serial listener task */
  xTaskCreate(
      vSerial0ListenerTaskCode,   /* Task function. */
      "serial0Listener_task",     /* String with name of task. */
      10000,                      /* Stack size in words. */
      NULL,                       /* Parameter passed as input of the task */
      2,                          /* Priority of the task. */
      &xSerial0ListenerTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xSerial0ListenerTask_hnd == NULL)
  {
    debugE("Error creating xSerial0Listener task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }

  /* we attempt to create the serial 2 listener task */
  xTaskCreate(
      vSerial2ListenerTaskCode,   /* Task function. */
      "serial2Listener_task",     /* String with name of task. */
      10000,                      /* Stack size in words. */
      NULL,                       /* Parameter passed as input of the task */
      5,                          /* Priority of the task. */
      &xSerial2ListenerTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xSerial2ListenerTask_hnd == NULL)
  {
    debugE("Error creating xSerial2Listener task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
