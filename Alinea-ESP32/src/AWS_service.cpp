/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-18 09:28:57
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-18 17:43:24
 * @ Description:
 *******************************************************************************************/

#include "aws_service.h"
#include "remoteDebug_service.h"
#include <stdio.h>
#include <MQTTClientSecure.h>

// construct the object awsMQTTClient of class MQTTClientSecure
MQTTClientSecure awsMQTTClient = MQTTClientSecure();
// construct the object awsTCPClient of class TCPClientSecure
TCPClientSecure awsTCPClient = TCPClientSecure();

/* because ou json messages are quite small we can keep them in the stack to speed up the process */

xTaskHandle xAWS_Send_Task_hnd = NULL;
xTaskHandle xAWS_Receive_Task_hnd = NULL;

/* -------------------------------- AWS SEND -------------------------------- */
/**
 * @brief  AWS SEND task
 * @note   
 * @param  *vParameter: 
 * @retval None
 */
void vAWSSendTaskCode(void *vParameter)
{
    for (;;)
    {

        vTaskDelay(1);
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
    for (;;)
    {

        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

/* we create a queue to receive all the incoming datas from the command center to be repackaged */
QueueHandle_t xQueueAWS_Send;

/* --------------------------- MAIN INITIALIZATION -------------------------- */
/**
 * @brief  Main Initialization routine
 * @note   
 * @retval 
 */
uint8_t uSetuoAwsService()
{
    awsMQTTClient.myTCPClient = awsTCPClient; // tell the MQTT object the TCP object because MQTT library is using the TCP/IP library too

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
        //vQueueDelete(xQueueCommandParse);
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