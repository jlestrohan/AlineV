/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 23:19:39
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-01 16:48:29
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_NTP_SERVICE_H
#define _IC_NTP_SERVICE_H

#include <stdint.h>

uint8_t uSetupNTPService();

uint32_t ulGetEpochTime();

#endif /* _IC_NTP_SERVICE_H */
