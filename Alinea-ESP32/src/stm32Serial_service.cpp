/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-03 00:05:29
 * @ Description:
 *******************************************************************************************/

#include "stm32Serial_service.h"
#include "min_protocol.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"

xTaskHandle xStm32TXSerialServiceTask_hnd = NULL; /* for TX */
xTaskHandle xStm32RXSerialServiceTask_hnd = NULL; /* for RX */

QueueHandle_t xQueueCommandParse;

/* min protocol mandatory vars */
struct min_context min_ctx;
uint32_t last_sent;

/**
 * @brief CALLBACK Transfer is starting
 * @param port
 */
void min_tx_start(uint8_t port)
{
  //NOP
}

/**
 * @ Brief CALLBACK Transfer Finished
 * @param port
 */
void min_tx_finished(uint8_t port)
{
  //NOP
}

/**
 * @brief  CALLBACK  Tell MIN how much space there is to write to the serial port. This is used
 *          inside MIN to decide whether to bother sending a frame or not.
 * @note   
 * @param  port: 
 * @retval 
 */
uint16_t min_tx_space(uint8_t port)
{
  // Ignore 'port' because we have just one context. But in a bigger application
  // with multiple ports we could make an array indexed by port to select the serial
  // port we need to use.
  uint16_t n = Serial2.availableForWrite();
  return n;
}

/**
 * @brief  CALLBACK Writes a single character to the Serial2 port
 * @note   
 * @param  port: 
 * @param  byte: 
 * @retval None
 */
void min_tx_byte(uint8_t port, uint8_t byte)
{
  /* Ignore 'port' because we have just one context. */
  Serial2.write(&byte, 1U);
}

/**
 * @brief Tell MIN the current time in milliseconds. 
 * @note   
 * @retval 
 */
uint32_t min_time_ms(void)
{
  return millis();
}

// Handle the reception of a MIN frame. This is the main interface to MIN for receiving
// frames. It's called whenever a valid frame has been received (for transport layer frames
// duplicates will have been eliminated).
void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
  // In this simple example application we just echo the frame back when we get one, with the MIN ID
  // one more than the incoming frame.
  //
  // We ignore the port because we have one context, but we could use it to index an array of
  // contexts in a bigger application.
  debugI("MIN frame with ID %d", min_id);
  debugI(" received at %ul", millis());
  debugI("%s", (char *)min_payload);

  //min_id++;
  // The frame echoed back doesn't go through the transport protocol: it's send back directly
  // as a datagram (and could be lost if there were noise on the serial line).
  //min_send_frame(&min_ctx, min_id, min_payload, len_payload);
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
  char buf[32];
  size_t buf_len;

  for (;;)
  {
    // Read some bytes from the USB serial port..
    if (Serial2.available() > 0)
    {
      buf_len = Serial2.readBytes(buf, 32U);
    }
    else
    {
      buf_len = 0;
    }

    /* .. and push them into MIN. It doesn't matter if the bytes are read in one by
     one or in a chunk (other than for efficiency) so this can match the way in which
     serial handling is done (e.g. in some systems the serial port hardware register could
     be polled and a byte pushed into MIN as it arrives). */
    min_poll(&min_ctx, (uint8_t *)buf, (uint8_t)buf_len);

    // Every 1s send a MIN frame using the reliable transport stream.
    uint32_t now = millis();
    // Use modulo arithmetic so that it will continue to work when the time value wraps
    if (now - last_sent > 1000U)
    {
      // Send a MIN frame with ID 0x33 (51 in decimal) and with a 4 byte payload of the
      // the current time in milliseconds. The payload will be in this machine's
      // endianness - i.e. little endian - and so the host code will need to flip the bytes
      // around to decode it. It's a good idea to stick to MIN network ordering (i.e. big
      // endian) for payload words but this would make this example program more complex.
      if (!min_queue_frame(&min_ctx, 0x33U, (uint8_t *)&now, 4U))
      {
        // The queue has overflowed for some reason
        Serial.print("Can't queue at time ");
        Serial.println(millis());
      }
      last_sent = now;
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

  /* waits for serial port */
  //while (!Serial2)
  //{
  //  ; // Wait for serial port
  //}

  // Initialize the single context. Since we are going to ignore the port value we could
  // use any value. But in a bigger program we would probably use it as an index.
  min_init_context(&min_ctx, 0);

  /* we attempt to create the serial listener task */
  xTaskCreate(
      vStm32TXSerialServiceTaskCode,   /* Task function. */
      "serial0Listener_task",          /* String with name of task. */
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
