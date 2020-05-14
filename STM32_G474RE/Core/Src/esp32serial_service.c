/*******************************************************************
 * esp32serial_service.c
 *
 *  Created on: Apr 28, 2020
 *      Author: Jack Lestrohan
 *
 *	Serializes and send encapsulated/formatted output to the USART
 *	ESP32 dedicated serial port
 *	All reception is made on a different task triggered via an IRQ
 *******************************************************************/

#include "esp32serial_service.h"
#include "configuration.h"
#include "printf.h"
#include "main.h"
#include "command_service.h"
#include "usart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "UartRingbuffer.h"
#include "hdlc_protocol.h"

#define MAX_HDLC_FRAME_LENGTH 512 /* this is the main frame length available */

osSemaphoreId_t xSemaphoreUartRingBuffer;

/**
 * @brief Function to send out one 8bit character
 * @param data
 */
void send_character(uint8_t data) {
	Uart_write(data);
}

/**
 * @brief Frame handler function. What to do with received data?
 * @param data
 * @param length
 */
void hdlc_frame_handler(const uint8_t *data, uint16_t length)
{
	printf("received thru HLDC: %.*s\n\r", length, (char *)data);

	//TODO: call JsonDecoder here then command center to decode commands
}

osMessageQueueId_t xQueueEspSerialTX;
osMessageQueueId_t xQueueEspSerialRX;
osMessageQueueId_t xQueueCommandParse; /* extern */

UART_HandleTypeDef huart3;

typedef StaticTask_t osStaticThreadDef_t;

static osThreadId_t xEsp32TXSerialServiceTaskHandle;
static osStaticThreadDef_t xEsp32TXSerialServiceTaControlBlock;
static uint32_t xEsp32TXSerialServiceTaBuffer[512];
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
static uint32_t xEsp32RXSerialServiceTaBuffer[512];
static const osThreadAttr_t xEsp32RXSerialServiceTa_attributes = {
		.name = "xEsp32RXSerialServiceTask",
		.stack_mem = &xEsp32RXSerialServiceTaBuffer[0],
		.stack_size = sizeof(xEsp32RXSerialServiceTaBuffer),
		.cb_mem = &xEsp32RXSerialServiceTaControlBlock,
		.cb_size = sizeof(xEsp32RXSerialServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_ESP32_RX
};


/**
 * Main SERIAL TX Task
 * @return
 */
void vEsp32TXSerialService_Start(void* vParameter)
{
	printf("Starting ESP32 Serial TX Service task...\n\r");
	char *msg = "Ready to receive UART";
	vSendFrame((uint8_t *)msg, strlen(msg));

	for (;;)
	{

		osDelay(1); /* every 5 seconds we send a test command */
	}
	osThreadTerminate(xEsp32TXSerialServiceTaskHandle);
}

/**
 * Main SERIAL RX Task using Head & Tail implementation
 * @return
 */
void vEsp32RXSerialService_Start(void* vParameter)
{
	printf("Starting ESP32 Serial RX Service task...\n\r");

	for (;;)
	{
		if (IsDataAvailable()) /* ask our little library if there's any data available for reading */
		{
			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_SET);

			char inChar = (char)Uart_read(); /* read one byte of data */
			//printf("%c", inChar);
			//Uart_write(inChar);

			// Pass all incoming data to hdlc char receiver
			vCharReceiver(inChar);



			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_RESET);
		}

		osDelay(1);
	}
	osThreadTerminate(xEsp32RXSerialServiceTaskHandle);
}

/**
 * @brief Main ESP32 serialization Init Routine
 * @return
 */
uint8_t uEsp32SerialServiceInit()
{
	/* Initializes the Ring Buffer */
	if (Ringbuf_init() == EXIT_FAILURE) {
		Error_Handler();
		return (EXIT_FAILURE);
	}

	xQueueEspSerialTX = osMessageQueueNew(5, sizeof(jsonMessage_t), NULL);
	if (xQueueEspSerialTX == NULL) {
		Error_Handler();
		return (EXIT_FAILURE);
	}

	xQueueEspSerialRX = osMessageQueueNew(5, sizeof(jsonMessage_t), NULL);
	if (xQueueEspSerialRX == NULL) {
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of TX Serial Task */
	xEsp32TXSerialServiceTaskHandle = osThreadNew(vEsp32TXSerialService_Start, NULL, &xEsp32TXSerialServiceTa_attributes);
	if (xEsp32TXSerialServiceTaskHandle == NULL) {
		printf("Front Servo Task TX Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of RX Serial Task */
	xEsp32RXSerialServiceTaskHandle = osThreadNew(vEsp32RXSerialService_Start, NULL, &xEsp32RXSerialServiceTa_attributes);
	if (xEsp32RXSerialServiceTaskHandle == NULL) {
		printf("Front Servo Task RX Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* Initialize Arduhdlc library with three parameters.
				1. Character send function, to send out HDLC frame one byte at a time.
				2. HDLC frame handler function for received frame.
				3. Length of the longest frame used, to allocate buffer in memory */
	uHdlcProtInit(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

	printf("Initializing ESP32 Serial Service... Success!\n\r");
	return EXIT_SUCCESS;
}



