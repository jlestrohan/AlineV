/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-23 12:01:08
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-28 08:07:28
 * @ Description:
 *******************************************************************************************/

#include "FreeRTOS.h"
#include "remoteDebug_service.h"
#include "autoconnect_service.h"
#include "configuration_esp32.h"
#include <stdlib.h>
#include "command_service.h"

WiFiClass WiFi;

/* functions definitions */
static void remoteDebug_task(void *parameter);
static void vProcessCmdRemoteDebug();

static xTaskHandle xRemoteDebuggerTask_hnd;

/* --------------------------- MAIN SERVICE SETUP --------------------------- */
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
        4096,                      /* Stack size in words. */
        NULL,                      /* Parameter passed as input of the task */
        14,                        /* Priority of the task. */
        &xRemoteDebuggerTask_hnd); /* Task handle. */

    if (&xRemoteDebuggerTask_hnd == NULL)
    {
        debugE("Failed creating xRemoteDebuggerTask_hnd");
        Serial.println("Failed creating xRemoteDebuggerTask_hnd");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* ------------------------- MAIN REMOTE DEBUG TASK ------------------------- */
/**
 * @brief  Remote Debug main task
 * @note   
 * @retval 
 */
static void remoteDebug_task(void *parameter)
{
    for (;;)
    {
        Debug.handle();
        vTaskDelay(20);
    }
    vTaskDelete(NULL);
}

/* ------------------------- CUSTOM COMMAND CALLBACK ------------------------ */
/**
 * @brief  Custom Command callback
 * @note   
 * @retval None
 */
static void vProcessCmdRemoteDebug()
{
    /* will send the whole command to the command center to be interpreted */
    //FIXME: crash on single command here
    String lastCmd = Debug.getLastCommand();
    if (xQueueCommandParse != NULL)
    {
        debugD("Sending command: %s", lastCmd.c_str());
        xQueueSend(xQueueCommandParse, lastCmd.c_str(), portMAX_DELAY); /* send to the command parser that will form the json and forward it to the postman */
    }
    else
    {
        debugI("Failed sending command: %s", lastCmd.c_str());
    }
}
