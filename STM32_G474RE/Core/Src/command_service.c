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
#include "jsmn.h"
#include <stdbool.h>

osMessageQueueId_t xQueueCommandParse;
UART_HandleTypeDef hlpuart1;

typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t xCommandParserServiceTaskHnd;
static osStaticThreadDef_t xCommandParserServiceTaControlBlock;
static uint32_t xCommandParserServiceTaBuffer[512];
static const osThreadAttr_t xCommandParserServiceTa_attributes = {
		.name = "xCommandParserServiceTask",
		.stack_mem = &xCommandParserServiceTaBuffer[0],
		.stack_size = sizeof(xCommandParserServiceTaBuffer),
		.cb_mem = &xCommandParserServiceTaControlBlock,
		.cb_size = sizeof(xCommandParserServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_CMD_SERVICE, };

jsmn_parser parser;

/* function prototypes */
void jsonDecode(const char *json, uint16_t length, bool reentry);

/**
 * Main Task routine
 * @param vParameter
 */
void vCommandParserServiceTask(void *vParameter)
{
	printf("Starting Command Parser Service task...\n\r");
	osStatus_t status = -1;
	jsonMessage_t msgType;

	jsmn_init(&parser);

	for (;;)
	{
		/* wait for pure JSON string TODO: bug here FIXME::: */
		status = osMessageQueueGet(xQueueCommandParse, &msgType, NULL, osWaitForever);
		if (status == osOK) {
			printf("received: %s\n\r", msgType.json);
			jsonDecode(msgType.json, msgType.msg_size, false);
			/* we just got the command, time to parse it and execute what has to be executed */

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
	xQueueCommandParse = osMessageQueueNew(2, sizeof(jsonMessage_t), NULL);
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
void jsonDecode(const char *json, uint16_t length, bool reentry)
{
	int r;
	jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * 256);

	if (reentry && tokens) {
		tokens = realloc(tokens, 512);
	}

	r = jsmn_parse(&parser, json, length, tokens, reentry ? 512 : 256);

	switch (r) {
	case JSMN_ERROR_INVAL:
		printf("bad token, JSON string is corrupted\n\r");
		Error_Handler(); //TODO remove this!!!
		break;
	case JSMN_ERROR_NOMEM:
		printf("not enough tokens, JSON string is too large. Trying to reallocate\n\r");
		if (!reentry) {
			reentry = true;
			jsonDecode(json, length, reentry); /* call self again let's try wioth a bigger buffer */
		}
		Error_Handler();
		break;
	case JSMN_ERROR_PART:
		printf("JSON string is too short, expecting more JSON data\n\r");
		Error_Handler();
		break;
	default:
		printf("JSON Parsing ok... processing command..\n\r");
		break;
	}

	vPortFree(tokens);
}



