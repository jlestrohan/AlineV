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

// check https://stm32f4-discovery.net/2017/07/stm32-tutorial-efficiently-receive-uart-data-using-dma/
// see https://www.youtube.com/watch?v=wj427hpP81s
// https://controllerstech.com/ring-buffer-using-head-and-tail-in-stm32/
// https://www.youtube.com/watch?v=tWryJb2L0cU IMPORTANT!!
// see https://github.com/MaJerle/stm32-usart-uart-dma-rx-tx/tree/master/projects

#include "esp32serial_service.h"
#include "configuration.h"
#include "debug.h"
#include "command.service.h"
#include "usart.h"
#include "UartRingbuffer.h"
#include "hdlc_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_HDLC_FRAME_LENGTH 512

osMessageQueueId_t xQueueEspSerialTX;
osMessageQueueId_t xQueueEspSerialRX;

UART_HandleTypeDef huart3;


typedef StaticTask_t osStaticThreadDef_t;

static osThreadId_t xEsp32TXSerialServiceTaskHandle;
static osStaticThreadDef_t xEsp32TXSerialServiceTaControlBlock;
static uint32_t xEsp32TXSerialServiceTaBuffer[1024];
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
static uint32_t xEsp32RXSerialServiceTaBuffer[1024];
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
	HAL_UART_Transmit(&hlpuart1, (uint8_t *)data, length, HAL_MAX_DELAY);
}

/**
 * Main SERIAL TX Task
 * @return
 */
void vEsp32TXSerialService_Start(void* vParameter)
{
	dbg_printf("Starting vEsp32TXSerial Service task...");
	//PayLoad_t payload;// = "serialized json here";
	//osStatus_t status;

	for (;;)
	{
		/* TODO: add queue here, that will send payload on request. */
		/*status = osMessageQueueGet(xQueueEspSerialTX, &payload, NULL, osWaitForever); /* wait for message */
		/*if (status == osOK) {
			vSendFrame((uint8_t *)payload.pLoadString , strlen(payload.pLoadString));
		}*/
		osDelay(20); /* every 5 seconds we send a test command */
	}


	osThreadTerminate(xEsp32TXSerialServiceTaskHandle);
}

/**
 * Main SERIAL TX Task using Head & Tail implementation
 * @return
 */
void vEsp32RXSerialService_Start(void* vParameter)
{
	dbg_printf("Starting vEsp32RXSerial Service task...");

	for (;;)
	{
		if (IsDataAvailable()) /* ask our little library if there's any data available for reading */
		{
			//HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_SET);

			char inChar = (char)Uart_read(); /* read one byte of data */
			Uart_write(inChar);
			// Pass all incoming data to hdlc char receiver
			//vCharReceiver(inChar);

			//HAL_UART_Transmit(&hlpuart1, (uint8_t *)&inChar, sizeof(char), HAL_MAX_DELAY);
			//HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_RESET);
		}

		osDelay(10);
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
	Ringbuf_init();

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

	/* Initialize Arduhdlc library with three parameters.
	1. Character send function, to send out HDLC frame one byte at a time.
	2. HDLC frame handler function for received frame.
	3. Length of the longest frame used, to allocate buffer in memory */
	uHdlcProtInit(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

	/* creation of TX Serial Task */
	xEsp32TXSerialServiceTaskHandle = osThreadNew(vEsp32TXSerialService_Start, NULL, &xEsp32TXSerialServiceTa_attributes);
	if (xEsp32TXSerialServiceTaskHandle == NULL) {
		dbg_printf("Front Servo Task TX Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of RX Serial Task */
	xEsp32RXSerialServiceTaskHandle = osThreadNew(vEsp32RXSerialService_Start, NULL, &xEsp32RXSerialServiceTa_attributes);
	if (xEsp32RXSerialServiceTaskHandle == NULL) {
		dbg_printf("Front Servo Task RX Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


