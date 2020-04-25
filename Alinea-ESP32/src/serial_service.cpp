/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-25 18:04:12
 * @ Description:
 *******************************************************************************************/

#include "serial_service.h"
#include <stdint.h>
#include "remoteDebug_service.h"
#include "SHA256.h"

#define CMD_TAG_MSG_MAX_LGTH 255 /* max length of a full tagged message */

QueueHandle_t serial2parser_queue;

static uint8_t tag_cmd_cb(char *params[]);
static uint8_t tag_dta_cb(char *params[]);
static uint8_t tag_syn_cb(char *params[]);
static uint8_t tag_ack_cb(char *params[]);
static uint8_t tag_rst_cb(char *params[]);
static uint8_t tag_err_cb(char *params[]);

/**
 * @brief  Sent tag struct, holds a record of anything that has been sent over already via uart
 * @note   
 * @retval None
 */
typedef struct
{
  uint8_t msg_num;   /* message number, increments ++ every time */
  char sha2sig[255]; /* sha2 footprint */
  uint16_t timeout;  /* timeout after which the command is removed from the list and considered lost */

} cmd_need_feedback_t;
/* list of max 50 commands that have been sent and require fb */
/* Once feedback is received, we can delete these from the list */
cmd_need_feedback_t cmd_sent_list[50];

/**
 * @brief  Command Tag type structure
 * @note   
 * @retval None
 */
typedef struct
{
  char name[CMD_TAG_MSG_MAX_LGTH];  /* litteral of the beginning of the command */
  uint8_t (*tag_cb)(char **params); /* callback for that command/data */
  uint8_t need_param;               /* must include data between tags (implies closing tag) */
  uint8_t need_hash;                /* must include SHA2 hash in first tag */
} cmd_parser_tag_t;
cmd_parser_tag_t cmd_parser_tag_list[] = {
    {"[CMD:", tag_cmd_cb, 1, 1},
    {"[DTA:", tag_dta_cb, 1, 1},
    {"[SYN]", tag_syn_cb, 0, 0},
    {"[ACK]", tag_ack_cb, 0, 0},
    {"[RST]", tag_rst_cb, 0, 0},
    {"[ERR:", tag_err_cb, 0, 1}};

/**
 * @brief  Command structure inside [CMD]tags
 * @note   
 * @retval None
 */
typedef struct
{

} cmd_parser_command_t;

/**
 * @brief  Serial Listener task
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void serialListener_task(void *parameter)
{
  char uartBuffer[CMD_TAG_MSG_MAX_LGTH + 1];
  int uartBufferPos = 0;

  for (;;)
  {
    if (Serial.available())
    {
      char ch = (char)Serial.read();
      if (ch == '\n') // is this the terminating carriage return
      {
        uartBuffer[uartBufferPos] = 0; // terminate the string with a 0
        uartBufferPos = 0;             // reset the index ready for another string
        /* todo: here we compare uartBuffer with cmd_parser_tag_list[] to check if the beginning of the command is recognized */
        /* send it thru a msgQueue to another dedicated task */
        xQueueSend(serial2parser_queue, &uartBuffer, portMAX_DELAY);
        //debugI("%s", uartBuffer);

        uartBuffer[0] = '\0';
      }
      else
      {
        if (uartBufferPos < CMD_TAG_MSG_MAX_LGTH)
        {
          uartBuffer[uartBufferPos++] = ch; // add the character into the buffer
        }
        else
        {
          /* message too long we trash it */
          debugE("Serial received message is too long - dumped!");
          uartBuffer[uartBufferPos] = 0; // terminate the string with a 0
          uartBufferPos = 0;             // reset the index ready for another string
          uartBuffer[0] = '\0';
        }
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

/**
 * @brief  Receives some UART string and decides if it's trash or a real TAG message
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void serialParser_task(void *parameter)
{
  char buffer[CMD_TAG_MSG_MAX_LGTH + 1];

  for (;;)
  {
    xQueueReceive(serial2parser_queue, &buffer, portMAX_DELAY);
    debugI("%s", buffer);
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

/**
 * @brief  Main Serial Listener setup
 * @note   
 * @retval None
 */
void setupUARTListener()
{
  xTaskHandle *serialListener_hndl;
  xTaskHandle *serialParser_hndl;

  /** FREERTOS OTA Task */
  // todo: check if tasks are created without error */
  xTaskCreate(
      serialListener_task,   /* Task function. */
      "serialListener_task", /* String with name of task. */
      10000,                 /* Stack size in words. */
      NULL,                  /* Parameter passed as input of the task */
      2,                     /* Priority of the task. */
      serialListener_hndl);  /* Task handle. */

  if (!serialListener_hndl)
    debugE("Error creating serial listener task!");

  serial2parser_queue = xQueueCreate(10, sizeof(char) * CMD_TAG_MSG_MAX_LGTH + 1);
  if (!serial2parser_queue)
  {
    debugE("error creatring the serial2data queue");
  }
  else
  {
    xTaskCreate(
        serialParser_task,   /* Task function. */
        "serialParser_task", /* String with name of task. */
        10000,               /* Stack size in words. */
        NULL,                /* Parameter passed as input of the task */
        1,                   /* Priority of the task. */
        serialParser_hndl);  /* Task handle. */

    if (!serialParser_hndl)
      debugE("Error creating serial parser task!");
  }
}

static uint8_t tag_cmd_cb(char *params[])
{

  return 0;
}

static uint8_t tag_dta_cb(char *params[])
{

  return 0;
}

static uint8_t tag_syn_cb(char *params[])
{

  return 0;
}

static uint8_t tag_ack_cb(char *params[])
{

  return 0;
}

static uint8_t tag_rst_cb(char *params[])
{

  return 0;
}

static uint8_t tag_err_cb(char *params[])
{

  return 0;
}