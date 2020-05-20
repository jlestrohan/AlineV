/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 22:13:03
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 20:14:22
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_AUTOCONNECT_SERVICE_H
#define _IC_AUTOCONNECT_SERVICE_H

#include "FreeRTOS.h"
#include "configuration_esp32.h"
#include <stdint.h>
#include "configuration_esp32.h"

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "73727170";

extern QueueHandle_t xQueueAWS_Send;

/**
 * @brief  Used by the data service to send here a complete json package ready to be sent over to AWS
 * @note   
 * @retval None
 */
typedef struct
{
    char jsonstr[MAX_JSON_MSG_SIZE]; /* the json string ready to be sent */
    uint16_t length;                 /* length of the json message, still useful... in case */
    char type[8];                    /* 3 letters, type so we know which MQTT topic to post that in */
} aws_rdy_data_t;

uint8_t uSetupAutoConnect();

#endif /* _IC_AUTOCONNECT_SERVICE_H */
