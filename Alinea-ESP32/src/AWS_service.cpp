/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-18 09:28:57
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 22:01:03
 * @ Description:
 *******************************************************************************************/

#include "FreeRTOS.h"
#include "aws_service.h"
#include "remoteDebug_service.h"
#include <stdio.h>
#include <PubSubClient.h>

/* because ou json messages are quite small we can keep them in the stack to speed up the process */

xTaskHandle xAWS_Send_Task_hnd = NULL;
xTaskHandle xAWS_Receive_Task_hnd = NULL;

/* -------------------------------- AWS SEND -------------------------------- */
/**
 * @brief  AWS SEND task. 
 *          This task prepares and encapsulates datas to send AWS to AWS. 
 *         
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
void vAWSSendTaskCode(void *vParameter)
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
void vAWSReceiveTaskCode(void *vParameter)
{
    aws_rdy_data_t aws_ReceiveBuf;

    for (;;)
    {
        xQueueReceive(xQueueAWS_Send, &aws_ReceiveBuf, portMAX_DELAY);
        Serial.println("FINAL JSON to be sent over is... : %s", aws_ReceiveBuf.jsonstr);

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
    xQueueAWS_Send = xQueueCreate(10, sizeof(aws_rdy_data_t));
    if (xQueueAWS_Send == NULL)
    {
        debugE("error creating the xQueueAWS_Send queue\n\r");
        Serial.println("error creating the xQueueAWS_Send queue\n\r");

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
        /* cannot create task, remove all created stuff and exit failure */
        //vQueueDelete(xQueueCommandParse);
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

/* ----------------------------- CONNECT TO AWS ----------------------------- */
/**
 * @brief  Connect to Amazon Webservices MQTT Broker
 * @note   
 * @retval 
 */
/*
uint8_t uConnectToAWS()
{
    int NodeState;

    // closes TCP connection
    awsTCPClient.Close();

    // get the broker address out of non-volatile storage
    String strBrokerAddress = preferences.getString("BrokerAddress", "");
    char *txtBrokerAddress = const_cast<char *>(strBrokerAddress.c_str()); // convert to char*

    // TCP-connect to AWS MQTT Server
    int TCP_Connect_Result = awsTCPClient.Connect(txtBrokerAddress, 8883);

    // Success?
    if (TCP_Connect_Result != 1)
    {
        // no = red LED
        BreadboardRGBLED.SwitchRGBLED(LED_RED);
        BreadboardRGBLED.setRGB(255, 0, 0); // and set the default color to red for dimming
        NodeState = NODESTATE_NOTCPCONNECTION;
    }
    else
    {
        // yes = yellow LED
        BreadboardRGBLED.SwitchRGBLED(LED_YELLOW);

        // take Device Name out of non-volatile storage
        String strDeviceID = preferences.getString("DeviceID", "");

        // MQTT CONNECT to AWS broker , use Device Name of your device as Client ID
        int MQTT_Connect_Result = awsMQTTClient.Connect(strDeviceID, "", "");

        if (MQTT_Connect_Result == 1)
        {
            // connected
            BreadboardRGBLED.SwitchRGBLED(LED_GREEN); // green LED
            BreadboardRGBLED.setRGB(0, 255, 0);       // and set the default color to green for dimming
            NodeState = NODESTATE_OK;

            Serial.println("MQTT connected.");
        }
        else
        {
            // not connected
            BreadboardRGBLED.SwitchRGBLED(LED_RED); // red LED
            BreadboardRGBLED.setRGB(255, 0, 0);     // and set the default color to red for dimming
            NodeState = NODESTATE_NOMQTTCONNECTION;

            Serial.println("MQTT not connected.");
        }
    }

    return NodeState;
}
*/