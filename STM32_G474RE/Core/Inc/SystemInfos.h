/*******************************************************************
 * SystemInfos.h
 *
 *  Created on: 14 avr. 2020
 *      Author: Jack Lestrohan
 *
 *      Very low priority task to fill up a struct tat contains all
 *      informations about the current processor
 *      	- UUID
 *      	- flash size
 *      	- internal temperature
 *      	- Internal voltage
 *
 *******************************************************************/

#ifndef INC_SYSTEMINFOS_H_
#define INC_SYSTEMINFOS_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"

typedef struct {
	uint32_t adc0;
	uint32_t adc1;
	uint32_t adc2;
} DMAInternalSensorsAdcValues_t;
extern DMAInternalSensorsAdcValues_t DMAInternalSensorsAdcValues;

extern osMessageQueueId_t xQueueDmaAdcInternalSensors;
extern uint32_t ADC_BUF[3];

typedef struct {
	char device_uuid[15];
	uint8_t	batteryStatus_percent;
} CoreInfo_t;

/******************************************************/
/** PUBLIC Structure with general service updated infos
 */
extern CoreInfo_t CoreInfo;
/******************************************************/

uint8_t uSystemInfoServiceInit();

#endif /* INC_SYSTEMINFOS_H_ */
