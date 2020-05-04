/*******************************************************************
 * esp32_serial.c
 *
 *  Created on: Apr 28, 2020
 *      Author: Jack Lestrohan
 *
 *	Serializes and send encapsulated/formatted output to the USART
 *	ESP32 dedicated serial port
 *	All reception is made on a different task triggered via an IRQ
 *******************************************************************/

// check https://stm32f4-discovery.net/2017/07/stm32-tutorial-efficiently-receive-uart-data-using-dma/
// see https://www.youtube.com/watch?v=wj427hpP81s
// https://controllerstech.com/ring-buffer-using-head-and-tail-in-stm32/
// https://www.youtube.com/watch?v=tWryJb2L0cU IMPORTANT!!
// see https://github.com/MaJerle/stm32-usart-uart-dma-rx-tx/tree/master/projects


#include "esp32serial_service.h"
#include "min_protocol.h"
#include "configuration.h"
#include "freertos_logger_service.h"
#include "usart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Size of Reception buffer */
#define LINEMAX 200 // Maximal allowed/expected line length

UART_HandleTypeDef huart3;

/* min protocol callbakcs */
void min_tx_byte(uint8_t port, uint8_t byte);
void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port);

/* min protocol context struct */
struct min_context min_ctx;

/* This is used to keep track of when the next example message will be sent */
uint32_t last_sent;

/* TX RTOSQueue for transmitting stuff TO the ESP32*/
osMessageQueueId_t xQueueStm32TXserial;
osMessageQueueId_t xQueueStm32RXserial;

/**
 * @brief Handle the reception of a MIN frame. This is the main interface to MIN for receiving
 * 			frames. It's called whenever a valid frame has been received (for transport layer frames
 * 			duplicates will have been eliminated).
 * @param min_id
 * @param min_payload
 * @param len_payload
 * @param port
 */
void min_application_handler(uint8_t min_id, uint8_t *min_payload, uint8_t len_payload, uint8_t port)
{
	/* In this simple example application we just echo the frame back when we get one, with the MIN ID
   one more than the incoming frame.

   We ignore the port because we have one context, but we could use it to index an array of
   contexts in a bigger application. */
	//loggerI(min_payload);
	//Serial.print("MIN frame with ID ");
	//Serial.print(min_id);
	//Serial.print(" received at ");
	//Serial.println(millis());
	//min_id++;
	// The frame echoed back doesn't go through the transport protocol: it's send back directly
	// as a datagram (and could be lost if there were noise on the serial line).
	//min_send_frame(&min_ctx, min_id, min_payload, len_payload);
}

/**
 * @brief Tell MIN the current time in milliseconds.
 * @return
 */
uint32_t min_time_ms(void)
{
	return HAL_GetTick();
}

typedef StaticTask_t osStaticThreadDef_t;

static osThreadId_t xEsp32TXSerialServiceTaskHandle;
static osStaticThreadDef_t xEsp32TXSerialServiceTaControlBlock;
static uint32_t xEsp32TXSerialServiceTaBuffer[256];
static const osThreadAttr_t xEsp32TXSerialServiceTa_attributes = {
		.name = "xEsp32TXSerialServiceTask",
		.stack_mem = &xEsp32TXSerialServiceTaBuffer[0],
		.stack_size = sizeof(xEsp32TXSerialServiceTaBuffer),
		.cb_mem = &xEsp32TXSerialServiceTaControlBlock,
		.cb_size = sizeof(xEsp32TXSerialServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_ESP32_TX
};

static osThreadId_t xEsp32RXSerialServiceTaskHandle;
static osStaticThreadDef_t xEsp32RXSerialServiceTaControlBlock;
static uint32_t xEsp32RXSerialServiceTaBuffer[256];
static const osThreadAttr_t xEsp32RXSerialServiceTa_attributes = {
		.name = "xEsp32RXSerialServiceTask",
		.stack_mem = &xEsp32RXSerialServiceTaBuffer[0],
		.stack_size = sizeof(xEsp32RXSerialServiceTaBuffer),
		.cb_mem = &xEsp32RXSerialServiceTaControlBlock,
		.cb_size = sizeof(xEsp32RXSerialServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_ESP32_RX
};

/**
 * @brief CALLBACK Transfer is starting
 * @param port
 */
void min_tx_start(uint8_t port)
{
	//FIXME: this is call at starting and a pain in the ass
	//loggerI("TX is starting...");
}

/**
 * @ Brief CALLBACK Transfer Finished
 * @param port
 */
void min_tx_finished(uint8_t port) {
	//FIXME same trouble as above!!!
	//loggerI("TX Finished Callback");
}

// Tell MIN how much space there is to write to the serial port. This is used
// inside MIN to decide whether to bother sending a frame or not.
uint16_t min_tx_space(uint8_t port)
{

	//uint16_t n = SerialUSB.availableForWrite();
	loggerI("tx space polling here");
	//FIXME: CHECK here how to handle this better!
	return 1024;
}

/**
 * @brief 	CALLBACK Sends a single byte to the selected port
 * 			Polling in the RX task allows that to be non blocking
 * @param port
 * @param byte
 */
void min_tx_byte(uint8_t port, uint8_t byte)
{
	loggerI("transmitting packet here");
	//HAL_UART_Transmit(&huart3, &byte, sizeof(uint8_t), HAL_MAX_DELAY);
	// FIXME: Queue is getting overflew
}

/**
 * Main SERIAL TX Task
 * @return
 */
void vEsp32TXSerialService_Start(void* vParameter)
{
	//osStatus_t status;
	//xMSGSerialObject_t bufferObj;	/* string or serial json to transmit */
	//uint8_t idx = 15;	/* index must be between 0 .. 63 */

	char *test = "THIS IS a Big Message for the ESP32 becauser we are going to have a fun time together soon";

	for (;;)
	{
		//status = osMessageQueueGet(xQueueStm32TXserial, &bufferObj, NULL, osWaitForever); /* wait for message */
		//if (status == osOK) {
		//	if(!min_queue_frame(&min_ctx, idx, (uint8_t *)test, strlen(test))) {
		/* The queue has overflowed for some reason */
		//		loggerE("Can't queue at time ");
		//	} else {
		//		loggerI("sent packet to UART3");
		//	}
		//}
		//HAL_UART_Transmit(&huart3, (uint8_t *)test, strlen(test), HAL_MAX_DELAY);
		osDelay(50);
	}
	osThreadTerminate(xEsp32TXSerialServiceTaskHandle);
}

/**
 * Main SERIAL TX Task
 * @return
 */
void vEsp32RXSerialService_Start(void* vParameter)
{
	char rx;
	osStatus_t status;
	char uartBuffer[LINEMAX];
	int uartBufferPos = 0;

	for (;;)
	{
		/* TODO: implement a queue here that will be triggered by a DMA reception of any UART activity
		 * see IRQ_handler */
		status = osMessageQueueGet(xQueueStm32RXserial, &rx, NULL, osWaitForever); /* wait for message */
		if (status == osOK) {

			if ((rx == '\r') || (rx == '\n')) // end-of-line condition
			{
				loggerI(uartBuffer);
				uartBuffer[uartBufferPos] = '\0'; // terminate the string with a 0
				uartBufferPos = 0;             // reset the index ready for another string
				/* todo: here we compare uartBuffer with cmd_parser_tag_list[] to check if the beginning of the command is recognized */
				/* send it thru a msgQueue to another dedicated task */

				uartBuffer[0] = '\0';
			}
			else
			{
				/* checks if the command received isn't too large (security) */
				if (uartBufferPos < LINEMAX)
				{
					uartBuffer[uartBufferPos++] = rx; // add the character into the buffer
				}
				/* if so we trash it */
				else
				{
					uartBuffer[0] = '\0';
					uartBufferPos = 0;
				}

				//	min_poll(&min_ctx, (uint8_t *)buf, (uint8_t)buf_len);

			}

			// after queue treatment call min_application_handler azs per https://github.com/min-protocol/min/blob/master/target/sketch_example1/sketch_example1.ino
		}
		osDelay(20);
	}
	osThreadTerminate(xEsp32RXSerialServiceTaskHandle);
}

/**
 * @brief Main ESP32 serialization Init Routine
 * @return
 */
uint8_t uEsp32SerialServiceInit()
{
	min_init_context(&min_ctx, 0);

	/* first let's create the transmit queue */
	xQueueStm32TXserial = osMessageQueueNew(255, sizeof(char), NULL); /* 255 chars max per frame so no need to go further */
	if (!xQueueStm32TXserial) {
		return (EXIT_FAILURE);
	}

	/* first let's create the transmit queue */
	xQueueStm32RXserial = osMessageQueueNew(255, sizeof(char), NULL); /* 255 chars max per frame so no need to go further */
	if (xQueueStm32RXserial == NULL) {
		return (EXIT_FAILURE);
	}

	/* creation of TX Serial Task */
	xEsp32TXSerialServiceTaskHandle = osThreadNew(vEsp32TXSerialService_Start, NULL, &xEsp32TXSerialServiceTa_attributes);
	if (xEsp32TXSerialServiceTaskHandle == NULL) {
		loggerE("Front Servo Task TX Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* creation of RX Serial Task */
	xEsp32RXSerialServiceTaskHandle = osThreadNew(vEsp32RXSerialService_Start, NULL, &xEsp32RXSerialServiceTa_attributes);
	if (xEsp32RXSerialServiceTaskHandle == NULL) {
		loggerE("Front Servo Task RX Initialization Failed");
		return (EXIT_FAILURE);
	}


	/* The board sends the message and expects to receive it back */
	/* DMA is programmed for reception before starting the transmission, in order to
	     be sure DMA Rx is ready when board 2 will start transmitting */

	/*##-2- Put UART peripheral in reception process ###########################*/
	uint8_t aRxBuffer[1];
	if (HAL_UART_Receive_DMA(&huart3, aRxBuffer, 1) != HAL_OK)
	{
		Error_Handler();
	}


	return EXIT_SUCCESS;
}



/* TODO IMPLEMENT THAT: using timers probably */
/**
 * void loop() {
  char buf[32];
  size_t buf_len;

  // Read some bytes from the USB serial port..
  if(SerialUSB.available() > 0) {
    buf_len = SerialUSB.readBytes(buf, 32U);
  }
  else {
    buf_len = 0;
  }
  // .. and push them into MIN. It doesn't matter if the bytes are read in one by
  // one or in a chunk (other than for efficiency) so this can match the way in which
  // serial handling is done (e.g. in some systems the serial port hardware register could
  // be polled and a byte pushed into MIN as it arrives).
  min_poll(&min_ctx, (uint8_t *)buf, (uint8_t)buf_len);

  // Every 1s send a MIN frame using the reliable transport stream.
  uint32_t now = millis();
  // Use modulo arithmetic so that it will continue to work when the time value wraps
  if (now - last_sent > 1000U) {
    // Send a MIN frame with ID 0x33 (51 in decimal) and with a 4 byte payload of the
    // the current time in milliseconds. The payload will be in this machine's
    // endianness - i.e. little endian - and so the host code will need to flip the bytes
    // around to decode it. It's a good idea to stick to MIN network ordering (i.e. big
    // endian) for payload words but this would make this example program more complex.
    if(!min_queue_frame(&min_ctx, 0x33U, (uint8_t *)&now, 4U)) {
      // The queue has overflowed for some reason
      Serial.print("Can't queue at time ");
      Serial.println(millis());
    }
    last_sent = now;
  }
}
 */

