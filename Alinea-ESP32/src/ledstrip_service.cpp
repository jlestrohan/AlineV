/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-12 23:18:29
 * @ Description:
 *******************************************************************************************/

#include "ledstrip_service.h"
#include <FastLed.h>
#include "remoteDebug_service.h"

#define NUM_LEDS 16
#define DATA_PIN 4

/* functions definitions */
void vLedStripTask(void *pvParameters);

xTaskHandle xLedStripTask_handle = NULL;

CRGB leds[NUM_LEDS];

/**
 * @brief  Main Initialization Routine
 * @note   
 * @retval 
 */
int uLedStripServiceInit()
{
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    randomSeed(analogRead(0));

    xLedStripCommandQueue = xQueueCreate(10, sizeof(lit_status_t));
    if (xLedStripCommandQueue == NULL)
    {
        debugI("xLedStripCommandQueue ... Error");
    }

    /* creates buzzer update task */
    xTaskCreate(
        vLedStripTask,          /* Task function. */
        "vLedStripTask",        /* String with name of task. */
        10000,                  /* Stack size in words. */
        NULL,                   /* Parameter passed as input of the task */
        2,                      /* Priority of the task. */
        &xLedStripTask_handle); /* Task handle. */

    if (xLedStripTask_handle == NULL)
    {
        debugI("xLedStripTask_handle ... Error!");
        return EXIT_FAILURE;
    }

    debugI("Led Strip Service created and running..");
    return EXIT_SUCCESS;
}

/**
 * @brief  This task receives a variable in the queue that gives the color and the lit status
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
void vLedStripTask(void *vParameter)
{
    lit_status_t lit_status;

    for (;;)
    {
        xQueueReceive(xLedStripCommandQueue, &lit_status, portMAX_DELAY);

        if (lit_status.is_lit)
        {
            for (uint8_t i = 0; i < NUM_LEDS; i++)
            {
                leds[i] = CRGB::DarkViolet;
            }
            FastLED.show();
        }
        else
        {
            FastLED.clear();
            FastLED.show();
        }

        vTaskDelay(300);
    }
    vTaskDelete(NULL);
}