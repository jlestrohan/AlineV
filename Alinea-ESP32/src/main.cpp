/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-02 23:51:42
 * @ Description:
 *******************************************************************************************/

// check https://github.com/muratdemirtas/ESP8266-UART-RX-INTERRUPT/blob/master/main.ino
// check https://github.com/maxgerhardt/pio-stm32-with-esp8266-dht11
// https://github.com/krzychb/esp-just-slip
// https://github.com/martin-ger/esp_slip_router
// https://forum.arduino.cc/index.php?topic=576983.0

#include <Arduino.h>
#include "autoconnect_service.h"
#include "ota_service.h"
#include "configuration_esp32.h"
#include <FreeRTOS.h>
#include "buzzer_service.h"
#include "stm32Serial_service.h"
#include "ntp_service.h"
#include "oled_service.h"
#include "remoteDebug_service.h"
#include "buzzer_service.h"
#include "command_parser.h"
#include "bluetooth_serial.h"

/* reemoving brownout detector */
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);

  uSetupBuzzer();
  //uSetupCmdParser();
  //setupBTSerial();
  setupAutoConnect();
  uSetupRemoteDebug();
  uSetupNTPService();
  uSetupSTM32SerialService();
  setupOLED();
  setupOTA();

  debugI("Ready.");
  debugI("%ul", GET_CHIPID());

  vPlayMelody(MelodyType_CommandReady);
}
