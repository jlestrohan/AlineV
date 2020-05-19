/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-19 15:43:43
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 20:09:00
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_DATA_SERVICE_H
#define _IC_DATA_SERVICE_H

#include "configuration_esp32.h"
#include "FreeRTOS.h"

extern QueueHandle_t xQueueDataJson;

typedef struct
{
    uint8_t json_str[MAX_JSON_MSG_SIZE];
    uint16_t length;
} xJsonPackage_t;

uint8_t uSetupDataServiceInit();

#endif /* _IC_DATA_SERVICE_H */
