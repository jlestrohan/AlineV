/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-22 17:49:11
 * @ Description:
 *******************************************************************************************/

#include "serial_listener.h"
#include "ota.h"

char uartBuffer[50];
int uartBufferIndex = 0;

RemoteDebug Debug;

/**
 * @brief  Serial Listener task
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void serialListener_task(void *parameter)
{
  for (;;)
  {
    if (Serial.available())
    {
      char ch = (char)Serial.read();
      if (ch == '\n') // is this the terminating carriage return
      {
        uartBuffer[uartBufferIndex] = 0; // terminate the string with a 0
        uartBufferIndex = 0;             // reset the index ready for another string
        // do something with the string
        debugI("%s", uartBuffer);
      }
      else
        uartBuffer[uartBufferIndex++] = ch; // add the character into the buffer
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
void setupUARTListener()
{
/** FREERTOS OTA Task */
  xTaskCreate(
      serialListener_task,   /* Task function. */
      "serialListener_task", /* String with name of task. */
      10000,                 /* Stack size in words. */
      NULL,                  /* Parameter passed as input of the task */
      2,                     /* Priority of the task. */
      NULL);                 /* Task handle. */
}