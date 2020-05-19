/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:21
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 18:32:40
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
#include "AWS_service.h"

/* function definitions */
static uint8_t _cmd_status(char **tokens, uint8_t count);
static uint8_t _cmd_ledstrip(char **tokens, uint8_t count);
static uint8_t _cmd_help(char **tokens, uint8_t count);
//void vDecodeJsonCommand(command_package_t *cmd_pack);
uint8_t split(const char *txt, char delim, char ***tokens);
void vEncodeJsonCommand(char **tokens, uint8_t count, char *cmd_string, size_t length);
uint8_t uCheckESP32Command(char **tokens, uint8_t count);

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

/* ------------------------ MAIN COMMAND PARSER TASK ------------------------ */
/**
 * @brief  Receives some UART string and decides if it's trash or a real TAG message
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vCommandParserTaskCode(void *pvParameters)
{
  char **tokens;
  uint8_t count;
  char command_string[CMD_LINE_MAX_LENGTH];

  for (;;)
  {

    xQueueReceive(xQueueCommandParse, &command_string, portMAX_DELAY);

    size_t len = strlen(command_string);
    debugI("Queue xQueueCommandParse received command: %.*s with size %d", len, command_string, len);
    count = split(command_string, ' ', &tokens);

    if (uCheckESP32Command(tokens, count) == EXIT_FAILURE)
    {
      /* command ips for the STM32 */
      debugD("Command is for the STM32");
      vEncodeJsonCommand(tokens, count, command_string, len);
    }

    /* freeing tokens */
    for (uint8_t i = 0; i < count; i++)
      free(tokens[i]);
    free(tokens);

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

/* --------------------------- MAIN SERVICE SETUP --------------------------- */
/**
 * @brief  Main initialization entry point
 * @note   
 * @retval None
 */
uint8_t uSetupCmdParser()
{
  xQueueCommandParse = xQueueCreate(20, sizeof(char) * CMD_LINE_MAX_LENGTH);
  if (xQueueCommandParse == NULL)
  {
    debugE("xQueueCommandParse queue ... error");
    vTaskDelete(NULL);
  }
  else
  {
    debugD("creating xQueueCommandParse queue ... Success...");
  }

  /* let's create the parser task first */
  xTaskCreate(
      vCommandParserTaskCode,   /* Task function. */
      "vCommandParserTaskCode", /* String with name of task. */
      10000,                    /* Stack size in words. */
      NULL,                     /* Parameter passed as input of the task */
      7,                        /* Priority of the task. */
      &xCommandParserTask_hnd); /* Task handle. */

  if (xCommandParserTask_hnd == NULL)
  {
    debugE("Error creating serial parser task!");
    /* cannot create task, remove all created stuff and exit failure */
    vQueueDelete(xQueueCommandParse);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
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
uint8_t split(const char *text, char delimiter, char ***tkns)
{
  int *tklen, *t, count = 1;
  char **array, *p = (char *)text;

  while (*p != '\0')
    if (*p++ == delimiter)
      count += 1;
  t = tklen = (int *)calloc(count, sizeof(int));
  for (p = (char *)text; *p != '\0'; p++)
    *p == delimiter ? *t++ : (*t)++;
  *tkns = array = (char **)malloc(count * sizeof(char *));
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
  free(tklen);
  return count;
}

/* ------------------------------- ENCODE JSON ------------------------------ */
/**
 * @brief  JSON Encoding service here. We format the final package of orders to the STM32
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval None
 */
void vEncodeJsonCommand(char **tokens, uint8_t count, char *cmd_string, size_t length)
{

  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3);
  DynamicJsonDocument doc(capacity + 100);

  doc["uuid"] = ESP.getEfuseMac();
  doc["timestamp"] = ulGetEpochTime();
  doc["type"] = "CMD";

  JsonObject data = doc.createNestedObject("data");
  data["command"] = tokens[0]; /* first token is the command " */
  JsonArray data_arguments = data.createNestedArray("args");

  /* rest of tokens are arguments */
  for (uint8_t i = 1; i < count; i++)
  {
    data_arguments.add(tokens[i]);
  }

  char output[capacity + 50];
  uint8_t len = serializeJson(doc, output);

  /* let's send that json document direct to the Serial Service for immediate sending */
  jsonMessage_t jsonMsg;
  jsonMsg.msg_size = len;
  memcpy(jsonMsg.json, output, len);

  if (xQueueSerialServiceTX != NULL)
  {
    debugD("Sending JSON to TX: %.*s", len, jsonMsg.json);
    xQueueSend(xQueueSerialServiceTX, &jsonMsg, portMAX_DELAY); /* send directly to the serial TX service, the post office */
  }
  else
  {
    debugE("xQueueSerialServiceTX Queue not available... Aborted!..");
  }
}

/* ------------------------- CHECK IF ESP32 COMMAND ------------------------- */
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
  /* we browse thru cEsp32commands and compare it with cmd 
   if that command is an ESP32 one we will treat that here */
  size_t espCmdAarray = sizeof(esp32_commands_list) / sizeof(esp32_commands_list[0]);

  for (uint8_t i = 0; i < espCmdAarray; i++)
  {
    if (strcmp(tokens[0], esp32_commands_list[i].command) == 0)
    {
      esp32_commands_list[i].commands_func(tokens, count);
      return EXIT_SUCCESS;
    }
  }
  return EXIT_FAILURE;
}

/* ------------------------------ HELP COMMAND ------------------------------ */
/**
 * @brief  HELP Command - Displays a Help console for all the commands available (include STM32 ones too)
 * @note   
 * @param  **tokens: 
 * @param  count: 
 * @retval 
 */
static uint8_t _cmd_help(char **tokens, uint8_t count)
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
static uint8_t _cmd_status(char **tokens, uint8_t count)
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
static uint8_t _cmd_ledstrip(char **tokens, uint8_t count)
{
  if (count <= 1)
    return EXIT_FAILURE;

  debugD("Executing ledstrip %s ESP32 commmand... ", tokens[1]);

  lit_status_t ledstatus;
  if (tokens[1])
  {
    if (strcmp(tokens[1], "on") == 0)
    {
      ledstatus.is_lit = true;
      if (xLedStripCommandQueue)
        xQueueSend(xLedStripCommandQueue, &ledstatus, portMAX_DELAY);
      vPlayMelody(MelodyType_CommandReceived); /* command received tune */
    }
    else if (strcmp(tokens[1], "off") == 0)
    {
      ledstatus.is_lit = false;
      if (xLedStripCommandQueue)
        xQueueSend(xLedStripCommandQueue, &ledstatus, portMAX_DELAY);
      vPlayMelody(MelodyType_CommandReceived); /* command received tune */
    }
    else
    {
      debugE("Command argument not valid\n\r");
      vPlayMelody(MelodyType_WrongArgument); /* command received tune */
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
/*uint8_t uDecodeJsonCommand(command_package_t *cmd_pack)
{
  /* Deserializes STM JSON */
/*StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, cmd_pack->txtCommand);

  if (error)
  {
    debugE("deserializeJson() failed with error: %s", error.c_str());
    return EXIT_FAILURE;
  }

  /* check if CMD = a data kind */

/* check if STM UUID is the right one - check configuration_esp32.h*/
/* if (doc["uuid"] != STM32_UUID)
  {
    debugE("Wrong STM32 uuid, operation not permitted, no data will be processed sorry!");
    return EXIT_FAILURE;
  }

  cmd2aws_package_t aws_pack;
  /* sends the deserialized json (which is now a struct) to the AWS service */
/* we will then rebuild a conformed JSON with the right information */
/* and that service will then directly send the package to the AWS IoT service */
/* the ESP32 only acts as a relay/adds information to the incoming data from the STM*/
/* Any emitter decides when to send the data */
/*strcpy(aws_pack.stm_uuid, doc["uuid"]);
  strcpy(aws_pack.cmd_type, doc["command_type"]);

  JsonArray arr = doc["data"].as<JsonArray>();
  int count = arr.size();

  printf("array has count: %d\n\r", count);
  /* we are done for this service */

/* switch */

//debugI("Free HEAP: %lu on HEAP TOTAL: %lu", ESP.getFreeHeap(), ESP.getHeapSize());
//debugI("Free PS Ram: %lu", ESP.getFreePsram());

/* 1 we decode JSON */
/* 2 we add everything */

//return EXIT_SUCCESS;
//}