/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-23 02:37:46
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-23 11:06:30
 * @ Description:
 *******************************************************************************************/

#include "wifi_module.h"
#include "AWS_Certificate.h"
#include <stdint.h>
#include "wifi_credentials.h"

extern "C"
{
#include "libb64/cdecode.h"
}

/* function definitions */
int b64decode(String b64Text, uint8_t *output);
void vWifiConnect();

WiFiClientSecure wiFiClient;                                             //WIFI Client

/* ------------------------------- WIFI SETUP ------------------------------- */
/**
 * @brief  Wifi setup routine
 * @note   
 * @retval 
 */
uint8_t uSetupWifi()
{
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
  
  return EXIT_SUCCESS;
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
  
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

int b64decode(String b64Text, uint8_t *output)
{
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char *)output, &s);
  return cnt;
}