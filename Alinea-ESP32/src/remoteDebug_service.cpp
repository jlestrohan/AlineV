/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-23 12:01:08
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 14:05:41
 * @ Description:
 *******************************************************************************************/

#include "remoteDebug_service.h"
#include "autoconnect_service.h"
#include "configuration_esp32.h"

void remoteDebug_task(void *parameter);

/**
 * @brief  Remote Debug setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupRemoteDebug()
{
    Debug.begin(THINGNAME); // Initialize the WiFi server

    /** FREERTOS Debug Task */
    xTaskCreate(
        remoteDebug_task,   /* Task function. */
        "remoteDebug_task", /* String with name of task. */
        10000,              /* Stack size in words. */
        NULL,               /* Parameter passed as input of the task */
        5,                  /* Priority of the task. */
        NULL);              /* Task handle. */
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
    vTaskDelete(NULL);
}