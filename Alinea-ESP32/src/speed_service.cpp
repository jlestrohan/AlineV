/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-09 10:35:09
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-11 20:19:51
 * @ Description:
 *******************************************************************************************/

#include "speed_service.h"
#include <stdio.h>
#include "remoteDebug_service.h"

/* functions definition */
void IRAM_ATTR handleInterrupt();

const byte interruptPin = 14;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

xTaskHandle vSpeedServiceTask_handle = NULL;

/* functions definitions */
void vSpeedServiceTask(void *pvParameters);

uint8_t uSetupSpeedService()
{
    //pinMode(interruptPin, INPUT_PULLDOWN);
    //attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);

    /* creates buzzer update task */
    xTaskCreate(
        vSpeedServiceTask,          /* Task function. */
        "vSpeedServiceTask",        /* String with name of task. */
        10000,                      /* Stack size in words. */
        NULL,                       /* Parameter passed as input of the task */
        10,                         /* Priority of the task. */
        &vSpeedServiceTask_handle); /* Task handle. */

    if (vSpeedServiceTask_handle == NULL)
    {
        debugE("vSpeedServiceTask ... Error!");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vSpeedServiceTask(void *pvParameters)
{
    for (;;)
    {
        if (interruptCounter > 0)
        {
            portENTER_CRITICAL(&mux);
            interruptCounter--;
            portEXIT_CRITICAL(&mux);

            numberOfInterrupts++;
            // debugI("An interrupt has occurred. Total: ");
            //debugI("%d", numberOfInterrupts);
        }
        vTaskDelay(30);
    }
    vTaskDelete(vSpeedServiceTask_handle);
}

//void IRAM_ATTR handleInterrupt()
//{
//portENTER_CRITICAL_ISR(&mux);
//debugI("An interrupt has occurred.  ");
//interruptCounter++;
//portEXIT_CRITICAL_ISR(&mux);
//}