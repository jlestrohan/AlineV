/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-22 22:52:46
 * @ Description:
 *******************************************************************************************/

#include "ledstrip_service.h"
#include <FastLed.h>
#include "remoteDebug_service.h"

#define NUM_LEDS 16
#define DATA_PIN 4

/* functions definitions */
static void vLedStripTask(void *pvParameters);

static xTaskHandle xLedStripTask_handle;

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

    /* creates buzzer update task */
    xTaskCreate(
        vLedStripTask,          /* Task function. */
        "vLedStripTask",        /* String with name of task. */
        2048,                   /* Stack size in words. */
        NULL,                   /* Parameter passed as input of the task */
        2,                      /* Priority of the task. */
        &xLedStripTask_handle); /* Task handle. */

    if (xLedStripTask_handle == NULL)
    {
        debugI("xLedStripTask_handle ... Error!");
        return EXIT_FAILURE;
    };
    debugI("Led Strip Service created and running..");
    return EXIT_SUCCESS;
}

/**
 * @brief  This task receives a variable in the queue that gives the color and the lit status
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
static void vLedStripTask(void *vParameter)
{
    xLedStripCommandQueue = xQueueCreate(10, sizeof(lit_status_t));
    if (xLedStripCommandQueue == NULL)
    {
        debugI("xLedStripCommandQueue ... Error");
        vTaskDelete(NULL);
    }
    else
    {
        debugI("Starting LED Strip Task... Success");
    }

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
            vTaskDelay(50); /* bug in library needs this called twice */
            FastLED.clear();
            FastLED.show();
        }

        vTaskDelay(300);
    }
    vTaskDelete(NULL);
}