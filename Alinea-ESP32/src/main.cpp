/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-20 16:29:58
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-21 23:07:35
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include "ota.h"

RemoteDebug Debug;

uint32_t mLastTime = 0;
uint32_t mTimeSeconds = 0;

/**
 * @brief  Main setup
 * @note   
 * @retval None
 */
void setup()
{
  Serial.begin(115200);

  setupOTA();
  debugI("Ready.");
  	system_print_meminfo();

  Serial.println("Ready UART");
}

/**
 * @brief  Main program loop
 * @note   
 * @retval None
 */
void loop()
{
  otaLoop();
    
  if ((millis() - mLastTime) >= 1000)
  {
    // Time
    mLastTime = millis();
    mTimeSeconds++;

    // Debug the time (verbose level)
    debugV("* Time: %u seconds (VERBOSE)", mTimeSeconds);
    if (mTimeSeconds % 5 == 0)
    { // Each 5 seconds
      // Debug levels
      debugV("* This is a message of debug level VERBOSE");
      debugD("* This is a message of debug level DEBUG");
      debugI("* This is a message of debug level INFO");
      debugW("* This is a message of debug level WARNING");
      debugE("* This is a message of debug level ERROR");
    }
  }

 if(Serial.available()>0)
  {
    byte x = Serial.read();
    debugI("%c",x);
    
  }
  
  
}

