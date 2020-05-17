/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-23 12:01:08
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-17 13:14:49
 * @ Description:
 *******************************************************************************************/

#include "FreeRTOS.h"
#include "remoteDebug_service.h"
#include "autoconnect_service.h"
#include "configuration_esp32.h"
#include <stdlib.h>
#include "command_service.h"

WiFiClass WiFi;

void remoteDebug_task(void *parameter);
void vProcessCmdRemoteDebug();

xTaskHandle xRemoteDebuggerTask_hnd = NULL;

/**
 * @brief  Remote Debug setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupRemoteDebug()
{
    Debug.setResetCmdEnabled(true); // Enable the reset command
    Debug.showProfiler(false);      // Profiler (Good to measure times, to optimize codes)
    Debug.showColors(true);         // Colors
    Debug.begin(THINGNAME);         // Initialize the WiFi server

    String helpCmd = "****\n\rinfo wifi - information about the Wifi connexion\n\r";
    helpCmd.concat("ledstrip on/off - light up/off the front bottom leds\n\r");
    helpCmd.concat("motors off - stops both motors\n\r");

    helpCmd.concat("****\n\r");

    Debug.setHelpProjectsCmds(helpCmd);
    Debug.setCallBackProjectCmds(&vProcessCmdRemoteDebug); /* command call back everytime a command is received */

    /** FREERTOS Debug Task */
    xTaskCreate(
        remoteDebug_task,          /* Task function. */
        "remoteDebug_task",        /* String with name of task. */
        10000,                     /* Stack size in words. */
        NULL,                      /* Parameter passed as input of the task */
        5,                         /* Priority of the task. */
        &xRemoteDebuggerTask_hnd); /* Task handle. */

    if (&xRemoteDebuggerTask_hnd == NULL)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  Remote Debug main task
 * @note   
 * @retval 
 */
void remoteDebug_task(void *parameter)
{
    for (;;)
    {
        Debug.handle();
        vTaskDelay(10);
    }
    vTaskDelete(&xRemoteDebuggerTask_hnd);
}

/**
 * @brief  Custom Command callback
 * @note   
 * @retval None
 */
void vProcessCmdRemoteDebug()
{
    String lastCmd = Debug.getLastCommand();

    /* we prepare a cmd_pack for any command if needed, as we expect to be sending a command to the STM32 */
    command_package_t cmd_pack;
    cmd_pack.cmd_type = CMD_TYPE_JSON_TEXT; /* command comes from text telnet */
    cmd_pack.cmd_route = CMD_TRANSMIT;

    strcpy(cmd_pack.txtCommand, lastCmd.c_str());

    if (xQueueCommandParse != NULL)
    {
        //debugI("Sending command: %s...", lastCmd.c_str());
        xQueueSend(xQueueCommandParse, &cmd_pack, portMAX_DELAY); /* send to the command parser that will form the json and forward it to the postman */
    }
    else
    {
        debugI("Failed sending command: %s...", lastCmd.c_str());
    }
}
