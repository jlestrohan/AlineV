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
#include "tiny-json.h"

#include <stdbool.h>
#include <string.h>

/* struct containing the values once extracted from JSON to be used afterward by the command interpreter */
typedef struct {
	uint64_t uuid;			/* ESP32 uuid */
	uint64_t timestamp;		/* message timestamp */
	char type[5];			/* 3 letters type */
} cmdPackage_t;
cmdPackage_t *cmdPackage;

osMessageQueueId_t xQueueCommandParse;
static osMessageQueueId_t xQueueCommandInterpreter;
UART_HandleTypeDef hlpuart1;

/* command parser (json) task*/
static osThreadId_t xCommandParserServiceTaskHnd;
static osStaticThreadDef_t xCommandParserServiceTaControlBlock;
static uint32_t xCommandParserServiceTaBuffer[1024];
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
static uint32_t xCommandInterpreterServiceTaBuffer[256];
static const osThreadAttr_t xCommandInterpreterServiceTa_attributes = {
		.name = "xCommandInterpreterServiceTask",
		.stack_mem = &xCommandInterpreterServiceTaBuffer[0],
		.stack_size = sizeof(xCommandInterpreterServiceTaBuffer),
		.cb_mem = &xCommandInterpreterServiceTaControlBlock,
		.cb_size = sizeof(xCommandInterpreterServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_CMD_INTERP_SERVICE };

/* function prototypes */
uint8_t uJsonDecode(uint8_t *json, uint16_t length);

/**
 * Main Task routine
 * @param vParameter
 */
void vCommandParserServiceTask(void *vParameter)
{
	printf("Starting Command Parser Service task...\n\r");

	osStatus_t status;
	jsonMessage_t msgType;

	xQueueCommandParse = osMessageQueueNew(2, sizeof(jsonMessage_t), NULL);
	if (xQueueCommandParse == NULL) {
		printf("Command Service Queue Initialization Failed\n\r");
		Error_Handler();
	}

	for (;;)
	{
		/* wait for pure JSON string TODO: bug here FIXME::: */
		status = osMessageQueueGet(xQueueCommandParse, &msgType, 0U, osWaitForever);
		if (status == osOK) {
			printf("Command Center received: %.*s\n\r", msgType.msg_size, msgType.json);
			uJsonDecode(msgType.json, msgType.msg_size); /* we start with tokcount 2 by default */

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
	printf("Starting Command Interpreter Service task...\n\r");

	//osStatus_t status;

	/* this queue is used to pass every token to the interpreter task */
	xQueueCommandInterpreter = osMessageQueueNew(2, sizeof(uint8_t), NULL);
	if (xQueueCommandParse == NULL) {
		printf("Command Interpreter  Queue Initialization Failed\n\r");
		Error_Handler();
	}

	for (;;)
	{
		//status = osMessageQueueGet(xQueueCommandInterpreter, &tokens, 0U, osWaitForever);
		//if (status == osOK) {
		//printf("we got our tokens again, let's check");


		//}

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
	/*xCommandInterpreterServiceTaskHnd = osThreadNew(vCommandInterpreterServiceTask, NULL, &xCommandInterpreterServiceTa_attributes);
	if (xCommandInterpreterServiceTaskHnd == NULL) {
		printf("Command Interpreter Service Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}*/

	printf("Initializing Command Parser Service... Success!\n\r");
	return EXIT_SUCCESS;
}


/**
 * Decodes JSON and starts parsing commands sent
 * @param json		The json *char
 * @param length	length of that json
 */
uint8_t uJsonDecode(uint8_t *json, uint16_t length)
{
	//https://github.com/rafagafe/tiny-json

	json_t pool[128];
	json_t const* parent = json_create( (char *)json, pool, 128 );
	if ( parent == NULL ) return EXIT_FAILURE;

	/** UUID **/
	json_t const* uuid_topic = json_getProperty( parent, "uuid" );
	if ( uuid_topic == NULL ) return EXIT_FAILURE;
	if ( json_getType( uuid_topic ) != JSON_INTEGER ) return EXIT_FAILURE;

	int64_t uuid_value = json_getInteger( uuid_topic );
	printf( "uuid: %lld\n\r", uuid_value);
	cmdPackage->uuid = uuid_value;

	/** TIMESTAMP **/
	json_t const* timestamp_topic = json_getProperty( parent, "timestamp" );
	if ( timestamp_topic == NULL ) return EXIT_FAILURE;
	if ( json_getType( timestamp_topic ) != JSON_INTEGER ) return EXIT_FAILURE;

	int64_t timestamp_value = json_getInteger( timestamp_topic );
	printf( "timestamp: %lld\n\r", timestamp_value);
	cmdPackage->uuid = timestamp_value;

	/** TYPE **/
	json_t const* type_topic = json_getProperty( parent, "type" );
	if ( type_topic == NULL ) return EXIT_FAILURE;
	if ( json_getType( type_topic ) != JSON_TEXT ) return EXIT_FAILURE;

	char const *type_value = json_getValue( type_topic );
	printf( "type: %s\n\r", type_value);
	strcpy(cmdPackage->type, type_value);

	/** NESTED DATA **/
	json_t const* data_topic = json_getProperty( parent, "data" );
	if ( data_topic == NULL ) return EXIT_FAILURE;
	if ( json_getType( data_topic ) != JSON_OBJ ) return EXIT_FAILURE;


	//TODO: nested command: https://github.com/rafagafe/tiny-json

	printf("done!");
	//osMessageQueuePut(xQueueCommandInterpreter, &tokens,  0U, osWaitForever);
	/* we're done here */

	return EXIT_SUCCESS;
}



