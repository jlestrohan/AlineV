/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 10:22:50
 * @ Description:
 *******************************************************************************************/

#include "ledstrip_module.h"
#include <FastLed.h>

#define NUM_LEDS 8
#define DATA_PIN 13

/* functions definitions */
void vSetLedStripLit(uint8_t status);

CRGB leds[NUM_LEDS];

/**
 * @brief  Main Initialization Routine
 * @note   
 * @retval 
 */
uint8_t uLedStripSetup(uint8_t status)
{
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    randomSeed(analogRead(0));
    vSetLedStripLit(true);
}

/**
 * @brief  Function that lits/darkens the ledstrip
 * @note   
 * @param  status: 
 * @retval None
 */
void vSetLedStripLit(uint8_t status)
{
   // if (status)
   // {
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            leds[i] = CRGB::DarkViolet;
        }
        FastLED.show();
        return;
    //}

    //FastLED.clear();
    //FastLED.show();
}

