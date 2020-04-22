/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-22 17:30:22
 * @ Description:
 *******************************************************************************************/

// check https://github.com/muratdemirtas/ESP8266-UART-RX-INTERRUPT/blob/master/main.ino
// check https://github.com/maxgerhardt/pio-stm32-with-esp8266-dht11
// https://github.com/krzychb/esp-just-slip
// https://github.com/martin-ger/esp_slip_router
// https://forum.arduino.cc/index.php?topic=576983.0

#include <Arduino.h>
#include "ota.h"
#include <FreeRTOS.h>

RemoteDebug Debug;

char uartBuffer[50];
int uartBufferIndex = 0;

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
 * @brief  Main setup
 * @note   
 * @retval None
 */
void setup()
{
  Serial.begin(115200);

  setupOTA();
  debugI("Ready.");

  /** FREERTOS OTA Task */
  xTaskCreate(
      serialListener_task,   /* Task function. */
      "serialListener_task", /* String with name of task. */
      10000,                 /* Stack size in words. */
      NULL,                  /* Parameter passed as input of the task */
      2,                     /* Priority of the task. */
      NULL);                 /* Task handle. */

  Serial.println("Ready UART");
}

/**
 * @brief  Main program loop
 * @note   
 * @retval None
 */
void loop()
{
}
