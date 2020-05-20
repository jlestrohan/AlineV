/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:35:25
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 17:10:27
 * @ Description:   Main ESP32 Alinea Hardware Configuration file
 *                  All software configuration must be recorded on the SD card for both ctrl
 *******************************************************************************************/

#ifndef _IC_CONFIGURATION_ESP32_H
#define _IC_CONFIGURATION_ESP32_H

#define STM32_UUID "1256454565" // <<-- STUB!!!!!!! FIXME:

/* AWS STUFF */
#define DEVICE_NAME "AlineV"
#define AWS_MAX_RECONNECT_TRIES 50
#define AWS_IOT_ENDPOINT "a2im1z2thfpkge-ats.iot.eu-west-3.amazonaws.com"
#define AWS_IOT_TOPIC "$aws/things/" DEVICE_NAME "/shadow/update"

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
