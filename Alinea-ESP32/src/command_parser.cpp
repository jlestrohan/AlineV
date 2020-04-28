/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:21
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 14:04:17
 * @ Description:
 *******************************************************************************************/

#include "command_parser.h"
#include "remoteDebug_service.h"
#include <stdlib.h>
#include <stdio.h>

xTaskHandle xCommandParserTask_hnd = NULL;
extern QueueHandle_t xQueueCommandParse;

/**
 * @brief  Receives some UART string and decides if it's trash or a real TAG message
 * @note   
 * @param  *parameter: 
 * @retval None
 */
void vCommandParserTaskCode(void *pvParameters)
{
    char buffer[CMD_TAG_MSG_MAX_LGTH];
    for (;;)
    {
        xQueueReceive(xQueueCommandParse, &buffer, portMAX_DELAY);
        debugI("%s", buffer);
        /* we process the command here ... */
        /* TODO: process the command */

        vTaskDelay(10);
    }
    vTaskDelete(NULL);
}

/**
 * @brief  Main initialization entry point
 * @note   
 * @retval None
 */
uint8_t setupCmdParser()
{
    /* we first initialize the queue that will handle all the incoming messages */
    xQueueCommandParse = xQueueCreate(10, sizeof(char) * CMD_TAG_MSG_MAX_LGTH);
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
        vQueueDelete(xQueueCommandParse);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*void cmd_parse(const char *command)
{
     if (strncmp(buffer, "tone1", 5) == 0)
    {
      wifiSuccessTune();
      debugI("tone received");
    } 
    else if (strncmp(buffer, "restart", 7) == 0)
    {
      debugI("%s", buffer);
      ESP.restart();
    }
    else
    {
    commandReceivedTune();
      debugI("%s", buffer);
    }
}*/