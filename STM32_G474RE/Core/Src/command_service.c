/*******************************************************************
 * command_service.c
 *
 *  Created on: Apr 28, 2020
 *      Author: Jack Lestrohan
 *
 *	treats commands from and to esp32
 *******************************************************************/
#define ESP_UUID	2883801292		/* security, all commands coming from another chip are trashed */

#include <command_service.h>
#include "esp32serial_service.h"
#include "configuration.h"
#include <stdlib.h>
#include "UartRingbuffer.h"
#include "printf.h"
#include "main.h"

osMessageQueueId_t xQueueCommandParse;
UART_HandleTypeDef hlpuart1;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xCommandParserServiceTaskHnd;
static osStaticThreadDef_t xCommandParserServiceTaControlBlock;
static uint32_t xCommandParserServiceTaBuffer[256];
static const osThreadAttr_t xCommandParserServiceTa_attributes = {
		.name = "xCommandParserServiceTask",
		.stack_mem = &xCommandParserServiceTaBuffer[0],
		.stack_size = sizeof(xCommandParserServiceTaBuffer),
		.cb_mem = &xCommandParserServiceTaControlBlock,
		.cb_size = sizeof(xCommandParserServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_CMD_SERVICE, };



/**
 * Main Task routine
 * @param vParameter
 */
void vCommandParserServiceTask(void *vParameter)
{
	printf("Starting Command Parser Service task...\n\r");
	osStatus_t status = -1;
	jsonMessage_t msgType;

	for (;;)
	{
		/* wait for pure JSON string TODO: bug here FIXME::: */
		status = osMessageQueueGet(xQueueCommandParse, &msgType, NULL, osWaitForever);
		if (status == osOK) {
			printf("received: %s\n\r", msgType.json);
		}

		osDelay(50);
	}
	osThreadTerminate(xCommandParserServiceTaskHnd);
}

/**
 * Main Command Service Initialization Routine
 * @return
 */
uint8_t uCmdParseServiceInit()
{
	xQueueCommandParse = osMessageQueueNew(5, sizeof(jsonMessage_t), NULL);
	if (xQueueCommandParse == NULL) {
		printf("Command Service Queue Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xCommandParserService Task */
	xCommandParserServiceTaskHnd = osThreadNew(vCommandParserServiceTask, NULL, &xCommandParserServiceTa_attributes);
	if (xCommandParserServiceTaskHnd == NULL) {
		printf("Command Service Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Command Parser Service... Success!\n\r");
	return EXIT_SUCCESS;
}

/**
 * Decodes JSON and starts parsing commands sent
 * @param type
 */
void jsonDecode(jsonMessage_t *type ){

}


