/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-21 23:13:00
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 11:16:50
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
//#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "wifi_module.h"
#include "ota_module.h"
#include "ledstrip_module.h"

// check https://raphberube.com/blog/2019/02/18/Making-the-ESP8266-work-with-AWS-IoT.html

#define SOFTWBUFFCAPACITY 256
#define MAX_HDLC_FRAME_LENGTH 256
#define MAX_JSON_LENGTH 256
#define STRING_LEN 128

// Local wireless network
#define MQTT_PUB_TOPIC "AlineV/atmospheric"
#define AWS_IOT_ENDPOINT "a2im1z2thfpkge-ats.iot.eu-west-3.amazonaws.com"
#define CONFIG_VERSION "0.35"

//SoftwareSerial swSer(14, 12, false, 256);
SoftwareSerial stmRxTx(4, 5); // The esp8266 RX TX used to match with the nucleo bord

void callback(char *topic, byte *payload, unsigned int len); // to get message from AWS
void connectToAws();                                         // to connect esp8266 to AWS core (mqtt protocol)
String getRxdata();                                          //to get sensor data from Nucleo board
String shadow;
void setCurrentTime();               // to be informed on the certification validation...
uint8_t sendDatatoAws(String jsonData); // post data in json format to aws dynamodb

PubSubClient pubSubClient(AWS_IOT_ENDPOINT, 8883, callback, wiFiClient); //MQTT Client

bool shadow_ready = false;
uint8_t r;
char buffer[MAX_JSON_LENGTH];
uint8_t pos;
bool isReady = false;

String rxData = ""; //get rxData as response (sensor data) from nucleo board

/* ------------------------------- MAIN SETUP ------------------------------- */
/**
 * @brief  Main Setup
 * @note   
 * @retval None
 */
void setup()
{
  Serial.begin(115200);
  stmRxTx.begin(19200);
  Serial.setDebugOutput(false);
  
  uSetupWifi(); /* setup and connects to wifi */
  uSetupOta();
  
  uLedStripSetup(true); /* lights up the ledstrip below */
  
  /*BEGIN: Copied from https://github.com/HarringayMakerSpace/awsiot/blob/master/Esp8266AWSIoTExample/Esp8266AWSIoTExample.ino*/
  setCurrentTime(); //  // get current time, otherwise certificates are flagged as expired
 
  isReady = true;
}

/* -------------------------------- MAIN LOOP ------------------------------- */
/**
 * @brief  Main Loop
 * @note   
 * @retval None
 */
void loop()
{
  connectToAws();
  uLoopOTA();
  getRxdata();

  if (shadow_ready)
  {
    sendDatatoAws(shadow);
    shadow_ready = false;
    //Serial.print(shadow);
  }
}

/*****==================================================================*****/
void connectToAws()
{
  if ((isReady) && !pubSubClient.connected())
  {
    Serial.print("PubSubClient connecting to: ");
    Serial.print(AWS_IOT_ENDPOINT);
    while (!pubSubClient.connected())
    {
      Serial.print(".");
      pubSubClient.connect(HOST_NAME);
    }
    Serial.println(" connected");
    pubSubClient.subscribe(MQTT_PUB_TOPIC);
  }
  pubSubClient.loop();
}

/*****==================================================================*****/
String getRxdata()
{

  //espRxTx.write(request); //write a post to TX_PIN to get data
  if (stmRxTx.available() > 0) //
  {
    shadow = stmRxTx.readString(); //read() read from RX_PIN  character by character
    shadow_ready = true;
  }
  //Serial.println(resp);
  //Serial.println(strlen(resp.c_str()));
  return "";
}

/*****==================================================================*****/
uint8_t sendDatatoAws(String jsonData)
{
  //The model we got from nucleo UART
  // String jsonData = "{\"Id\": 4, \"PT\": 602, \"Dt\": {\"AH\": 49.00, \"Tp\": 20.00,
  //\"Ms\": 56.00, \"Clr\": red, \"Gs\": 32.00, \"WL\": empty, \"RGB\": [0,0,0]}}";

  String mqttTopic; /* dynamic topic */
  
  StaticJsonDocument<MAX_JSON_LENGTH> doc;
  DeserializationError err = deserializeJson(doc, jsonData);
  
   if (err)
    {
        Serial.println("deserializeJson() failed with code ");
        Serial.println(err.c_str());
        return EXIT_FAILURE;
    }
  
  /* serialization doc */
    StaticJsonDocument<MAX_JSON_LENGTH> newdoc;
  
  String stm32_uuid = doc["id"];
  //newdoc["timestamp"] = 
 
  //Json document construction
  JsonObject root = newdoc.to<JsonObject>();
  root["uuid"] = stm32_uuid;
  root["timestamp"] = time(nullptr);
  
  JsonObject Data = newdoc.createNestedObject("payload");

  /* we create the data accordingly */
  /* Atmospheric */
  if (doc["cmd"] = "ATM")
  { /* atmospheric data */
    String sensortype = doc["data"]["sns"];
    float pressure = doc["data"]["Ps"];
    float temperature = doc["data"]["Tp"];

    Data["sensor_type"] = sensortype;
    Data["pressure"] = pressure;
    Data["temperature"] = temperature;
    Data["humidity"] = 0;
    
    mqttTopic = "AlineV/atmospheric";
  }

  /*Serial.printf("Sending  [%s]: ", MQTT_PUB_TOPIC);*/
  serializeJson(root, Serial); //Json doc serialisation
  Serial.println();
  char shadow[MAX_JSON_LENGTH];
  serializeJson(root, shadow, sizeof(shadow));

/* let's publish this */
  if (!pubSubClient.publish(MQTT_PUB_TOPIC, shadow, false)) // send json data to dynamoDbB topic
    Serial.println("ERROR??? :");
  Serial.println(pubSubClient.state()); //Connected '0'*/
  
  return EXIT_SUCCESS;
}

/*****==================================================================*****/
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on ");
  Serial.print(topic);
  Serial.print(": ");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


/*****==================================================================*****/
void setCurrentTime()
{
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  Serial.println(now);
  
}
