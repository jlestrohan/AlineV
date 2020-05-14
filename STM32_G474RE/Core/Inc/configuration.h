/*******************************************************************
 * ALINEA_CONFIG.h
 *
 *  Created on: 26 avr. 2020
 *      Author: Jack Lestrohan
 *
 *******************************************************************/

#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

// IMPORTANT - uncomment this out in production!!!
//# define NDEBUG

#include "cmsis_os2.h"
#include <FreeRTOS.h>

typedef StaticTask_t osStaticThreadDef_t; /* defined once here to lighten the code elsewhere */

/**
 * DEBUG CONFIGURATION
 * Sensors dbg_printf enable/disable
 */

/** sensors loggers **/
#//define DEBUG_HCSR04_LEFT45
//#define DEBUG_HCSR04_RIGHT45
//#define DEBUG_HCSR04_FRONT
//#define DEBUG_HCSR04_BOTTOM
//#define DEBUG_HCSR04_REAR
//#define DEBUG_SYSTEMINFOS
//#define DEBUG_BMP280

/** service activation **/
#define DEBUG_SERVICE_LCD_MENU
#define DEBUG_SERVICE_BUTTON
#define DEBUG_SERVICE_HCSR04
#define DEBUG_SERVICE_MG90S
#define DEBUG_SERVICE_ESP32_SERIAL
#define DEBUG_SERVICE_MOTORS
#define DEBUG_SERVICE_NAVCONTROL
#define DEBUG_SERVICE_UVLED
//#define DEBUG_SERVICE_CMD_PARSER
//#define DEBUG_SERVICE_BMP280
#define DEBUG_SERVICE_CMPS12


/**
 * BEHAVIOUR CONTROL
 */
/* buttons debouncing */
#define BTN_DEBOUNCE_MS								150 /*  ms */

 /* on a forward motion, the rover will stop if the US BOTTOM sensor goes above the defined distance */
#define US_BOTTOM_SENSOR_HOLE_MIN_STOP_CM			10

/* what distance is considered by both angles of front distance sensing as "potential obstacle" */
#define US_ANGLE_MIN_STOP_CM						20	/* obstacle stop */
#define US_ANGLE_MIN_DANGER_CM						30	/* obstacle near danger */
#define US_ANGLE_MIN_WARNING_CM						40	/* obstacle potentially near, warning */

/* distance from which we stop the forward motion once an obstacle has been detected */
#define US_FRONT_MIN_STOP_CM						20	/* obstacle stop */
#define US_FRONT_MIN_DANGER_CM						30	/* obstacle near danger */
#define US_FRONT_MIN_WARNING_CM						40	/* obstacle potentially near, warning */

/* on a backward motion distance under which the rover will stop going backward */
#define US_REAR_MIN_STOP_CM							20


/**
 * osPriorityNone          =  0,         ///< No priority (not initialized).
  osPriorityIdle          =  1,         ///< Reserved for Idle thread.
  osPriorityLow           =  8,         ///< Priority: low
  osPriorityLow1          =  8+1,       ///< Priority: low + 1
  osPriorityLow2          =  8+2,       ///< Priority: low + 2
  osPriorityLow3          =  8+3,       ///< Priority: low + 3
  osPriorityLow4          =  8+4,       ///< Priority: low + 4
  osPriorityLow5          =  8+5,       ///< Priority: low + 5
  osPriorityLow6          =  8+6,       ///< Priority: low + 6
  osPriorityLow7          =  8+7,       ///< Priority: low + 7
  osPriorityBelowNormal   = 16,         ///< Priority: below normal
  osPriorityBelowNormal1  = 16+1,       ///< Priority: below normal + 1
  osPriorityBelowNormal2  = 16+2,       ///< Priority: below normal + 2
  osPriorityBelowNormal3  = 16+3,       ///< Priority: below normal + 3
  osPriorityBelowNormal4  = 16+4,       ///< Priority: below normal + 4
  osPriorityBelowNormal5  = 16+5,       ///< Priority: below normal + 5
  osPriorityBelowNormal6  = 16+6,       ///< Priority: below normal + 6
  osPriorityBelowNormal7  = 16+7,       ///< Priority: below normal + 7
  osPriorityNormal        = 24,         ///< Priority: normal
  osPriorityNormal1       = 24+1,       ///< Priority: normal + 1
  osPriorityNormal2       = 24+2,       ///< Priority: normal + 2
  osPriorityNormal3       = 24+3,       ///< Priority: normal + 3
  osPriorityNormal4       = 24+4,       ///< Priority: normal + 4
  osPriorityNormal5       = 24+5,       ///< Priority: normal + 5
  osPriorityNormal6       = 24+6,       ///< Priority: normal + 6
  osPriorityNormal7       = 24+7,       ///< Priority: normal + 7
  osPriorityAboveNormal   = 32,         ///< Priority: above normal
  osPriorityAboveNormal1  = 32+1,       ///< Priority: above normal + 1
  osPriorityAboveNormal2  = 32+2,       ///< Priority: above normal + 2
  osPriorityAboveNormal3  = 32+3,       ///< Priority: above normal + 3
  osPriorityAboveNormal4  = 32+4,       ///< Priority: above normal + 4
  osPriorityAboveNormal5  = 32+5,       ///< Priority: above normal + 5
  osPriorityAboveNormal6  = 32+6,       ///< Priority: above normal + 6
  osPriorityAboveNormal7  = 32+7,       ///< Priority: above normal + 7
  osPriorityHigh          = 40,         ///< Priority: high
  osPriorityHigh1         = 40+1,       ///< Priority: high + 1
  osPriorityHigh2         = 40+2,       ///< Priority: high + 2
  osPriorityHigh3         = 40+3,       ///< Priority: high + 3
  osPriorityHigh4         = 40+4,       ///< Priority: high + 4
  osPriorityHigh5         = 40+5,       ///< Priority: high + 5
  osPriorityHigh6         = 40+6,       ///< Priority: high + 6
  osPriorityHigh7         = 40+7,       ///< Priority: high + 7
  osPriorityRealtime      = 48,         ///< Priority: realtime
  osPriorityRealtime1     = 48+1,       ///< Priority: realtime + 1
  osPriorityRealtime2     = 48+2,       ///< Priority: realtime + 2
  osPriorityRealtime3     = 48+3,       ///< Priority: realtime + 3
  osPriorityRealtime4     = 48+4,       ///< Priority: realtime + 4
  osPriorityRealtime5     = 48+5,       ///< Priority: realtime + 5
  osPriorityRealtime6     = 48+6,       ///< Priority: realtime + 6
  osPriorityRealtime7     = 48+7,       ///< Priority: realtime + 7
 */
/* tasks priorities here */
#define OSTASK_PRIORITY_BUTTON_ONBOARD					osPriorityBelowNormal1
#define OSTASK_PRIORITY_NAVCONTROL_NORM_MOTION			osPriorityAboveNormal3
#define OSTASK_PRIORITY_NAVCONTROL_AVOID_MOTION			osPriorityAboveNormal4
#define OSTASK_PRIORITY_NAVCONTROL_DECISION				osPriorityBelowNormal2
#define OSTASK_PRIORITY_BUTTON_ADD						osPriorityBelowNormal3
#define OSTASK_PRIORITY_HCSR04							osPriorityHigh
#define OSTASK_PRIORITY_HCSR04_CTL						osPriorityHigh
#define OSTASK_PRIORITY_MG90S							osPriorityBelowNormal5
#define OSTASK_PRIORITY_MG90S_3PROBES					osPriorityBelowNormal6
#define OSTASK_PRIORITY_QMC5883							osPriorityBelowNormal2
#define OSTASK_PRIORITY_MPU6050							osPriorityLow7
#define OSTASK_PRIORITY_LCDMENU							osPriorityAboveNormal
#define OSTASK_PRIORITY_ESP32_TX						osPriorityNormal1
#define OSTASK_PRIORITY_ESP32_RX						osPriorityNormal2
#define OSTASK_PRIORITY_SYSTEMINFO						osPriorityLow6
#define OSTASK_PRIORITY_UVLED							osPriorityBelowNormal4
#define OSTASK_PRIORITY_CMD_SERVICE						osPriorityNormal3
#define OSTASK_PRIORITY_CMD_INTERP_SERVICE				osPriorityNormal4
#define OSTASK_PRIORITY_MOTORS CONTROL					osPriorityHigh
#define OSTASK_PRIORITY_BMP280							osPriorityLow5

#endif /* INC_CONFIGURATION_H_ */
