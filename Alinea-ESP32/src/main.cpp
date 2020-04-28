/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 18:37:54
 * @ Description:
 *******************************************************************************************/

// check https://github.com/muratdemirtas/ESP8266-UART-RX-INTERRUPT/blob/master/main.ino
// check https://github.com/maxgerhardt/pio-stm32-with-esp8266-dht11
// https://github.com/krzychb/esp-just-slip
// https://github.com/martin-ger/esp_slip_router
// https://forum.arduino.cc/index.php?topic=576983.0

#include <Arduino.h>
#include "configuration_esp32.h"
#include <FreeRTOS.h>
#include "autoconnect_service.h"
#include "buzzer_service.h"
#include "serial_service.h"
#include "ntp_service.h"
#include "oled_service.h"
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"
#include "bluetooth_serial.h"

#define GET_CHIPID() (ESP.getCpuFreqMHz())

RemoteDebug Debug;

/**
 * @brief  Main program loop
 * @note   
 * @retval None
 */
void loop()
{
  // nada here everything is FreeRTOS!
}

/**
 * @brief  Main setup
 * @note   
 * @retval None
 */
void setup()
{
  Serial.begin(115200);

  setupBuzzer();
  setupCmdParser();
  //setupBTSerial();
  setupAutoConnect();
  setupRemoteDebug();
  setupNTPService();
  setupUARTListener();
  setupOLED();

  debugI("Ready.");
  //Serial.println(GET_CHIPID());

  Serial.println("Ready UART");
  commandReadyTune();
}
