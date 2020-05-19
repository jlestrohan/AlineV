/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:35:25
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 01:10:04
 * @ Description:   Main ESP32 Alinea Hardware Configuration file
 *                  All software configuration must be recorded on the SD card for both ctrl
 *******************************************************************************************/

#ifndef _IC_CONFIGURATION_ESP32_H
#define _IC_CONFIGURATION_ESP32_H

#define STM32_UUID "1256454565"

#define CMD_LINE_MAX_LENGTH 50
#define MAX_HDLC_FRAME_LENGTH 1024 /* this is the main frame length available */
#define MAX_JSON_MSG_SIZE 256

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
