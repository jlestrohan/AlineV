/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:21
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-18 14:46:17
 * @ Description: Parse any command received from  a consumer and take the appropriate action
 
 If you're willing to use this code, no problem at all please feel free to do it... but please...
 
      RESPECT THE AUTHOR... Mention that you're using someone else's work and pay them the honor
      they deserve.... 
      
        MENTION THEM (me)... Have fun :)
      
      (Not targetting at anyone in particular, unless someone feels targetted...)
    
 *******************************************************************************************/

// https://arduinojson.org/v6/assistant/

#include "FreeRTOS.h"
#include "command_service.h"
#include "ledstrip_service.h"
#include "remoteDebug_service.h"
#include <stdlib.h>
#include <stdio.h>
#include "buzzer_service.h"
#include <ArduinoJson.h>
#include "stm32Serial_service.h"
#include "ntp_service.h"
#include "configuration_esp32.h"

/* function definitions */
uint8_t _cmd_status(char **tokens, uint8_t count);
uint8_t _cmd_ledstrip(char **tokens, uint8_t count);
uint8_t _cmd_help(char **tokens, uint8_t count);
void vDecodeJsonCommand(command_package_t *cmd_pack);
void vEncodeJsonCommand(char **tokens, uint8_t count, command_type_t type);

/** 
 * @brief  ESP32 COMMANDS ARE TO BE ADDED HERE 
 * @note   
 * @retval None
 */
typedef struct
{
  char command[CMD_LINE_MAX_LENGTH];
  uint8_t (*commands_func)(char **tokens, uint8_t count);
} ESP32_Commands_t;

ESP32_Commands_t esp32_commands_list[] = {
    {"ledstrip", _cmd_ledstrip},
    {"status", _cmd_status},
    {"help", _cmd_help}};

xTaskHandle xCommandParserTask_hnd = NULL;
QueueHandle_t xQueueCommandParse;
QueueHandle_t xQueueSerialServiceTX; /* extern */

/* functions definitiojns */
void cmd_process(command_package_t *command_pack);
int split(const char *txt, char delim, char ***tokens);
void encodeJsonCommand(char **tokens, uint8_t count, command_type_t type);
uint8_t uDecodeJsonCommand(command_package_t *cmd_pack);
void vCmdtype_text(command_type_t type, char *buffer);
uint8_t uCheckESP32Command(char **tokens, uint8_t count);

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
    /* this is a standard encapsulated command package, can be incoming from either 
    the jsonDecoder service, the remote Debugger or any other source.
    This service will decide what to do with the package */
    cmd_process(&commandPack); /*(command_package_t)*/

    //vPlayMelody(MelodyType_CommandReceived); /* command received tune */44

    /* at this stage the task is done with treating an incoming command */
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
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
  /* we first split that command contained in the received package */
  char **tokens;
  uint8_t count;

  /* is it a command to send the STM32 or execute on the esp ? */
  switch (command_pack->cmd_route)
  {
  case PKT_TRANSMIT: /* the command has to be packaged to JSON and sent over to STM32 */
    count = split(command_pack->txtCommand, ' ', &tokens);
    /* the command has to be transmitted, we just got a text that we have to actually parse in order to make that a real JSON command, let's start that */
    /* whom to transmit the command ? ESP32 or STM32 ? */
    if (!uCheckESP32Command(tokens, count))
    {
      vEncodeJsonCommand(tokens, count, command_pack->cmd_type);
    }

    for (uint8_t i = 0; i < count; i++) /* freeing tokens */
      free(tokens[i]);
    free(tokens);
    break;

  case PKT_RECEIVED: /* the packet has been received and we have to decode it and take a further action...*/
                     /* the packet is a json received from the stm32 */
    //debugI("COMMAND ROUTINE: %s", command_pack->txtCommand);

    /* we will decode the STM json to check if the message is a DATA of a COMMAND */
    /* if it is a data, we send the stuff to AWS service to add a few more things,
    recreate the JSON, packages it into an AWS data pack. */
    uDecodeJsonCommand(command_pack);
    break;

  default:
    break;
  }
}

/* --------------------- SPLIT STRING UTILITY FUNCTIONS --------------------- */
/**
 * @brief  Generic function to split a string
 * @note   
 * @param  *txt: 
 * @param  delim: 
 * @param  ***tokens: 
 * @retval 
 */
int split(const char *text, char delimiter, char ***tkns)
{
  int *tklen, *t, count = 1;
  char **array, *p = (char *)text;

  while (*p != '\0')
    if (*p++ == delimiter)
      count += 1;
  t = tklen = (int *)calloc(count, sizeof(int));
  for (p = (char *)text; *p != '\0'; p++)
    *p == delimiter ? *t++ : (*t)++;
  *tkns = array = (char **)pvPortMalloc(count * sizeof(char *));
  t = tklen;
  p = *array++ = (char *)calloc(*(t++) + 1, sizeof(char *));
  while (*text != '\0')
  {
    if (*text == delimiter)
    {
      p = *array++ = (char *)calloc(*(t++) + 1, sizeof(char *));
      text++;
    }
    else
      *p++ = *text++;
  }
  vPortFree(tklen);
  return count;
}

/**
 * @brief  JSON Encoding service here. We format the final package of orders to the STM32
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval None
 */
void vEncodeJsonCommand(char **tokens, uint8_t count, command_type_t type)
{
  char type_buf[4]; /* stores a 3 chars type of message */
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3);
  DynamicJsonDocument doc(capacity + 100);

  doc["uuid"] = ESP.getEfuseMac();
  doc["timestamp"] = ulGetEpochTime();
  vCmdtype_text(type, type_buf);
  doc["type"] = type_buf;

  JsonObject data = doc.createNestedObject("data");
  data["command"] = tokens[0]; /* first token is the command " */
  JsonArray data_arguments = data.createNestedArray("args");

  /* rest of tokens are arguments */
  for (uint8_t i = 1; i < count; i++)
  {
    data_arguments.add(tokens[i]);
  }

  char output[capacity + 50];
  uint8_t length = serializeJson(doc, output);

  /* let's send that json document direct to the Serial Service for immediate sending */
  jsonMessage_t jsonMsg;
  jsonMsg.msg_size = length;
  memcpy(jsonMsg.json, output, length);
  xQueueSend(xQueueSerialServiceTX, &jsonMsg, portMAX_DELAY); /* send directly to the serial TX service, the post office */
}

void vCmdtype_text(command_type_t type, char *buffer)
{
  switch (type)
  {
  case CMD_TYPE_JSON_CMD:
    strcpy(buffer, "CMD");
    break;
  case CMD_TYPE_JSON_SYN:
    strcpy(buffer, "SYN");
    break;
  case CMD_TYPE_JSON_ACK: /* acknowledge from STM32 */
    strcpy(buffer, "ACK");
  case CMD_TYPE_JSON_TEXT: /* command coming from text telnet */
    strcpy(buffer, "TXT");
    break;
  default:
    break;
  }
}

/* -------- CHECK IF ESP32 COMMAND (Or must be sent ovber by airlone) ------- */
/**
 * @brief   This function checks if the entered first command (first word of the command string) belongs to the ESP32
            or the STM32. 
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval 
 */
uint8_t uCheckESP32Command(char **tokens, uint8_t count)
{
  /* we browse thru cEsp32commands and compare it with cmd */
  /* if that command is an ESP32 one we will treat that here */
  size_t espCmdAarray = sizeof(esp32_commands_list) / sizeof(esp32_commands_list[0]);

  //debugI(" %s\n", cEsp32commands[0]);
  for (uint8_t i = 0; i < espCmdAarray; i++)
  {
    if (strcmp(tokens[0], esp32_commands_list[i].command) == 0)
    {
      esp32_commands_list[i].commands_func(tokens, count);
      return EXIT_FAILURE; /* command found it's an esp32 command */
    }
  }
  return EXIT_SUCCESS;
}

/* ------------------------------ HELP COMMAND ------------------------------ */
/**
 * @brief  HELP Command - Displays a Help console for all the commands available (include STM32 ones too)
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval 
 */
uint8_t _cmd_help(char **tokens, uint8_t count)
{
  debugI("---------------- HELP -----------------------------------------\n\r");
  debugD("Commands available: \n\n\r");
  debugD("ESP32 Commands -----------------\n\r");
  debugD("\t\tledstrip on/off: lights up/off the front leds\n\r");
  debugD("\t\tstatus: wifi(wifi status)\n\n");
  debugD("----------------------------------------------------------------\n\r");

  return EXIT_SUCCESS;
}

/* ----------------------------- STATUS COMMAND ----------------------------- */
/**
 * @brief  STATUS command
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval 
 */
uint8_t _cmd_status(char **tokens, uint8_t count)
{
  if (tokens[1])
  {
    if (strcmp(tokens[1], "wifi") == 0)
    {
      debugI("---------------- WIFI INFO -------------------------------------");
      debugI("Connected to WiFi AP: %s", WiFi.SSID().c_str());
      debugI("IP: %s", WiFi.localIP().toString().c_str());
      debugI("IPv6: %s", WiFi.localIPv6().toString().c_str());
      debugI("MAC: %s", WiFi.macAddress().c_str());
      debugI("RSSI: %d%%", (uint8_t)abs(WiFi.RSSI()));
      debugI("Hostname: %s", WiFi.getHostname());
      debugI("----------------------------------------------------------------");
    }
    else
    {
      debugE("Command argument not valid... \n\r");
    }
  }
  return EXIT_SUCCESS;
}

/* ---------------------------- LEDSTRIP COMMAND ---------------------------- */
/**
 * @brief  LEDSTRIP command
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval 
 */
uint8_t _cmd_ledstrip(char **tokens, uint8_t count)
{
  lit_status_t ledstatus;
  if (tokens[1])
  {
    if (strcmp(tokens[1], "on") == 0)
    {
      ledstatus.is_lit = true;
      if (xLedStripCommandQueue)
        xQueueSend(xLedStripCommandQueue, &ledstatus, portMAX_DELAY);
    }
    else if (strcmp(tokens[1], "off") == 0)
    {
      ledstatus.is_lit = false;
      if (xLedStripCommandQueue)
        xQueueSend(xLedStripCommandQueue, &ledstatus, portMAX_DELAY);
    }
    else
    {
      debugE("Command argument not valid\n\r");
    }
  }
  return EXIT_SUCCESS;
}

/* --------------------------- DECODE JSON COMMAND FROM STM32-------------------------- */
/**
 * @brief  Data or Command to ESP32 ?
 * @note   
 * @param  *cmd_pack: 
 * @retval None
 */
uint8_t uDecodeJsonCommand(command_package_t *cmd_pack)
{
  /* Deserializes STM JSON */
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, cmd_pack->txtCommand);

  if (error)
  {
    debugE("deserializeJson() failed with error: %s", error.c_str());
    return EXIT_FAILURE;
  }

  /* check if CMD = a data kind */

  /* check if STM UUID is the right one - check configuration_esp32.h*/
  if (doc["uuid"] != STM32_UUID)
  {
    debugE("Wrong STM32 uuid, operation not permitted, no data will be processed sorry!");
    return EXIT_FAILURE;
  }

  /* sends the deserialized json (which is now a struct) to the AWS service */

  /* we are done for this service */

  /* switch */
  debugI("RECEIVED: %.*s for %d bytes", cmd_pack->command_size, cmd_pack->txtCommand, cmd_pack->command_size);
  //debugI("Free HEAP: %lu on HEAP TOTAL: %lu", ESP.getFreeHeap(), ESP.getHeapSize());
  //debugI("Free PS Ram: %lu", ESP.getFreePsram());

  /* 1 we decode JSON */
  /* 2 we add everything */

  return EXIT_SUCCESS;
}