/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 07:56:23
 * @ Description:
 *******************************************************************************************/

#include "serial_service.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"

xTaskHandle xSerialListenerTask_hnd = NULL;
QueueHandle_t xQueueCommandParse;

/**
 * @brief  Serial Listener task. Listens to UART and sends every command received to the command parser
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vSerialListenerTaskCode(void *pvParameters)
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
  vTaskDelete(NULL);
}

/**
 * @brief  Main Serial Listener setup
 * @note   
 * @retval None
 */
uint8_t setupUARTListener()
{
  /* we attempt to create the serial listener task */
  xTaskCreate(
      vSerialListenerTaskCode,   /* Task function. */
      "serialListener_task",     /* String with name of task. */
      10000,                     /* Stack size in words. */
      NULL,                      /* Parameter passed as input of the task */
      2,                         /* Priority of the task. */
      &xSerialListenerTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xSerialListenerTask_hnd == NULL)
  {
    debugE("Error creating xSerialListenerTask_hnd task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
