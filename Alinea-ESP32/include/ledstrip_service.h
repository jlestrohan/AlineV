/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:27
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-18 16:54:35
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_LEDSTRIP_SERVICE_H
#define _IC_LEDSTRIP_SERVICE_H

#define FASTLED_INTERNAL

#include <FastLed.h>
#include <stdint.h>

extern QueueHandle_t xLedStripCommandQueue;

typedef struct
{
    uint8_t led_color; /* red when backward, Aqua forward */
    uint8_t is_lit;    /* boolean */
} lit_status_t;

int uLedStripServiceInit();

#endif /* _IC_LEDSTRIP_SERVICE_H */
