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
#include "printf.h"
#include "main.h"
#include "command_service.h"
#include "usart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"

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

	for (;;)
	{

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
	printf("Starting ESP32 Serial RX Service task...\n\r");

	char Rx_Buffer[UART_BUFFER_SIZE]; /* will stay until power off so no heap frag... */
	int uartBufferPos = 0;
	jsonMessage_t msgType;

	for (;;)
	{
		if (IsDataAvailable()) /* ask our little library if there's any data available for reading */
		{
			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_SET);

			char inChar = (char)Uart_read(); /* read one byte of data */
			//Uart_write(inChar);

			if (inChar == '\n')  // is this the terminating carriage return
			{
				Rx_Buffer[uartBufferPos] = 0; // terminate the string with a 0
				uartBufferPos = 0;             // reset the index ready for another string
				/* todo: here we compare uartBuffer with cmd_parser_tag_list[] to check if the beginning of the command is recognized */
				/* send it thru a msgQueue to another dedicated task */

				printf("Attempting to queue message: %s\n\r", Rx_Buffer);
				strcpy(msgType.json, Rx_Buffer);
				msgType.msg_size = strlen(Rx_Buffer);
				osMessageQueuePut(xQueueCommandParse, &msgType, 0U, osWaitForever);

				Rx_Buffer[0] = '\0';
			}
			else
			{
				/* checks if the command received isn't too large (security) */
				if (uartBufferPos < UART_BUFFER_SIZE)
				{
					Rx_Buffer[uartBufferPos++] = inChar; // add the character into the buffer
				}
				/* if so we trash it */
				else
				{
					Rx_Buffer[0] = '\0';
					uartBufferPos = 0;
				}
			}



			HAL_GPIO_WritePin(GPIOA, LD3_Pin, GPIO_PIN_RESET);
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

	printf("Initializing ESP32 Serial Service... Success!\n\r");
	return EXIT_SUCCESS;
}


