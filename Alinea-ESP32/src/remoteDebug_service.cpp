/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-23 12:01:08
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-06 17:11:56
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
QueueHandle_t xQueueSerialServiceTX; /* extern */

/**
 * @brief  Remote Debug setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupRemoteDebug()
{
    Debug.setResetCmdEnabled(true); // Enable the reset command
    Debug.showProfiler(true);       // Profiler (Good to measure times, to optimize codes)
    Debug.showColors(false);        // Colors
    Debug.begin(THINGNAME);         // Initialize the WiFi server

    String helpCmd = "****\n\rinfo wifi - information about the Wifi connexion\n\r";
    helpCmd.concat("motors off - stops both motors\n\r");

    helpCmd.concat("****\n\r");

    Debug.setHelpProjectsCmds(helpCmd);
    Debug.setCallBackProjectCmds(&vProcessCmdRemoteDebug);

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

    if (lastCmd == "info wifi")
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

    if (lastCmd == "motors off")
    {
        cmdPackage_t cmd_pack;
        cmd_pack.cmd_type = CMD_TYPE_TEXT;
        strcpy(cmd_pack.txtCommand, lastCmd.c_str());

        if (xQueueSerialServiceTX != NULL)
        {
            debugI("Sending command: %s...", lastCmd.c_str());
            xQueueSend(xQueueSerialServiceTX, &cmd_pack, portMAX_DELAY); /* send directly to the serial TX service, the post office */
        }
        else
        {
            debugI("Failed sending command: %s...", lastCmd.c_str());
        }
    }
}