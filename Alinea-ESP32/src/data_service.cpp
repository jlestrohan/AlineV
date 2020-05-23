/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-19 15:43:59
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-21 11:18:12
 * @ Description: This task decodes a json coming from the STM32 and makes it ready to be
 *                  sent over to AWS, via the embedded AWS service.
 *******************************************************************************************/

#include "configuration_esp32.h"
#include <stdio.h>
#include "data_service.h"
#include <FreeRTOS.h>
#include "remoteDebug_service.h"
#include <ArduinoJson.h>
#include "ntp_service.h"
#include "autoconnect_service.h"

static xTaskHandle xDataReceiveJson;
//QueueHandle_t xQueueAWS_Send; /* extern */

/* function definitions */
static uint8_t uRemakeJSON(xJsonPackage_t *jsonPack);

static void vDataReceiveJsonTask(void *vParameters)
{
    //xJsonPackage_t json_msg;

    for (;;)
    {
        /* if (xQueueDataJson != NULL)
        {
            xQueueReceive(xQueueDataJson, &json_msg, portMAX_DELAY);
            uRemakeJSON(&json_msg);

            // debugI("%.*s", json_msg.length, (char *)json_msg.json_str);
        }*/

        vTaskDelay(20);
    }
    vTaskDelete(NULL);
}

/* ----------------------- MAIN INITIALIZATION ROUTINE ---------------------- */
/**
 * @brief  Main Data Service Initialization routine
 * @note   
 * @retval 
 */
uint8_t uSetupDataServiceInit()
{
    //xQueueDataJson = xQueueCreate(3, sizeof(xJsonPackage_t));
    // if (xQueueDataJson == NULL)
    /// {
    //     debugE("xQueueDataJson ... Error");
    //      return EXIT_FAILURE;
    //  }
    //debugI("xQueueDataJson ... Success!");

    /* let's create the parser task first */
    xTaskCreate(
        vDataReceiveJsonTask,   /* Task function. */
        "vDataReceiveJsonTask", /* String with name of task. */
        2048,                   /* Stack size in words. */
        NULL,                   /* Parameter passed as input of the task */
        8,                      /* Priority of the task. */
        &xDataReceiveJson);     /* Task handle. */

    if (xDataReceiveJson == NULL)
    {
        debugE("Error creating serial parser task!");
        /* cannot create task, remove all created stuff and exit failure */
        //  vQueueDelete(xQueueDataJson);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  TYake a JSON coming from the STM32 and rebuild it adding infos from the STM32
 *          Timestamp, uuid etc etc..
 * @note   
 * @param  *jsonPack: 
 * @retval 
 */
static uint8_t inline uRemakeJSON(xJsonPackage_t *jsonPack)
{
    /* deserialization doc */
    StaticJsonDocument<MAX_JSON_MSG_SIZE> doc;
    DeserializationError err = deserializeJson(doc, jsonPack->json_str);

    /* serialization doc */
    StaticJsonDocument<MAX_JSON_MSG_SIZE> newdoc;

    if (err)
    {
        debugE("deserializeJson() failed with code %s", err.c_str());
        return EXIT_FAILURE;
    }

    /* constant topics */
    const char *stm32_uuid = doc["uuid"];
    newdoc["stm32_uuid"] = stm32_uuid;
    newdoc["esp32_uuid"] = ESP.getEfuseMac();

    newdoc["timestamp"] = ulGetEpochTime();

    const char *cmd_type = doc["command_type"];
    newdoc["type"] = doc["command_type"];

    JsonObject data = newdoc.createNestedObject("data");

    /* rebuild according to the type of data */
    /*** ATM ***/
    if (strcmp(cmd_type, "ATM") == 0)
    {
        data["sensortype"] = doc["data"]["sensor_type"];
        data["pressure"] = doc["data"]["pressure"];
        data["pressure_unit"] = "hpa";
        data["temperature"] = doc["data"]["temperature"];
        data["temperature_unit"] = "C";
    }

    char output[MAX_JSON_MSG_SIZE];
    uint8_t len = serializeJson(newdoc, output);

    /* now we can send out the newdoc to AWS_service which will handle the communication there, providing a struct containig the type and the json and its length 
    We will use a type to determine further in what MQTT topic to post each and every aws_rdy_data struct accordingly */
    /* prepare the package */
    aws_rdy_data_t awsData;
    strcpy(awsData.jsonstr, output);
    awsData.length = len;
    strcpy(awsData.type, doc["command_type"]);

    /* sends it over */
    if (xQueueAWS_Send != NULL)
    {
        if (xQueueAWS_Send != NULL)
        {
            if (!heap_caps_check_integrity_all(true))
                Serial.println("HEAP CORRUPTION DETECTED HERE!");
            //xQueueSend(xQueueAWS_Send, &awsData, portMAX_DELAY);
        }
    }
    else
    {
        debugE("Unable to use xQueueAWS_Send to send data over to AWS.. aborting!");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}