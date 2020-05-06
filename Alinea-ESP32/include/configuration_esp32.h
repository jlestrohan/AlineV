/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:35:25
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-06 17:52:38
 * @ Description:   Main ESP32 Alinea Hardware Configuration file
 *                  All software configuration must be recorded on the SD card for both ctrl
 *******************************************************************************************/

#ifndef _IC_CONFIGURATION_ESP32_H
#define _IC_CONFIGURATION_ESP32_H

/**
 * @brief free up pointer and pointed memory at the same time 
 * @note   
 * @retval 
 */
#define FREE(x)      \
    {                \
        if (x)       \
            free(x); \
        x = NULL;    \
    }

/* both functions in one macro */
#define DEBUG_SERIAL(x)    \
    {                      \
        debugI(x);         \
        Serial.println(x); \
    }
#define THINGNAME "Alinea-ESP32"

#endif /* _IC_CONFIGURATION_ESP32_H */
