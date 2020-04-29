/**
 ******************************************************************************
 * @file    sdcard_service.h
 * @author  Jack Lestrohan
 * @brief   SD Card initial handler service file header
 ******************************************************************************
 * @attention
 *					Connect pins:
 *
 *						PB13	->	SD_SCK
 *						PB14	->	SD_MISO
 *						PB15	->	SD_MOSI
 *						PA4		->	CS
 *
 ******************************************************************************
 */

#ifndef INC_SDCARD_SERVICE_H_
#define INC_SDCARD_SERVICE_H_

#include <stdint.h>

uint8_t uSdCardServiceInit();

#endif /* INC_SDCARD_SERVICE_H_ */
