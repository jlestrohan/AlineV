/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 21:31:59
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include <Preferences.h>
#include "autoconnect_service.h"
#include "ota_service.h"
#include "configuration_esp32.h"
#include "remoteDebug_service.h"
#include <FreeRTOS.h>
#include "buzzer_service.h"
#include "stm32Serial_service.h"
#include "ntp_service.h"
#include "oled_service.h"
#include "buzzer_service.h"
#include "command_service.h"
#include "bluetooth_serial.h"
#include "speed_service.h"
#include "ledstrip_service.h"
#include "AWS_service.h"
#include "data_service.h"

/* reemoving brownout detector */
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define GET_CHIPID() (ESP.getEfuseMac());

/* functions definitions */
void vBootCounterUpdate();

RemoteDebug Debug;
QueueHandle_t xLedStripCommandQueue;

Preferences preferences;

/**
 * @brief  Main program loop
 * @note   
 * @retval None
 */
void loop()
{
  // nada here everything is FreeRTOS!
}

/* ------------------------------- MAIN SETUP ------------------------------- */
/**
 * @brief  Main setup
 * @note   
 * @retval None
 */
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  DEBUG_SERIAL("Starting Program...");

  vBootCounterUpdate();

  uLedStripServiceInit(); /* check */
  uSetupBuzzer();         /* check */
  uSetupCmdParser();
  //setupBTSerial();
  uSetupAutoConnect(); /* check */
  uSetupRemoteDebug(); /* check */
  uSetupNTPService();  /* check */
  uSetupDataServiceInit();
  uSetupSTM32SerialService(); /* check */
  //uSetupOLED();
  //uSetupSpeedService();
  uSetupAwsService();
  uSetupOTA();

  lit_status_t ledstatus;
  ledstatus.is_lit = true;
  if (xLedStripCommandQueue)
    xQueueSend(xLedStripCommandQueue, &ledstatus, portMAX_DELAY);

  char *cmdRdyESP = "ack restart";
  if (xQueueCommandParse != NULL)
    /* let's inform the STM that we have just rebooted */
    xQueueSend(xQueueCommandParse, &cmdRdyESP, portMAX_DELAY);

  vPlayMelody(MelodyType_CommandReady);
}

// TODO: connect 2xI2C TOF https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/#7

/* ------------------------------ BOOT COUNTER ------------------------------ */
/**
 * @brief  Updates (and print) the boot counter
 * @note   
 * @retval None
 */
void vBootCounterUpdate()
{
  /* we update the boot count counter to know how many times our device has booted */
  preferences.begin("alinev-core", false);
  // Get the counter value, if the key does not exist, return a default value of 0
  // Note: Key name is limited to 15 chars.
  unsigned int boot_counter = preferences.getUInt("boot_counter", 0);

  /* Increase counter by 1 */
  boot_counter++;

  // Print the counter to Serial Monitor
  //debugI.printf("Number of boots: %u\n", boot_counter);

  // Store the counter to the Preferences
  preferences.putUInt("boot_counter", boot_counter);

  // Close the Preferences
  preferences.end();
}