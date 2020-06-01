/**
 *****************************************************************************************************
 * sensor_hr04_service.c
 *
 *  Created on: Mar 21, 2020
 *      Author: Jack Lestrohan
 *
 *
 *	Timers and Pinout:
 *
 *		Sonar				Timer	PWM Channel		Echo Channels		Trig Pin	Echo Pin
 *		--------------------------------------------------------------------------------------------------
 *		HC_SR04_SONAR_REAR		TIM1 	3				Dir1, Ind2			PC2			PC0		REAR
 *		HC_SR04_SONAR_FRONT		TIM2	3				Dir1, Ind2			PB10		PA0		FRONT
 *		HC_SR04_SONAR_BOTTOM	TIM3	3				Dir1, Ind2			PB0			PA6		BOTTOM
 *
 *********************************************************************************************************
 */

#include <stdlib.h>
#include "configuration.h"
#include <assert.h>
#include "main.h"
#include "gpio.h"
#include <FreeRTOS.h>
#include <HCSR04_service.h>
#include "MG90S_service.h"
#include "IRQ_Handler.h"
#include "tim.h"
#include "printf.h"
#include <string.h>
#include <stdio.h>

#define SERVO_TOLERANCE 5

/* functions definitions */
static uint16_t median_filter(uint16_t datum);
static uint8_t isInsideTolerance(uint8_t value, uint8_t position, uint8_t tolerance);

/* mutexed variables */
HR04_SensorsData_t HR04_SensorsData;
HR04_SensorsData_t HR04_OldSensorsData;
osMutexId_t mHR04_SensorsDataMutex;
osMessageQueueId_t xQueueHCSR04DataSend;

osMessageQueueId_t xQueueMg90sMotionOrder; /* extern */
osMutexId_t mServoPositionMutex; /* extern */

/* flag to set any sensors active/inactive according to nav control decisions */
static osThreadId_t xHr04SensorTaskHandle;
static osStaticThreadDef_t xHr04SensorTaControlBlock;
static uint32_t xHr04SensorTaBuffer[256];
static const osThreadAttr_t xHr04SensorTa_attributes = {
		.stack_mem = &xHr04SensorTaBuffer[0],
		.stack_size = sizeof(xHr04SensorTaBuffer),
		.name = "xHr04SensorServiceTask",
		.cb_size = sizeof(xHr04SensorTaControlBlock),
		.cb_mem = &xHr04SensorTaControlBlock,
		.priority = (osPriority_t) OSTASK_PRIORITY_HCSR04, };

/* median filter */
#define STOPPER 0                                      /* Smaller than any datum */
#define    MEDIAN_FILTER_SIZE    (5)

/* functions definitions */
static uint8_t HC_SR04_StartupTimers();


/**
 *	HR04 Sensors Task
 * @param argument
 */
static void vHr04SensorTaskStart(void *argument)
{
	printf("Starting HCSR_04 Service task...\n\r");

	HR04_SensorRaw sensorCapturedData;
	osStatus_t status;
	register uint16_t sensread;

	MUTEX_SERVO_TAKE
	xServoPosition_t xServoPos;
	MUTEX_SERVO_GIVE

	for (;;)
	{
		/* we accept data from the IRQ to populate the HR04_SensorsData after filtering garbage out */
		status = osMessageQueueGet(xQueueHCSR04DataSend, &sensorCapturedData, 0U, osWaitForever);
		if (status == osOK)
		{
			HR04_OldSensorsData = HR04_SensorsData;
			sensread = sensorCapturedData.distance_data;
			xServoPos = xGetServoPosition();

			MUTEX_HCSR04_TAKE
			/* now let's filter out and populate data from the IRQ; */

			switch (sensorCapturedData.sensor_number) {
			case HR04_SONAR_REAR:
				HR04_SensorsData.dist_rear = sensread;
				break;
			case HR04_SONAR_BOTTOM:
				HR04_SensorsData.dist_bottom = sensread;
				break;
			case HR04_SONAR_FRONT:
				MUTEX_SERVO_TAKE

				if (isInsideTolerance(xServoPos, SERVO_DIRECTION_LEFT45, SERVO_TOLERANCE)) {
					HR04_SensorsData.dist_left45 = sensread;
				} else if (isInsideTolerance(xServoPos, SERVO_DIRECTION_CENTER, SERVO_TOLERANCE)) {
					HR04_SensorsData.dist_front = sensread;
				} else if (isInsideTolerance(xServoPos, SERVO_DIRECTION_RIGHT45, SERVO_TOLERANCE)) {
					HR04_SensorsData.dist_right45 = sensread;
				}

				MUTEX_SERVO_GIVE
				break;

						default: break;
			}
			MUTEX_HCSR04_GIVE
		}

#ifdef DEBUG_HCSR04_ALL
		MUTEX_HCSR04_TAKE
		printf("left45: %0*d - center: %0*d - right45: %0*d - bot: %0*d - rear: %0*d\n\r", 3,
				HR04_SensorsData.dist_left45, 3, HR04_SensorsData.dist_front, 3, HR04_SensorsData.dist_right45,
				3,HR04_SensorsData.dist_bottom, 3,HR04_SensorsData.dist_rear);
		MUTEX_HCSR04_GIVE
#endif

		osDelay(1);
	}
	osThreadTerminate(NULL);
}

/**
 * Initialization function
 * @return
 */
uint8_t uHcsr04ServiceInit()
{
	/* create mutex for struct protection */
	mHR04_SensorsDataMutex = osMutexNew(NULL);

	xQueueHCSR04DataSend = osMessageQueueNew(10,  sizeof(HR04_SensorRaw), NULL);
	if (xQueueHCSR04DataSend == NULL) {
		printf("Error Initializing xQueueHCSR04DataSend HCSR04 Queue...\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}


	/* creation of HR04Sensor1_task */
	xHr04SensorTaskHandle = osThreadNew(vHr04SensorTaskStart, NULL, &xHr04SensorTa_attributes);
	if (xHr04SensorTaskHandle == NULL) {
		printf("HR04 Sensor Task Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	printf("Initializing HC-SR04 Service... Success!\n\r");

	if (HC_SR04_StartupTimers() != EXIT_SUCCESS) {
		printf("HC_SR04 Timers Initialization Failed\n\r");
		Error_Handler();
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/**
 * Starts up the different timers
 * @return
 */
static uint8_t HC_SR04_StartupTimers()
{
	TIM_HandleTypeDef timHandlers[] = {htim1, htim2, htim3};

	for ( int i=0; i< HC_SR04_SONARS_CNT; i++) {
		/* starst up the different channels for Sensor 1 */
		if (HAL_TIM_PWM_Start(&*(timHandlers+i), TIM_CHANNEL_3) != HAL_OK) {
			printf("Unable to start HVSR-04 PWM timer\n\r");
			return (EXIT_FAILURE);
		}

		if (HAL_TIM_IC_Start(&*(timHandlers+i), TIM_CHANNEL_1) != HAL_OK) {
			printf("Unable to start HVSR-04 Rising Edge timer\n\r");
			return (EXIT_FAILURE);
		}

		if (HAL_TIM_IC_Start_IT(&*(timHandlers+i), TIM_CHANNEL_2) != HAL_OK) {
			printf("Unable to start HVSR-04 Falling Edge timer\n\r");
			return (EXIT_FAILURE);
		}
	}

	return (EXIT_SUCCESS);
}

/**
 * Median filter to filter out values
 * @param datum
 * @return
 */
static inline uint16_t median_filter(uint16_t datum)
{
	struct pair
	{
		struct pair   *point;                              /* Pointers forming list linked in sorted order */
		uint16_t  value;                                   /* Values to sort */
	};
	static struct pair buffer[MEDIAN_FILTER_SIZE] = {0}; /* Buffer of nwidth pairs */
	static struct pair *datpoint = buffer;               /* Pointer into circular buffer of data */
	static struct pair small = {NULL, STOPPER};          /* Chain stopper */
	static struct pair big = {&small, 0};                /* Pointer to head (largest) of linked list.*/

	struct pair *successor;                              /* Pointer to successor of replaced data item */
	struct pair *scan;                                   /* Pointer used to scan down the sorted list */
	struct pair *scanold;                                /* Previous value of scan */
	struct pair *median;                                 /* Pointer to median */
	uint16_t i;

	if (datum == STOPPER)
	{
		datum = STOPPER + 1;                             /* No stoppers allowed. */
	}

	if ( (++datpoint - buffer) >= MEDIAN_FILTER_SIZE)
	{
		datpoint = buffer;                               /* Increment and wrap data in pointer.*/
	}

	datpoint->value = datum;                           /* Copy in new datum */
	successor = datpoint->point;                       /* Save pointer to old value's successor */
	median = &big;                                     /* Median initially to first in chain */
	scanold = NULL;                                    /* Scanold initially null. */
	scan = &big;                                       /* Points to pointer to first (largest) datum in chain */

	/* Handle chain-out of first item in chain as special case */
	if (scan->point == datpoint)
	{
		scan->point = successor;
	}
	scanold = scan;                                     /* Save this pointer and   */
	scan = scan->point ;                                /* step down chain */

	/* Loop through the chain, normal loop exit via break. */
	for (i = 0 ; i < MEDIAN_FILTER_SIZE; ++i)
	{
		/* Handle odd-numbered item in chain  */
		if (scan->point == datpoint)
		{
			scan->point = successor;                      /* Chain out the old datum.*/
		}

		if (scan->value < datum)                        /* If datum is larger than scanned value,*/
		{
			datpoint->point = scanold->point;             /* Chain it in here.  */
			scanold->point = datpoint;                    /* Mark it chained in. */
			datum = STOPPER;
		};

		/* Step median pointer down chain after doing odd-numbered element */
		median = median->point;                       /* Step median pointer.  */
		if (scan == &small)
		{
			break;                                      /* Break at end of chain  */
		}
		scanold = scan;                               /* Save this pointer and   */
		scan = scan->point;                           /* step down chain */

		/* Handle even-numbered item in chain.  */
		if (scan->point == datpoint)
		{
			scan->point = successor;
		}

		if (scan->value < datum)
		{
			datpoint->point = scanold->point;
			scanold->point = datpoint;
			datum = STOPPER;
		}

		if (scan == &small)
		{
			break;
		}

		scanold = scan;
		scan = scan->point;
	}
	return (median->value);
}

/**
 * returns true if position - toleranbce / value / position +tolerance
 * @param position
 * @param tolerance
 * @return
 */
static uint8_t isInsideTolerance(uint8_t value, uint8_t position, uint8_t tolerance)
{
	return ((value <= position + tolerance) && (value >= position - tolerance));
}

