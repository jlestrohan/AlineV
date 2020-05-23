/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-10 23:46:27
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 10:14:44
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_LEDSTRIP_MODULE_H
#define _IC_LEDSTRIP_MODULE_H

#include <FastLed.h>
#include <stdint.h>

typedef struct
{
    uint8_t led_color; /* red when backward, Aqua forward */
    uint8_t is_lit;    /* boolean */
} lit_status_t;

/**
 * @brief  To be included in the setup
 * @note   
 * @param  status: bool true false : lit/unlit
 * @retval 
 */
uint8_t uLedStripSetup(uint8_t status);

/**
 * @brief  
 * @note   
 * @param  status: bool true false : lit/unlit
 */
void vSetLedStripLit(uint8_t status);

#endif /* _IC_LEDSTRIP_MODULE_H */
