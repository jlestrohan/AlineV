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
static osMessageQueueId_t xQueueCommandInterpreter;
UART_HandleTypeDef hlpuart1;

/* command parser (json) task*/
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

/* command interpreter task */
static osThreadId_t xCommandInterpreterServiceTaskHnd;
static osStaticThreadDef_t xCommandInterpreterServiceTaControlBlock;
static uint32_t xCommandInterpreterServiceTaBuffer[512];
static const osThreadAttr_t xCommandInterpreterServiceTa_attributes = {
		.name = "xCommandInterpreterServiceTask",
		.stack_mem = &xCommandInterpreterServiceTaBuffer[0],
		.stack_size = sizeof(xCommandInterpreterServiceTaBuffer),
		.cb_mem = &xCommandInterpreterServiceTaControlBlock,
		.cb_size = sizeof(xCommandInterpreterServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_CMD_INTERP_SERVICE };

jsmn_parser parser;

/* function prototypes */
void jsonDecode(const char *json, uint16_t length, bool reentry, size_t tokcount);

/**
 * Main Task routine
 * @param vParameter
 */
void vCommandParserServiceTask(void *vParameter)
{
	printf("Starting Command Parser Service task...\n\r");
	osStatus_t status;
	jsonMessage_t msgType;

	jsmn_init(&parser);

	xQueueCommandParse = osMessageQueueNew(2, sizeof(jsonMessage_t), NULL);
	if (xQueueCommandParse == NULL) {
		printf("Command Service Queue Initialization Failed\n\r");
		Error_Handler();
	}


	for (;;)
	{
		/* wait for pure JSON string TODO: bug here FIXME::: */
		status = osMessageQueueGet(xQueueCommandParse, &msgType, NULL, osWaitForever);
		if (status == osOK) {
			printf("received: %s\n\r", msgType.json);
			jsonDecode(msgType.json, msgType.msg_size, false, 2); /* we start with tokcount 2 by default */
			/* we just got the command, time to parse it and execute what has to be executed */

		}

		osDelay(50);
	}
	osThreadTerminate(xCommandParserServiceTaskHnd);
}

/**
 * Command Interpreter routine
 * @param vParameter
 */
void vCommandInterpreterServiceTask(void *vParameter)
{
	osStatus_t status;
	jsmntok_t tokens[512];

	/* this queue is used to pass every token to the interpreter task */
	xQueueCommandInterpreter = osMessageQueueNew(2, sizeof(jsmntok_t) * 256, NULL);
	if (xQueueCommandParse == NULL) {
		printf("Command Interpreter  Queue Initialization Failed\n\r");
		Error_Handler();
	}

	for (;;)
	{
		status = osMessageQueueGet(xQueueCommandInterpreter, &tokens, 0U, osWaitForever);
		if (status == osOK) {
			printf("we got our tokens again, let's check");


		}

		osDelay(100);
	}
	osThreadTerminate(NULL);
}



/**
 * Main Command Service Initialization Routine
 * @return
 */
uint8_t uCmdParseServiceInit()
{

	/* creation of xCommandParserService Task */
	xCommandParserServiceTaskHnd = osThreadNew(vCommandParserServiceTask, NULL, &xCommandParserServiceTa_attributes);
	if (xCommandParserServiceTaskHnd == NULL) {
		printf("Command Service Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of xCommandInterpreterService Task */
	xCommandInterpreterServiceTaskHnd = osThreadNew(vCommandInterpreterServiceTask, NULL, &xCommandInterpreterServiceTa_attributes);
	if (xCommandInterpreterServiceTaskHnd == NULL) {
		printf("Command Interpreter Service Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing Command Parser Service... Success!\n\r");
	return EXIT_SUCCESS;
}

/**
 * Decodes JSON and starts parsing commands sent
 * @param json		The json *char
 * @param length	length of that json
 * @param reentry	false if we are coming for thez first time, true if this is a rentry business
 */
void jsonDecode(const char *json, uint16_t length, bool reentry, size_t tokcount)
{
	int r;
	jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * tokcount);
	if(!tokens) Error_Handler();

	/* number of tokens in the json file */
	r = jsmn_parse(&parser, json, length, tokens, tokcount);

	switch (r) {
	case JSMN_ERROR_INVAL:
		printf("bad token, JSON string is corrupted\n\r");
		break;

	case JSMN_ERROR_NOMEM:
		printf("not enough tokens, JSON string is too large. Trying to reallocate\n\r");
		vPortFree(tokens);
		jsonDecode(json, length, true, tokcount * 2); /* recursion in the same function with a realloc */
		break;

	case JSMN_ERROR_PART:
		printf("JSON string is too short, expecting more JSON data\n\r");
		break;

	default: /* PARSING OK, Now we need to check every commands, for this we will send it to a new thread */
		printf("JSON Parsing ok... processing command..\n\r");

		osMessageQueuePut(xQueueCommandInterpreter, &tokens,  0U, osWaitForever);
		/* we're done here */
		break;
	}

	/* keep this here! */
	vPortFree(tokens);
}



