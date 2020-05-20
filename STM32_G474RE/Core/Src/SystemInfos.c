/*******************************************************************
 * SystemInfos.c
 *
 *  Created on: 14 avr. 2020
 *      Author: aez03
 *
 *		ADC5 Ranks:
 *						1 = Temperature
 *						2 = VBat
 *						3 = VRefInt
 *
 *******************************************************************/

/**
 * Calibration Data for G474RE from datasheet https://www.st.com/resource/en/datasheet/stm32g474re.pdf
 *
 * 	TS_CAL1 - TS ADC raw data acquired at a temperature of 30 °C (± 5 °C),		VDDA = VREF+ = 3.0 V (± 10 mV)
		0x1FFF 75A8 - 0x1FFF 75A9

		TS_CAL2 - TS ADC raw data acquired at a	temperature of 110 °C (± 5 °C),	VDDA = VREF+ = 3.0 V (± 10 mV)
		0x1FFF 75CA - 0x1FFF 75CB

		VREFINT - Raw data acquired at a temperature of 30 °C (± 5 °C), VDDA = VREF+ = 3.0 V (± 10 mV)
		0x1FFF 75AA - 0x1FFF 75AB
 *
 */
#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFF75CA))
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFF75A8))
#define VREFINT_CAL ((uint16_t*) ((uint32_t) 0x1FFF75AA))
#define VDD_CALIB ((uint16_t) (330)) // 3.3V rail
#define VDD_APPLI ((uint16_t) (328)) // actually reads 3.28V on multimeter

#define STM32_UUID ((uint32_t *)0x1FFF7590)

#include "SystemInfos.h"
#include "configuration.h"
#include <stdlib.h>
#include <stdint.h>
#include "printf.h"
#include "main.h"
#include "adc.h"

//double readProcessorTemperature (uint32_t temperature_adc);

osMessageQueueId_t xQueueDmaAdcInternalSensors;
DMAInternalSensorsAdcValues_t DMAInternalSensorsAdcValues;
uint32_t ADC_BUF[3];
CoreInfo_t CoreInfo;

UART_HandleTypeDef hlpuart1;

static osThreadId_t xSysteminfoServiceTaskHandle;
static osStaticThreadDef_t  xSysteminfoServiceTaControlBlock;
static uint32_t  xSysteminfoServiceTaBuffer[1024];
static const osThreadAttr_t xSysteminfoServiceTa_attributes = {
		.name = "xSysteminfoServiceTask",
		.stack_mem = & xSysteminfoServiceTaBuffer[0],
		.stack_size = sizeof(xSysteminfoServiceTaBuffer),
		.cb_mem = &xSysteminfoServiceTaControlBlock,
		.cb_size = sizeof( xSysteminfoServiceTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_SYSTEMINFO
};

/**
 *
 */
void xSysteminfoServiceTaskStart(void *vParameters)
{
	HAL_ADC_Start_DMA(&hadc5, (uint32_t *)ADC_BUF, 3);
	HAL_ADC_Start_IT(&hadc5);

	DMAInternalSensorsAdcValues_t AdcValues;
	osStatus_t status;
	uint32_t proctemp;

	for (;;)
	{
		status = osMessageQueueGet(xQueueDmaAdcInternalSensors, &AdcValues, 0U, 0U);
		if (status == osOK) {

			//printf("Temperature = %f, VBat = %lu, VRefIn = %lu", readProcessorTemperature(AdcValues.adc0), AdcValues.adc1, AdcValues.adc2);
			//proctemp = readProcessorTemperature(AdcValues.adc0);

			//printf("Temperature = %f", proc_temperature);
			//HAL_ADC_Start_IT(&hadc5);
		}

		osDelay(200);
	}
	osThreadTerminate(xSysteminfoServiceTaskHandle);
}

/**
 * System Information Main Initialization Routine
 * @return
 */
uint8_t uSystemInfoServiceInit()
{
	/* creating queue for DMA Internal sensors ADC */
	xQueueDmaAdcInternalSensors = osMessageQueueNew(10, sizeof(DMAInternalSensorsAdcValues_t), NULL);
	if (xQueueDmaAdcInternalSensors == NULL) {
		printf("HR04 Sensor Queue Initialization Failed");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	/* creation of SysteminfoService Task */
	xSysteminfoServiceTaskHandle = osThreadNew(xSysteminfoServiceTaskStart, NULL, &xSysteminfoServiceTa_attributes);
	if (xSysteminfoServiceTaskHandle == NULL) {
		printf("System Info Task Initialization Failed");
		return (EXIT_FAILURE);
	}

	/* saves up the UUID into the system public structure */
	sprintf(CoreInfo.device_uuid, "0x%lx 0x%lx 0x%lx", STM32_UUID[0], STM32_UUID[1], STM32_UUID[2]);

	return EXIT_SUCCESS;
}

/*double readProcessorTemperature (uint32_t *temperature_adc)
{
	double proc_temp = ((*temperature_adc * VDD_APPLI / VDD_CALIB) - (int32_t)*TEMP30_CAL_ADDR);
	proc_temp *= (110-30);
		proc_temp -= (int32_t) *TEMP30_CAL_ADDR;
	proc_temp *= (int32_t)(110-30);
	proc_temp /= (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
	proc_temp += 30;
	return‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍‍ (proc_temp);
}*/

