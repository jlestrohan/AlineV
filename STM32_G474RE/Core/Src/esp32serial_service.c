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

osMessageQueueId_t xQueueEspSerialTX;
osMutexId_t mUartRingBufferMutex; /*extern */
osMessageQueueId_t xQueueCommandParse; /* extern */

UART_HandleTypeDef huart3;

/** SERIAL TX TASK **/
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

/** SERIAL RX TASK **/
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
 * @brief Function to send out one 8bit character
 * @param data
 */
static void send_character(uint8_t data)
{
	osMutexAcquire(mUartRingBufferMutex, osWaitForever);
	Uart_write(data);
	osMutexRelease(mUartRingBufferMutex);
}

/**
 * @brief Frame handler function. What to do with received data?
 * @param data
 * @param length
 */
static void hdlc_frame_handler(uint8_t *data, uint8_t length)
{
	jsonMessage_t msg;

	//printf("hdlc_frame_handler, from ESP32: %.*s with length: %d\n\r", length, data, length);
	/* some cleanup */
	memcpy(msg.json, data, length);
	msg.msg_size = length;

	/* straight to command service to be interpreted */
	osMessageQueuePut(xQueueCommandParse, &msg, 0U, osWaitForever);
}

/**
 * Main SERIAL TX Task
 * @return
 */
void vEsp32TXSerialService_Start(void* vParameter)
{
	printf("Starting ESP32 Serial TX Service task...\n\r");

	osStatus_t status;
	jsonMessage_t msg_packet;

	xQueueEspSerialTX = osMessageQueueNew(2, sizeof(jsonMessage_t), NULL);
	if (xQueueEspSerialTX == NULL) {
		Error_Handler();
	}

	for (;;)
	{
		/* awaits for the frame to send */
		status = osMessageQueueGet(xQueueEspSerialTX, &msg_packet, 0U, osWaitForever);
		if (status == osOK) {
			//printf("sending hdlc frame for %d bytes", msg_packet.msg_size);
			osMutexAcquire(mHdlcProtocolMutex, osWaitForever);
			vSendFrame(msg_packet.json, msg_packet.msg_size);
			osMutexRelease(mHdlcProtocolMutex);
		}

		osDelay(20); /* every 5 seconds we send a test command */
	}
	osThreadTerminate(NULL);
}

/**
 * Main SERIAL RX Task using Head & Tail implementation
 * @return
 */
void vEsp32RXSerialService_Start(void* vParameter)
{
	printf("Starting ESP32 Serial RX Service task...\n\r");

	/* Initializes the RX Ring Buffer */
	if (Ringbuf_init() == EXIT_FAILURE) {
		printf("Failed creating RX Ring buffer, this is a CRITICAL ERROR, exiting now!\n\r");
		Error_Handler();
	}

	for (;;)
	{
		if (IsDataAvailable()) /* ask our little library if there's any data available for reading */
		{
			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_SET);

			osMutexAcquire(mUartRingBufferMutex, osWaitForever);
			char inChar = (char)Uart_read(); /* read one byte of data */
			osMutexRelease(mUartRingBufferMutex);

			// Pass all incoming data to hdlc char receiver
			osMutexAcquire(mHdlcProtocolMutex, osWaitForever);
			vCharReceiver(inChar);
			osMutexRelease(mHdlcProtocolMutex);

			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_RESET);
		}
		osDelay(10);
	}
	osThreadTerminate(NULL);
}

/**
 * @brief Main ESP32 serialization Init Routine
 * @return
 */
uint8_t uEsp32SerialServiceInit()
{
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



