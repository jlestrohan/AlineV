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
#include "printf.h"
#include "main.h"
#include <tiny-json.h>
#include <stdbool.h>
#include <string.h>
#include "MotorsControl_service.h"

/* struct containing the values once extracted from JSON to be used afterward by the command interpreter */
typedef struct {
	uint64_t uuid;			/* ESP32 uuid */
	uint64_t timestamp;		/* message timestamp */
	char type[5];			/* 3 letters type */
} cmdPackage_t;
cmdPackage_t *cmdPackage;

osMessageQueueId_t xQueueCommandParse;
UART_HandleTypeDef hlpuart1;

/* function definitions */
uint8_t _cmd_motors(char **tokens, uint8_t count);


/**
 * @brief  STM32 COMMANDS ARE TO BE ADDED HERE
 * @note
 * @retval None
 */
typedef struct
{
	char command[CMD_LINE_MAX_LENGTH];
	uint8_t (*commands_func)(char **tokens, uint8_t count);
} STM32_Commands_t;

STM32_Commands_t stm32_commands_list[] = {
		{"motors", _cmd_motors},
};


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
#ifdef DEBUG_ESP32_COMMAND_CHAIN
			printf("Command Center received: %.*s\n\r", msgType.msg_size, msgType.json);
#endif
			//uJsonDecode(msgType.json, msgType.msg_size);

		}

		osDelay(50);
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

	printf("Initializing Command Parser Service... Success!\n\r");
	return (EXIT_SUCCESS);
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
	if ( parent == NULL ) return (EXIT_FAILURE);

	/** UUID **/
	json_t const* uuid_topic = json_getProperty( parent, "uuid" );
	if ( uuid_topic == NULL ) return (EXIT_FAILURE);
	if ( json_getType( uuid_topic ) != JSON_INTEGER ) return (EXIT_FAILURE);

	int64_t uuid_value = json_getInteger( uuid_topic );
	printf( "uuid: %lld\n\r", uuid_value);
	cmdPackage->uuid = uuid_value;

	/** TIMESTAMP **/
	json_t const* timestamp_topic = json_getProperty( parent, "timestamp" );
	if ( timestamp_topic == NULL ) return (EXIT_FAILURE);
	if ( json_getType( timestamp_topic ) != JSON_INTEGER ) return (EXIT_FAILURE);

	int64_t timestamp_value = json_getInteger( timestamp_topic );
	printf( "timestamp: %lld\n\r", timestamp_value);
	cmdPackage->uuid = timestamp_value;

	/** TYPE **/
	json_t const* type_topic = json_getProperty( parent, "type" );
	if ( type_topic == NULL ) return (EXIT_FAILURE);
	if ( json_getType( type_topic ) != JSON_TEXT ) return (EXIT_FAILURE);

	char const *type_value = json_getValue( type_topic );
	printf( "type: %s\n\r", type_value);
	strcpy(cmdPackage->type, type_value);

	/** NESTED DATA **/
	json_t const* data_topic = json_getProperty( parent, "data" );
	if ( data_topic == NULL ) return (EXIT_FAILURE);
	if ( json_getType( data_topic ) != JSON_OBJ ) return (EXIT_FAILURE);

	//FIXME STUB
	//char **tokens = (char *[]) {"motors", "forward"};
	//uint8_t count = 2;

	//TODO: nested command: https://github.com/rafagafe/tiny-json
	/* we browse thru cEsp32commands and compare it with cmd
	   if that command is an ESP32 one we will treat that here */
	//size_t stmCmdAarray = sizeof(stm32_commands_list) / sizeof(stm32_commands_list[0]);

	/*for (uint8_t i = 0; i < stmCmdAarray; i++)
	{
		if (strcmp(tokens[0], stm32_commands_list[i].command) == 0)
		{
			//stm32_commands_list[i].commands_func(tokens, count);
			return EXIT_SUCCESS;
		}
	}*/

	//return EXIT_FAILURE;

	return (EXIT_SUCCESS);
}

/**************************************************** COMMANDS **************************************
 *
 */
uint8_t _cmd_motors(char **tokens, uint8_t count)
{
	MotorMotion_t motorMotion;

	if (count <=1) return (EXIT_FAILURE);

#ifdef DEBUG_ESP32_COMMAND_CHAIN
	printf("Executing motors %s STM32 commmand ", tokens[1]);
#endif

	if (tokens[1])
	{
		if (strcmp(tokens[1], "forward") == 0)
		{
			//motorMotion = MOTOR_MOTION_FORWARD;
			//osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
		}
		else if (strcmp(tokens[1], "idle") == 0)
		{
			//motorMotion = MOTOR_MOTION_IDLE;
			//osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
		}
		else if (strcmp(tokens[1], "backward") == 0)
		{
			//motorMotion = MOTOR_MOTION_BACKWARD;
			//osMessageQueuePut(xQueueMotorMotionOrder, &motorMotion, 0U, osWaitForever);
		}
	}
	return (EXIT_SUCCESS);
}
