/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-18 09:28:57
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-20 18:00:14
 * @ Description:
 *******************************************************************************************/

#include "configuration_esp32.h"
#include "aws_service.h"
#include "remoteDebug_service.h"
#include <stdio.h>
#include "autoconnect_service.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "AWS_Certificate.h"

static xTaskHandle xAWS_Send_Task_hnd = NULL;
static xTaskHandle xAWS_Receive_Task_hnd = NULL;
static xTaskHandle xAWS_Connect_Task_hnd = NULL;

/* functions definitions */
void connectToAWS();

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

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

/* ---------------------------- MQTT CONNECT TASK --------------------------- */
void vAWSConnectTaskCode(void *vParameter)
{
    EventBits_t uxBits;

    for (;;)
    {

        vTaskDelay(1000);
    }
    vTaskDelete(NULL);
}

/* --------------------------- MAIN INITIALIZATION -------------------------- */
/**
 * @brief  Main Initialization routine
 * @note   
 * @retval 
 */
uint8_t
uSetupAwsService()
{
    xQueueAWS_Send = xQueueCreate(3, sizeof(aws_rdy_data_t));
    if (xQueueAWS_Send == NULL)
    {
        DEBUG_SERIAL("error creating the xQueueAWS_Send queue");

        return EXIT_FAILURE;
    }

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
        Serial.println("Error creating serial parser task!");
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

    /* let's create the parser task first */
    xTaskCreate(
        vAWSConnectTaskCode,     /* Task function. */
        "vAWSConnectTaskCode",   /* String with name of task. */
        10000,                   /* Stack size in words. */
        NULL,                    /* Parameter passed as input of the task */
        12,                      /* Priority of the task. */
        &xAWS_Connect_Task_hnd); /* Task handle. */

    if (xAWS_Connect_Task_hnd == NULL)
    {
        debugE("Error creating xAWS_Connect_Task_hnd task!");
        /* cannot create task, remove all created stuff and exit failure */
        vQueueDelete(xQueueAWS_Send);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void connectToAWS()
{
    // Configure WiFiClientSecure to use the AWS certificates we generated
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Connect to the MQTT broker on the AWS endpoint we defined earlier
    client.begin(AWS_IOT_ENDPOINT, 8883, net);

    // Try to connect to AWS and count how many times we retried.
    int retries = 0;
    DEBUG_SERIAL("Connecting to AWS IOT");

    while (!client.connect(DEVICE_NAME) && retries < AWS_MAX_RECONNECT_TRIES)
    {
        Serial.print(".");
        delay(100);
        retries++;
    }

    // Make sure that we did indeed successfully connect to the MQTT broker
    // If not we just end the function and wait for the next loop.
    if (!client.connected())
    {
        Serial.println(" Timeout!");
        debugE("MQTT Timeout!...");
        return;
    }

    // If we land here, we have successfully connected to AWS!
    // And we can subscribe to topics and send messages.
    DEBUG_SERIAL("MQTT Connected!");
}