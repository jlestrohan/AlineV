/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-18 09:28:57
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 08:36:02
 * @ Description:
 *******************************************************************************************/

#include "FreeRTOS.h"
#include "aws_service.h"
#include "remoteDebug_service.h"
#include <stdio.h>
#include "autoconnect_service.h"

/* because ou json messages are quite small we can keep them in the stack to speed up the process */

static xTaskHandle xAWS_Send_Task_hnd = NULL;
static xTaskHandle xAWS_Receive_Task_hnd = NULL;

const char *awsEndpoint = "a2im1z2thfpkge-ats.iot.eu-west-3.amazonaws.com";
// https://hieromon.github.io/AutoConnect/howtoembed.html#used-with-mqtt-as-a-client-application
/* -------------------------------- AWS SEND -------------------------------- */
/**
 * @brief  AWS SEND task. 
 *          This task prepares and encapsulates datas to send AWS to AWS. 
 *         
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
static void vAWSSendTaskCode(void *vParameter)
{
    for (;;)
    {

        vTaskDelay(10);
    }
    vTaskDelete(NULL);
}

/* ------------------------------- AWSRECEIVE ------------------------------- */
/**
 * @brief  AWS RECEIVE task
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
static void vAWSReceiveTaskCode(void *vParameter)
{
    aws_rdy_data_t aws_ReceiveBuf;

    for (;;)
    {
        if (xQueueAWS_Send != NULL)
        {
            xQueueReceive(xQueueAWS_Send, &aws_ReceiveBuf, portMAX_DELAY);
            debugI("FINAL JSON to be sent over is... : %s", (char *)aws_ReceiveBuf.jsonstr);
        }
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

/* --------------------------- MAIN INITIALIZATION -------------------------- */
/**
 * @brief  Main Initialization routine
 * @note   
 * @retval 
 */
uint8_t uSetupAwsService()
{
    xQueueAWS_Send = xQueueCreate(3, sizeof(aws_rdy_data_t));
    if (xQueueAWS_Send == NULL)
    {
        debugE("error creating the xQueueAWS_Send queue\n\r");
        Serial.println("error creating the xQueueAWS_Send queue\n\r");

        return EXIT_FAILURE;
    }

    /* creating PubSubClient handler */
    //PubSubClient client(

    /* create tasks */
    /* let's create the parser task first */
    xTaskCreate(
        vAWSSendTaskCode,         /* Task function. */
        "vCommandParserTaskCode", /* String with name of task. */
        10000,                    /* Stack size in words. */
        NULL,                     /* Parameter passed as input of the task */
        1,                        /* Priority of the task. */
        &xAWS_Send_Task_hnd);     /* Task handle. */

    if (xAWS_Send_Task_hnd == NULL)
    {
        debugE("Error creating serial parser task!");
        return EXIT_FAILURE;
    }

    /* let's create the parser task first */
    xTaskCreate(
        vAWSReceiveTaskCode,      /* Task function. */
        "vCommandParserTaskCode", /* String with name of task. */
        10000,                    /* Stack size in words. */
        NULL,                     /* Parameter passed as input of the task */
        1,                        /* Priority of the task. */
        &xAWS_Receive_Task_hnd);  /* Task handle. */

    if (xAWS_Receive_Task_hnd == NULL)
    {
        debugE("Error creating serial parser task!");
        /* cannot create task, remove all created stuff and exit failure */
        vQueueDelete(xQueueAWS_Send);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
