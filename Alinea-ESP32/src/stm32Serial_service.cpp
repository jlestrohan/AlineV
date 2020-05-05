/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-06 01:41:48
 * @ Description:
 *******************************************************************************************/

#include "stm32Serial_service.h"
#include "configuration_esp32.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"
#include "packet_ptcl.h"

xTaskHandle xStm32TXSerialServiceTask_hnd = NULL; /* for TX */
xTaskHandle xStm32RXSerialServiceTask_hnd = NULL; /* for RX */

QueueHandle_t xQueueCommandParse;

/**
 * @brief   Serial 2 Listener task (from STM32)
 *          Listens to UART2 and pass all commands to the parser queue.
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vStm32TXSerialServiceTaskCode(void *pvParameters)
{

  for (;;)
  {

    vTaskDelay(10);
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
  size_t buf_len;
  char incomingByte[230]; // for incoming serial data
  for (;;)
  {
    // Read some bytes from the USB serial port..
    if (Serial2.available() > 0)
    {
      //buf_len = Serial2.readBytes(buf, 32U);
      // read the incoming byte:
      debugI("%s", Serial2.readString().c_str());
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

  /* Allocates memory pool */

  /* Initialize the single context. Since we are going to ignore the port value we could
   use any value. But in a bigger program we would probably use it as an index. */
  //min_init_context(&min_ctx, 0);

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
    debugE("Error creating xStm32TXSerialService task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }

  /* we attempt to create the serial 2 listener task */
  xTaskCreate(
      vStm32RXSerialServiceTaskCode,   /* Task function. */
      "serial2Listener_task",          /* String with name of task. */
      10000,                           /* Stack size in words. */
      NULL,                            /* Parameter passed as input of the task */
      5,                               /* Priority of the task. */
      &xStm32RXSerialServiceTask_hnd); /* Task handle. */

  /* check and deinit stuff if applicable */
  if (xStm32RXSerialServiceTask_hnd == NULL)
  {
    debugE("Error creating xStm32RXSerialService task!");
    /* cannot create task, remove all created stuff and exit failure */
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
