/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:21
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-08 09:34:43
 * @ Description: Parse any command received from  a consumer and take the appropriate action
 *******************************************************************************************/

// https://arduinojson.org/v6/assistant/

#include "command_service.h"
#include "remoteDebug_service.h"
#include <stdlib.h>
#include <stdio.h>
#include "buzzer_service.h"
#include <ArduinoJson.h>
#include "stm32Serial_service.h"

xTaskHandle xCommandParserTask_hnd = NULL;
QueueHandle_t xQueueCommandParse;
QueueHandle_t xQueueSerialServiceTX; /* extern */

/* functions definitiojns */
void cmd_process(command_package_t *command_pack);
int split(const char *txt, char delim, char ***tokens);
void encodeJsonCommand(char **tokens, uint8_t count);

/**
 * @brief  Receives some UART string and decides if it's trash or a real TAG message
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vCommandParserTaskCode(void *pvParameters)
{
  command_package_t commandPack;

  for (;;)
  {
    xQueueReceive(xQueueCommandParse, &commandPack, portMAX_DELAY);
    /* we process the command here ... */
    cmd_process(&commandPack);
    vPlayMelody(MelodyType_CommandReceived);

    /* at this stage the task is done with treating an incoming command */
    vTaskDelay(10);
  }
  vTaskDelete(xCommandParserTask_hnd);
}

/**
 * @brief  Main initialization entry point
 * @note   
 * @retval None
 */
uint8_t uSetupCmdParser()
{
  /* we first initialize the queue that will handle all the incoming messages */
  xQueueCommandParse = xQueueCreate(20, sizeof(command_package_t));
  if (xQueueCommandParse == NULL)
  {
    debugE("error creating the xQueueCommandParse queue");
    return EXIT_FAILURE;
  }

  /* let's create the parser task first */
  xTaskCreate(
      vCommandParserTaskCode,   /* Task function. */
      "vCommandParserTaskCode", /* String with name of task. */
      10000,                    /* Stack size in words. */
      NULL,                     /* Parameter passed as input of the task */
      1,                        /* Priority of the task. */
      &xCommandParserTask_hnd); /* Task handle. */

  if (xCommandParserTask_hnd == NULL)
  {
    debugE("Error creating serial parser task!");
    /* cannot create task, remove all created stuff and exit failure */
    //vQueueDelete(xQueueCommandParse);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 * @brief  Processes the command received
 * @note   
 * @param  *command_pack: 
 * @retval None
 */
void cmd_process(command_package_t *command_pack)
{
  /* is it a command to send the STM32 or execute on the esp ? */
  switch (command_pack->cmd_route)
  {
  case CMD_TRANSMIT: /* the command has to be packaged and sent over to STM32 */
    /* the command has to be transmitted, we just got a text that we have to actually parse in order to make that a real JSON command, let's start that */
    char **tokens;
    uint8_t count;
    count = split(command_pack->txtCommand, ' ', &tokens);
    encodeJsonCommand(tokens, count);

    for (uint8_t i = 0; i < count; i++) /* freeing tokens */
      free(tokens[i]);
    free(tokens);

    break;

  case CMD_RECEIVED: /* the command has been received to be executed on the ESP32 */

    break;
  }
}

/**
 * @brief  Generic function to split a string
 * @note   
 * @param  *txt: 
 * @param  delim: 
 * @param  ***tokens: 
 * @retval 
 */
int split(const char *txt, char delim, char ***tokens)
{
  int *tklen, *t, count = 1;
  char **arr, *p = (char *)txt;

  while (*p != '\0')
    if (*p++ == delim)
      count += 1;
  t = tklen = (int *)calloc(count, sizeof(int));
  for (p = (char *)txt; *p != '\0'; p++)
    *p == delim ? *t++ : (*t)++;
  *tokens = arr = (char **)malloc(count * sizeof(char *));
  t = tklen;
  p = *arr++ = (char *)calloc(*(t++) + 1, sizeof(char *));
  while (*txt != '\0')
  {
    if (*txt == delim)
    {
      p = *arr++ = (char *)calloc(*(t++) + 1, sizeof(char *));
      txt++;
    }
    else
      *p++ = *txt++;
  }
  free(tklen);
  return count;
}

void encodeJsonCommand(char **tokens, uint8_t count)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3);
  DynamicJsonDocument doc(capacity + 100);

  doc["timestamp"] = 12312312;
  doc["type"] = "CMD";

  JsonObject data = doc.createNestedObject("data");
  data["command"] = tokens[0]; /* first token is the command " */
  JsonArray data_arguments = data.createNestedArray("args");

  /* rest of tokens are arguments */
  for (uint8_t i = 1; i < count; i++)
  {
    data_arguments.add(tokens[i]);
  }
  char output[capacity];
  serializeJson(doc, output);

  /* let's send that json document direct to the Serial Service for immediate sending */
  jsonMessage_t jsonMsg;
  jsonMsg.msg_size = capacity;
  strcpy(jsonMsg.json, output);
  xQueueSend(xQueueSerialServiceTX, &jsonMsg, portMAX_DELAY); /* send directly to the serial TX service, the post office */
}