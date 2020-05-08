/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 05:41:10
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-08 06:53:37
 * @ Description:   General command parser, check if any command string given to the main
 *                  entry point is valid and then dispatches accordingly
 *******************************************************************************************/

#ifndef _IC_COMMAND_PARSER_H
#define _IC_COMMAND_PARSER_H

#include <FreeRTOS.h>

extern QueueHandle_t xQueueCommandParse;

/**
 * @brief  Adressable Devices: (uint16_t DeviceFlag)
 * @note   
 * @param  DeviceFlag: 
 * @retval 
 */

#define MOTOR_LEFT (1 << 0)         /* control & speed info */
#define MOTOR_RIGHT (1 << 1)        /* control & speed info */
#define FRONT_ULTRASONIC (1 << 2)   /* control (on/off) & data (beware the fast acquisition vs the slow transmission) */
#define BACK_ULTRASONIC (1 << 3)    /* see above */
#define BOTTOM_ULTRASONIC (1 << 4)  /* see above */
#define FRONT_SENSOR_SERVO (1 << 5) /* control & position (pwm value) */
#define BMP280_SENSOR (1 << 6)      /* control (on/off) & pressure information */
#define MPU6050_SENSOR (1 << 7)
#define SDCARD_DEVICE (1 << 8) /* read file, status, control */
#define LED_ONBOARD (1 << 9)
#define LED_UART_EXTERNAL (1 << 10)

#define CORE_SYSTEM_STATUS (1 << 14)    /* when requested infos are temperature, device uuid, active threads, avg load, uptime, etc etc... */
#define CORE_SYSTEM_DEEPSLEEP (1 << 15) /* special flag, will take precedence over all the rest, when received the device goes into deepsleep mode!! */

/**
 * @brief  Action, flags
 * @note   
 * @retval None
 */
#define CONTROL_OFF (1 << 0) /* Commands the device to turn on */
#define CONTROL_ON (1 << 1)  /* Commands the device to turn off - Beware, nav control will always have the last word. If you ask the bottom sensor */
                             /*to turn off, the Nav Control will immediately stop the engines to avoid putting the robot in a situation of danger. */
                             /*Also, if you command the right motor to turn off, the robot will not hit something left if it detects an obstacle */
#define REQ_STATUS (1 << 2)  /* requests a full status, payload may vary according to the available data */
#define REQ_DATA (1 << 3)    /* requests data acquisition & transmission for the selected device(s)  */
#define SEND_DATA (1 << 4)   /* Informs the device that arguments are available along with the command.. ie Buzzer, play tune 6 */
#define REQ_READY (1 << 5)   /* requests the other MCU if it is ready to accept communications - adapt code accordingly */
#define ACK_READY (1 << 6)   /* acknowledge ready status */

/**
 * @brief  ESP32 Devices
 * @note   
 * @retval None
 */
#define BUZZER_DEVICE (1 << 0)          /* play tune command essentially */
#define CORE_SYSTEM_DATA (1 << 14)      /* when requested infos are temperature, device uuid, active threads, avg load, uptime, etc etc... */
#define CORE_SYSTEM_DEEPSLEEP (1 << 15) /* special flag, will take precedence over all the rest, when received the device goes into deepsleep mode!! */

#define UART_PAYLOAD_MAX_LGTH 255 /* max command length MUST BE 100 below MAX_HDLC_FRAME_LENGTH */

//uint16_t cmdActionFlags;

typedef enum
{
    CMD_RECEIVED,
    CMD_TRANSMIT
} commandRoute_t;

typedef enum
{
    CMD_TYPE_JSON,
    CMD_TYPE_TEXT
} command_type_t;

/* this will be sent out thru xCommandQueue, can be anything from JSON to a simple text command coming from telnet */
struct command_package_t
{
    commandRoute_t cmd_route;
    command_type_t cmd_type;
    char txtCommand[255]; /* in the case of a simple text command, we put it here, the stm will parse it as it wishes */
};

//uint16_t DeviceFlag;

uint8_t uSetupCmdParser();
void cmd_parse(const char *command);

#endif /* _IC_COMMAND_PARSER_H */
