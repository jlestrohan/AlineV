/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:27
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-11 09:11:29
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_LEDSTRIP_SERVICE_H
#define _IC_LEDSTRIP_SERVICE_H

#include <FastLed.h>
#include <stdint.h>

extern QueueHandle_t xLedStripCommandQueue;

typedef struct
{
    int led_color; /* red when backward, Aqua forward */
    int is_lit;    /* boolean */
} lit_status_t;

int uLedStripServiceInit();

#endif /* _IC_LEDSTRIP_SERVICE_H */
