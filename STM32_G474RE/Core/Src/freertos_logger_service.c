/**
 ******************************************************************************
 * @file    logger_service.c
 * @author  Jack Lestrohan
 * @brief   Logger service file
 ******************************************************************************
 */
#include <freertos_logger_service.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include <string.h>
#include "usart.h"
#include <FreeRTOS.h>

#define		MESSAGE_BUFFER		200

UART_HandleTypeDef *_huartHandler;

/**
 * message queue definition
 */
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTask_t osStaticThreadDef_t;

typedef struct
{                                // object data type
	char msgBuf[MESSAGE_BUFFER];
	uint16_t msgIncId;
	uint8_t priority;
} MSGQUEUE_OBJ_t;
static MSGQUEUE_OBJ_t msg;

//static uint8_t queue_loggerBuffer[10 * sizeof(MSGQUEUE_OBJ_t)];
//static osStaticMessageQDef_t queue_loggerControlBlock;

static uint16_t incMsgIdCounter = 0;
static osMessageQueueId_t queue_loggerHandle;
osMutexId_t mutex_loggerService_Hnd;

static uint8_t status;

typedef enum
{
	logServiceNotInit, logServiceinitOK,
} logServiceStatus;
logServiceStatus logStatus = logServiceNotInit;

/**
 * Definitions for LoggerServiceTask
 */
osThreadId_t LoggerServiceTaHandle;
uint32_t LoggerServiceTaBuffer[256];
osStaticThreadDef_t LoggerServiceTaControlBlock;
const osThreadAttr_t LoggerServiceTa_attributes = { .name = "LoggerServiceTask",
        .stack_mem = &LoggerServiceTaBuffer[0],
        .stack_size = sizeof(LoggerServiceTaBuffer),
        .cb_mem = &LoggerServiceTaControlBlock,
        .cb_size = sizeof(LoggerServiceTaControlBlock),
        .priority = (osPriority_t) osPriorityLow, };

uint8_t _serviceStatus = false;
void StartLoggerServiceTask(void *argument);

/**
 * Initialize log service - must be called once at the start of the program
 * @param huart
 */
uint8_t log_initialize(UART_HandleTypeDef *huart)
{
	assert_param(huart);

	_huartHandler = huart;
	queue_loggerHandle = osMessageQueueNew(10, sizeof(MSGQUEUE_OBJ_t), NULL);
	if (!queue_loggerHandle) return (EXIT_FAILURE);
	mutex_loggerService_Hnd = osMutexNew(NULL);
	logStatus = logServiceinitOK;

	/* creation of LoggerServiceTask */
	LoggerServiceTaHandle = osThreadNew(StartLoggerServiceTask, NULL, &LoggerServiceTa_attributes);
	if (!LoggerServiceTaHandle) return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

/**
 * raw logger function, must be called from macros
 * @param log_msg
 * @param priority
 * @return
 */
void log_service(char *log_msg, LogPriority priority)
{
	assert_param(log_msg);

	/* if (logStatus != logServiceinitOK) log_initialize(); */

	incMsgIdCounter++;
	/*  fill up the queue with the logger message given as argument */

	memcpy(msg.msgBuf, log_msg, MESSAGE_BUFFER);
	msg.msgIncId = incMsgIdCounter;
	msg.priority = priority;
	osMessageQueuePut(queue_loggerHandle, &msg, 0U, osWaitForever);
}

/**
 * Returns a human readable log priority message according to the input LogPriority type provided
 * @param priority
 * @return
 */
char* decodeLogPriority(LogPriority priority)
{
	switch (priority)
	{
		case LOG_VERBOSE:
			return ("[VERBOSE]");
			break;
		case LOG_INFO:
			return ("[INFO]");
			break;
		case LOG_WARNING:
			return ("[WARNING]");
			break;
		case LOG_ERROR:
			return ("[ERROR]");
			break;
		case LOG_ALERT:
			return ("[ALERT]");
			break;
		default:
			return ("");
			break;
	}
}

/**
 * processes the dequeuing and sends to the UART
 * must be called from a task
 */
void log_processUart_task()
{

	osMessageQueueGet(queue_loggerHandle, &msg, NULL, osWaitForever); // wait for message
	if (status == osOK) {
		char finalmsg[MESSAGE_BUFFER + 50];

		sprintf(finalmsg, "(id) %04d | (timestamp) %08lu %s | %s\r\n", msg.msgIncId, osKernelGetTickCount(), decodeLogPriority(msg.priority), msg.msgBuf);

		status = osMutexAcquire(mutex_loggerService_Hnd, 0U); // will wait until mutex is ok
		HAL_UART_Transmit(_huartHandler, (uint8_t*) finalmsg, strlen(finalmsg), 0xFFFF);
		osMutexRelease(mutex_loggerService_Hnd);
	}

}

/**
 * main logger service task
 * @param argument
 */
void StartLoggerServiceTask(void *argument)
{
	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		log_processUart_task();
		/* BEGIN Add code here if needed */

		/* END Add code here if needed */
		osDelay(1);
	}
	osThreadTerminate(LoggerServiceTaHandle);
}
