/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-21 23:13:00
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-25 15:35:30
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include "AWS_Certificate.h"
#include "wifi_credentials.h"
#include "ESP8266WiFi.h"
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include "wifi_credentials.h"
#include <ArduinoJson.h>
#include "ledstrip_module.h"

extern "C"
{
#include "libb64/cdecode.h"
}

/* functions definitions */
int b64decode(String b64Text, uint8_t *output);
void vWifiConnect();
static const char *motorMotion(uint8_t motionNum);

// check https://raphberube.com/blog/2019/02/18/Making-the-ESP8266-work-with-AWS-IoT.html

#define SOFTWBUFFCAPACITY 512
#define MAX_HDLC_FRAME_LENGTH 256
#define MAX_JSON_LENGTH 512
#define STRING_LEN 128

// Local wireless network
#define MQTT_PUB_TOPIC "AlineV/data/atmospheric"
#define AWS_IOT_ENDPOINT "a2im1z2thfpkge-ats.iot.eu-west-3.amazonaws.com"
#define CONFIG_VERSION "0.35"

//SoftwareSerial swSer(14, 12, false, 256);
SoftwareSerial stmRxTx(4, 5); // The esp8266 RX TX used to match with the nucleo bord

void callback(char *topic, byte *payload, unsigned int len); // to get message from AWS
void connectToAws();                                         // to connect esp8266 to AWS core (mqtt protocol)
String getRxdata();                                          //to get sensor data from Nucleo board

void setCurrentTime();                  // to be informed on the certification validation...
uint8_t sendDatatoAws(String jsonData); // post data in json format to aws dynamodb

WiFiClientSecure wiFiClient;
PubSubClient pubSubClient(AWS_IOT_ENDPOINT, 8883, callback, wiFiClient); //MQTT Client

uint8_t shadowReady = false;
bool isReady = false;

 uint8_t receivedChars[MAX_JSON_LENGTH]; // an array to store the received data
size_t numBytes;
uint8_t inChar;
uint16_t pos;

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
  stmRxTx.setTimeout(100);
  Serial.setDebugOutput(false);

  uint8_t binaryCert[AWS_CERT_CRT.length() * 3 / 4];
  int len = b64decode(AWS_CERT_CRT, binaryCert);
  wiFiClient.setCertificate(binaryCert, len);

  uint8_t binaryPrivate[AWS_CERT_PRIVATE.length() * 3 / 4];
  len = b64decode(AWS_CERT_PRIVATE, binaryPrivate);
  wiFiClient.setPrivateKey(binaryPrivate, len);

  uint8_t binaryCA[AWS_CERT_CA.length() * 3 / 4];
  len = b64decode(AWS_CERT_CA, binaryCA);
  wiFiClient.setCACert(binaryCA, len);

  vWifiConnect();

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
  getRxdata();

  if (shadowReady)
  {
    receivedChars[numBytes-1] = 0;
    Serial.print((char *)receivedChars);
    sendDatatoAws((const char *)receivedChars);
    shadowReady = false;
    receivedChars[0] = '\0';
    
  }
}

/* ------------------------- CONNECT TO AWS ROUTINE ------------------------- */
/**
 * @brief  Main connection endtry function to AWS IoT Core services
 * @note   
 * @retval None
 */
void connectToAws()
{
  if ((isReady) && !pubSubClient.connected())
  {
    Serial.print(F("PubSubClient connecting to: "));
    Serial.print(AWS_IOT_ENDPOINT);
    while (!pubSubClient.connected())
    {
      Serial.print(".");
      pubSubClient.connect(HOSTNAME);
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
    numBytes = stmRxTx.readBytes(receivedChars, MAX_JSON_LENGTH);
    
    //..if (inChar == 0x7e ||pos < MAX_JSON_LENGTH) {      
    //pos = 0;
      shadowReady = true;
    //} else {
   //   receivedChars[pos] = (char)inChar;
   //   pos++;
   // }
    

    
    
    
    
   // shadow = stmRxTx.readString(); //read() read from RX_PIN  character by character
    //Serial.println(shadow);
    
  }
  //Serial.println(resp);
  //Serial.println(strlen(resp.c_str()));
  return "";
}

/* ---------------------------- DATA SEND TO AWS ---------------------------- */
/**
 * @brief  
 * @note   
 * @param  jsonData: 
 * @retval 
 */
uint8_t sendDatatoAws(String jsonData)
{
  String mqttTopic; /* dynamic topic */

  const size_t capacity = JSON_OBJECT_SIZE(5) + 1000;
  DynamicJsonDocument doc(capacity);
  DeserializationError err = deserializeJson(doc, jsonData);

  if (err)
  {
    Serial.print(F("deserializeJson() failed with code: "));
    Serial.println(err.c_str());
    return EXIT_FAILURE;
  }

  /* serialization doc */
  DynamicJsonDocument newdoc(capacity);

  String stm32_uuid = doc["id"];
  //newdoc["timestamp"] =

  //Json document construction
  JsonObject root = newdoc.to<JsonObject>();
  //JsonObject Payload = root.createNestedObject("payload");
  //JsonObject Reported = State.createNestedObject("reported");

  root["deviceid"] = stm32_uuid;
  //root["ts"] = time(nullptr);

  /* we create the data accordingly */
  /**********************************************************/
  /* ATMOSPHERIC */
  if (doc["cmd"] == "ATM")
  { /* atmospheric data */
    String sensortype = doc["data"]["sns"];
    float pressure = doc["data"]["Ps"];
    float temperature = doc["data"]["Tp"];
    float humidity = doc["data"]["Hm"];

    root["sensor_type"] = sensortype;
    root["pressure"] = pressure;
    root["temperature"] = temperature;
    root["humidity"] = 0;

    mqttTopic = "AlineV/data/atmospheric";
  }
  /**********************************************************/
  /* NAVIGATION */
  else if (doc["cmd"] == "NAV")
  {
    //"data":{"mtSpL":0,"mtSpR":0,"mtMotL":0,"mtMotR":0,"cmPi":0,"cmRo":0,"cmHdg":27,"hcFr":0,"hcBt":7,"Uv":true}}

    root["heading"] = doc["data"]["cmHdg"];
    root["roll"] = doc["data"]["cmRo"];
    root["pitch"] = doc["data"]["cmPi"];
    root["uvstatus"] = doc["data"]["Uv"];
    root["obst_front_cm"] = doc["data"]["hcFr"];
    root["obst_rear_cm"] = doc["data"]["HcRr"];
    root["obst_bottm_cm"] = doc["data"]["hcBt"];
    root["speed_left"] = doc["data"]["mtSpL"];
    root["speed_right"] = doc["data"]["mtSpR"];

    root["motionL"] = motorMotion(doc["data"]["mtMotL"]);
    root["motionR"] = motorMotion(doc["data"]["mtMotR"]);

    mqttTopic = "AlineV/data/navigation";
  }

  /*Serial.printf("Sending  [%s]: ", MQTT_PUB_TOPIC);*/
  serializeJson(root, Serial); //Json doc serialisation
  Serial.println();
  char shadow[MAX_JSON_LENGTH];
  serializeJson(root, shadow, sizeof(shadow));

  /* let's publish this */
  if (!pubSubClient.publish(mqttTopic.c_str(), shadow, false))
  { // send json data to dynamoDbB topic
    Serial.println(F("ERROR??? :"));
    Serial.println(pubSubClient.state()); //Connected '0'*/
  }

  return EXIT_SUCCESS;
}

/* ------------------------------ AWS CALLBACK ------------------------------ */
/**
 * @brief  Amazon AWS IoT Core callback
 * @note   
 * @param  *topic: 
 * @param  *payload: 
 * @param  length: 
 * @retval None
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  //Serial.print(F("Message received on "));
  //Serial.print(topic);
  //Serial.print(F(": "));
  //for (unsigned int i = 0; i < length; i++)
  //{
  //  Serial.print((char)payload[i]);
  //}
  // Serial.println();
}

/* -------------------------------- NTP SETUP ------------------------------- */
/**
 * @brief  
 * @note   
 * @retval None
 */
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

/* ------------------------------ WIFI CONNECT ------------------------------ */
/**
 * @brief  Connection to wifi
 * @note   
 * @retval None
 */
void vWifiConnect()
{
  /* first we try to check in the EEPROM if we have any credentials stored */

  Serial.print(F("Connecting to "));
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.waitForConnectResult();
  Serial.print(F(", WiFi connected, IP address: "));
  Serial.println(WiFi.localIP());
}

int b64decode(String b64Text, uint8_t *output)
{
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char *)output, &s);
  return cnt;
}

static const char *motorMotion(uint8_t motionNum)
{
  switch (motionNum)
  {
  case 0:
    return "IDL";
    break;
  case 1:
    return "FWD";
    break;
  case 2:
    return "BWD";
    break;
  default:
    return "IDL";
    break;
  }
}