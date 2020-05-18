/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-18 09:28:50
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-18 16:51:53
 * @ Description:   Handles the forwarding/emitting of the datas over the cloud.
 
 * If it is in forwarding mode, it will take the data sent by the STM, recreate a JSON 
 * including time and several additional infos from the ESP, and packages the thing for AWS
 
 * This service is only dedicated to the exchanges between the ESP32 AND amazon AWS
 
 * One task is dedicated in receiving structs from the command center which are directly coming 
 * from the STM32 (datas only) AND the ESP32 (additional sensors, speed etc...)
 * These DATAS are rebuilt into an AWS enveloppe (json) including a few more information:
 * Timestamp, ESPUUID, Mac Address... 
 * These datas are conformed to the AWS dedicated format that the cloud dev have decided. 
 
 * One other task is dedicated in receving the data from AWS.
 * According to the content of the json received, and after deserialization, that task is
 * able to determine if:
 *          - this is a command TO the ESP32
 *          - this is a command TO the STM32
 *      using the dedicated command_center service.
 
 * The vehicle is then fully able to obey directives coming from any AWS service available,
 * and yet more to come... deep learning, fleet management etc etc...
 
 *******************************************************************************************/

#ifndef _IC_AWS_SERVICE_H
#define _IC_AWS_SERVICE_H

#include "FreeRTOS.h"
#include <stdint.h>
#include "configuration_esp32.h"

typedef struct
{
    char key[20];
    char value[20];
} aws_keyValue_t;

typedef struct
{
    char stm_uuid[30];              /* stm32 UUID */
    char cmd_type[5];               /* cmd_type 3 letters */
    aws_keyValue_t keys_values[50]; /* info array - max data topics */
} cmd2aws_package_t;

uint8_t uSetuoAwsService();

#endif /* _IC_AWS_SERVICE_H */
